// SPDX-License-Identifier: GPL-2.0-only
/*
 * inode.c
 *
 * PURPOSE
 *  Inode handling routines for the OSTA-UDF(tm) filesystem.
 *
 * COPYRIGHT
 *  (C) 1998 Dave Boynton
 *  (C) 1998-2004 Ben Fennema
 *  (C) 1999-2000 Stelias Computing Inc
 *
 * HISTORY
 *
 *  10/04/98 dgb  Added rudimentary directory functions
 *  10/07/98      Fully working udf_block_map! It works!
 *  11/25/98      bmap altered to better support extents
 *  12/06/98 blf  partition support in udf_iget, udf_block_map
 *                and udf_read_inode
 *  12/12/98      rewrote udf_block_map to handle next extents and descs across
 *                block boundaries (which is not actually allowed)
 *  12/20/98      added support for strategy 4096
 *  03/07/99      rewrote udf_block_map (again)
 *                New funcs, inode_bmap, udf_next_aext
 *  04/19/99      Support for writing device EA's for major/minor #
 */

#include "formats/udf/udfdecl.h"

#define UDF_MAX_INDIR_EXTS 16

int udf_next_aext(struct inode *inode, struct extent_position *epos,
		  struct kernel_lb_addr *eloc, uint32_t *elen, int8_t *etype,
		  int inc)
{
	unsigned int indirections = 0;
	int ret = 0;
	udf_pblk_t block;

	while (1) {
		ret = udf_current_aext(inode, epos, eloc, elen,
				       etype, inc);
		if (ret <= 0)
			return ret;
		if (*etype != (EXT_NEXT_EXTENT_ALLOCDESCS >> 30))
			return ret;

		if (++indirections > UDF_MAX_INDIR_EXTS) {
			udf_err(inode->i_sb,
				"too many indirect extents in inode %llu\n",
				inode->i_ino);
			return -EFSCORRUPTED;
		}

		epos->block = *eloc;
		epos->offset = sizeof(struct allocExtDesc);
		brelse(epos->bh);
		block = udf_get_lb_pblock(inode->i_sb, &epos->block, 0);
		epos->bh = sb_bread(inode->i_sb, block);
		if (!epos->bh) {
			udf_debug("reading block %u failed!\n", block);
			return -EIO;
		}
	}
}

int udf_current_aext(struct inode *inode, struct extent_position *epos,
		     struct kernel_lb_addr *eloc, uint32_t *elen, int8_t *etype,
		     int inc)
{
	int alen;
	uint8_t *ptr;
	struct short_ad *sad;
	struct long_ad *lad;
	struct udf_inode_info *iinfo = UDF_I(inode);

	if (!epos->bh) {
		if (!epos->offset)
			epos->offset = udf_file_entry_alloc_offset(inode);
		ptr = iinfo->i_data + epos->offset -
			udf_file_entry_alloc_offset(inode) +
			iinfo->i_lenEAttr;
		alen = udf_file_entry_alloc_offset(inode) +
							iinfo->i_lenAlloc;
	} else {
		struct allocExtDesc *header =
			(struct allocExtDesc *)epos->bh->b_data;

		if (!epos->offset)
			epos->offset = sizeof(struct allocExtDesc);
		ptr = epos->bh->b_data + epos->offset;
		if (check_add_overflow(sizeof(struct allocExtDesc),
				le32_to_cpu(header->lengthAllocDescs), &alen))
			return -1;

		if (alen > epos->bh->b_size)
			return -1;
	}

	switch (iinfo->i_alloc_type) {
	case ICBTAG_FLAG_AD_SHORT:
		sad = udf_get_fileshortad(ptr, alen, &epos->offset, inc);
		if (!sad)
			return 0;
		*etype = le32_to_cpu(sad->extLength) >> 30;
		eloc->logicalBlockNum = le32_to_cpu(sad->extPosition);
		eloc->partitionReferenceNum =
				iinfo->i_location.partitionReferenceNum;
		*elen = le32_to_cpu(sad->extLength) & UDF_EXTENT_LENGTH_MASK;
		break;
	case ICBTAG_FLAG_AD_LONG:
		lad = udf_get_filelongad(ptr, alen, &epos->offset, inc);
		if (!lad)
			return 0;
		*etype = le32_to_cpu(lad->extLength) >> 30;
		*eloc = lelb_to_cpu(lad->extLocation);
		*elen = le32_to_cpu(lad->extLength) & UDF_EXTENT_LENGTH_MASK;
		break;
	default:
		udf_debug("alloc_type = %u unsupported\n", iinfo->i_alloc_type);
		return -EINVAL;
	}

	if (eloc->partitionReferenceNum >= UDF_SB(inode->i_sb)->s_partitions) {
		udf_debug("invalid partition reference %u (partitions %u)\n",
			  eloc->partitionReferenceNum,
			  UDF_SB(inode->i_sb)->s_partitions);
		return -EFSCORRUPTED;
	}

	return 1;
}

#define UDF_MAX_ICB_NESTING 1024
#define FE_MAPPED_PERMS	(FE_PERM_U_READ | FE_PERM_U_WRITE | FE_PERM_U_EXEC | \
			 FE_PERM_G_READ | FE_PERM_G_WRITE | FE_PERM_G_EXEC | \
			 FE_PERM_O_READ | FE_PERM_O_WRITE | FE_PERM_O_EXEC)

static int udf_alloc_i_data(struct inode *, size_t);
static umode_t udf_convert_permissions(struct fileEntry *);

static int udf_read_inode(struct inode *inode, bool hidden_inode)
{
	struct buffer_head *bh = NULL;
	struct fileEntry *fe;
	struct extendedFileEntry *efe;
	uint16_t ident;
	struct udf_inode_info *iinfo = UDF_I(inode);
	struct udf_sb_info *sbi = UDF_SB(inode->i_sb);
	struct kernel_lb_addr *iloc = &iinfo->i_location;
	unsigned int link_count;
	unsigned int indirections = 0;
	int bs = inode->i_sb->s_blocksize;
	int ret = -EIO;
	uint32_t uid, gid;
	struct timespec64 ts;

reread:
	if (iloc->partitionReferenceNum >= sbi->s_partitions) {
		udf_debug("partition reference: %u > logical volume partitions: %u\n",
			  iloc->partitionReferenceNum, sbi->s_partitions);
		return -EIO;
	}

	if (iloc->logicalBlockNum >=
	    sbi->s_partmaps[iloc->partitionReferenceNum].s_partition_len) {
		udf_debug("block=%u, partition=%u out of range\n",
			  iloc->logicalBlockNum, iloc->partitionReferenceNum);
		return -EIO;
	}

	/*
	 * Set defaults, but the inode is still incomplete!
	 * Note: get_new_inode() sets the following on a new inode:
	 *      i_sb = sb
	 *      i_no = ino
	 *      i_flags = sb->s_flags
	 *      i_state = 0
	 * clean_inode(): zero fills and sets
	 *      i_count = 1
	 *      i_nlink = 1
	 *      i_op = NULL;
	 */
	bh = udf_read_ptagged(inode->i_sb, iloc, 0, &ident);
	if (!bh) {
		udf_err(inode->i_sb, "(ino %llu) failed !bh\n", inode->i_ino);
		return -EIO;
	}

	if (ident != TAG_IDENT_FE && ident != TAG_IDENT_EFE &&
	    ident != TAG_IDENT_USE) {
		udf_err(inode->i_sb, "(ino %llu) failed ident=%u\n",
			inode->i_ino, ident);
		goto out;
	}

	fe = (struct fileEntry *)bh->b_data;
	efe = (struct extendedFileEntry *)bh->b_data;

	if (fe->icbTag.strategyType == cpu_to_le16(4096)) {
		struct buffer_head *ibh;

		ibh = udf_read_ptagged(inode->i_sb, iloc, 1, &ident);
		if (ident == TAG_IDENT_IE && ibh) {
			struct kernel_lb_addr loc;
			struct indirectEntry *ie;

			ie = (struct indirectEntry *)ibh->b_data;
			loc = lelb_to_cpu(ie->indirectICB.extLocation);

			if (ie->indirectICB.extLength) {
				brelse(ibh);
				memcpy(&iinfo->i_location, &loc,
				       sizeof(struct kernel_lb_addr));
				if (++indirections > UDF_MAX_ICB_NESTING) {
					udf_err(inode->i_sb,
						"too many ICBs in ICB hierarchy"
						" (max %d supported)\n",
						UDF_MAX_ICB_NESTING);
					goto out;
				}
				brelse(bh);
				goto reread;
			}
		}
		brelse(ibh);
	} else if (fe->icbTag.strategyType != cpu_to_le16(4)) {
		udf_err(inode->i_sb, "unsupported strategy type: %u\n",
			le16_to_cpu(fe->icbTag.strategyType));
		goto out;
	}
	if (fe->icbTag.strategyType == cpu_to_le16(4))
		iinfo->i_strat4096 = 0;
	else /* if (fe->icbTag.strategyType == cpu_to_le16(4096)) */
		iinfo->i_strat4096 = 1;

	iinfo->i_alloc_type = le16_to_cpu(fe->icbTag.flags) &
							ICBTAG_FLAG_AD_MASK;
	if (iinfo->i_alloc_type != ICBTAG_FLAG_AD_SHORT &&
	    iinfo->i_alloc_type != ICBTAG_FLAG_AD_LONG &&
	    iinfo->i_alloc_type != ICBTAG_FLAG_AD_IN_ICB) {
		ret = -EIO;
		goto out;
	}
	iinfo->i_hidden = hidden_inode;
	iinfo->i_unique = 0;
	iinfo->i_lenEAttr = 0;
	iinfo->i_lenExtents = 0;
	iinfo->i_lenAlloc = 0;
	iinfo->i_next_alloc_block = 0;
	iinfo->i_next_alloc_goal = 0;
	if (fe->descTag.tagIdent == cpu_to_le16(TAG_IDENT_EFE)) {
		iinfo->i_efe = 1;
		iinfo->i_use = 0;
		ret = udf_alloc_i_data(inode, bs -
					sizeof(struct extendedFileEntry));
		if (ret)
			goto out;
		memcpy(iinfo->i_data,
		       bh->b_data + sizeof(struct extendedFileEntry),
		       bs - sizeof(struct extendedFileEntry));
	} else if (fe->descTag.tagIdent == cpu_to_le16(TAG_IDENT_FE)) {
		iinfo->i_efe = 0;
		iinfo->i_use = 0;
		ret = udf_alloc_i_data(inode, bs - sizeof(struct fileEntry));
		if (ret)
			goto out;
		memcpy(iinfo->i_data,
		       bh->b_data + sizeof(struct fileEntry),
		       bs - sizeof(struct fileEntry));
	} else if (fe->descTag.tagIdent == cpu_to_le16(TAG_IDENT_USE)) {
		iinfo->i_efe = 0;
		iinfo->i_use = 1;
		iinfo->i_lenAlloc = le32_to_cpu(
				((struct unallocSpaceEntry *)bh->b_data)->
				 lengthAllocDescs);
		if (iinfo->i_lenAlloc > bs - sizeof(struct unallocSpaceEntry)) {
			ret = -EFSCORRUPTED;
			goto out;
		}
		ret = udf_alloc_i_data(inode, bs -
					sizeof(struct unallocSpaceEntry));
		if (ret)
			goto out;
		memcpy(iinfo->i_data,
		       bh->b_data + sizeof(struct unallocSpaceEntry),
		       bs - sizeof(struct unallocSpaceEntry));
		inode->entry = bh;
		return 0;
	}

	ret = -EIO;
	read_lock(&sbi->s_cred_lock);
	uid = le32_to_cpu(fe->uid);
	if (uid == UDF_INVALID_ID ||
	    UDF_QUERY_FLAG(inode->i_sb, UDF_FLAG_UID_SET))
		inode->i_uid = sbi->s_uid;
	else
		i_uid_write(inode, uid);

	gid = le32_to_cpu(fe->gid);
	if (gid == UDF_INVALID_ID ||
	    UDF_QUERY_FLAG(inode->i_sb, UDF_FLAG_GID_SET))
		inode->i_gid = sbi->s_gid;
	else
		i_gid_write(inode, gid);

	if (fe->icbTag.fileType != ICBTAG_FILE_TYPE_DIRECTORY &&
			sbi->s_fmode != UDF_INVALID_MODE)
		inode->i_mode = sbi->s_fmode;
	else if (fe->icbTag.fileType == ICBTAG_FILE_TYPE_DIRECTORY &&
			sbi->s_dmode != UDF_INVALID_MODE)
		inode->i_mode = sbi->s_dmode;
	else
		inode->i_mode = udf_convert_permissions(fe);
	inode->i_mode &= ~sbi->s_umask;
	iinfo->i_extraPerms = le32_to_cpu(fe->permissions) & ~FE_MAPPED_PERMS;

	read_unlock(&sbi->s_cred_lock);

	link_count = le16_to_cpu(fe->fileLinkCount);
	if (!link_count) {
		if (!hidden_inode) {
			ret = -ESTALE;
			goto out;
		}
		link_count = 1;
	}
	set_nlink(inode, link_count);

	inode->i_size = le64_to_cpu(fe->informationLength);
	iinfo->i_lenExtents = inode->i_size;

	if (iinfo->i_efe == 0) {
		inode->i_blocks = le64_to_cpu(fe->logicalBlocksRecorded) <<
			(inode->i_sb->s_blocksize_bits - 9);

		udf_disk_stamp_to_time(&ts, fe->accessTime);
		inode_set_atime_to_ts(inode, ts);
		udf_disk_stamp_to_time(&ts, fe->modificationTime);
		inode_set_mtime_to_ts(inode, ts);
		udf_disk_stamp_to_time(&ts, fe->attrTime);
		inode_set_ctime_to_ts(inode, ts);

		iinfo->i_unique = le64_to_cpu(fe->uniqueID);
		iinfo->i_lenEAttr = le32_to_cpu(fe->lengthExtendedAttr);
		iinfo->i_lenAlloc = le32_to_cpu(fe->lengthAllocDescs);
		iinfo->i_checkpoint = le32_to_cpu(fe->checkpoint);
		iinfo->i_streamdir = 0;
		iinfo->i_lenStreams = 0;
	} else {
		inode->i_blocks = le64_to_cpu(efe->logicalBlocksRecorded) <<
		    (inode->i_sb->s_blocksize_bits - 9);

		udf_disk_stamp_to_time(&ts, efe->accessTime);
		inode_set_atime_to_ts(inode, ts);
		udf_disk_stamp_to_time(&ts, efe->modificationTime);
		inode_set_mtime_to_ts(inode, ts);
		udf_disk_stamp_to_time(&ts, efe->attrTime);
		inode_set_ctime_to_ts(inode, ts);
		udf_disk_stamp_to_time(&iinfo->i_crtime, efe->createTime);

		iinfo->i_unique = le64_to_cpu(efe->uniqueID);
		iinfo->i_lenEAttr = le32_to_cpu(efe->lengthExtendedAttr);
		iinfo->i_lenAlloc = le32_to_cpu(efe->lengthAllocDescs);
		iinfo->i_checkpoint = le32_to_cpu(efe->checkpoint);

		/* Named streams */
		iinfo->i_streamdir = (efe->streamDirectoryICB.extLength != 0);
		iinfo->i_locStreamdir =
			lelb_to_cpu(efe->streamDirectoryICB.extLocation);
		iinfo->i_lenStreams = le64_to_cpu(efe->objectSize);
		if (iinfo->i_lenStreams >= inode->i_size)
			iinfo->i_lenStreams -= inode->i_size;
		else
			iinfo->i_lenStreams = 0;
	}
	inode->i_generation = iinfo->i_unique;

	/*
	 * Sanity check length of allocation descriptors and extended attrs to
	 * avoid integer overflows
	 */
	if (iinfo->i_lenEAttr > bs || iinfo->i_lenAlloc > bs)
		goto out;
	/* Now do exact checks */
	if (udf_file_entry_alloc_offset(inode) + iinfo->i_lenAlloc > bs)
		goto out;
	/* Sanity checks for files in ICB so that we don't get confused later */
	if (iinfo->i_alloc_type == ICBTAG_FLAG_AD_IN_ICB) {
		/*
		 * For file in ICB data is stored in allocation descriptor
		 * so sizes should match
		 */
		if (iinfo->i_lenAlloc != inode->i_size)
			goto out;
		/* File in ICB has to fit in there... */
		if (inode->i_size > bs - udf_file_entry_alloc_offset(inode))
			goto out;
	}

	switch (fe->icbTag.fileType) {
	case ICBTAG_FILE_TYPE_DIRECTORY:
		inode->i_mode |= S_IFDIR;
		inc_nlink(inode);
		break;
	case ICBTAG_FILE_TYPE_REALTIME:
	case ICBTAG_FILE_TYPE_REGULAR:
	case ICBTAG_FILE_TYPE_UNDEF:
	case ICBTAG_FILE_TYPE_VAT20:
		inode->i_mode |= S_IFREG;
		break;
	case ICBTAG_FILE_TYPE_BLOCK:
		inode->i_mode |= S_IFBLK;
		break;
	case ICBTAG_FILE_TYPE_CHAR:
		inode->i_mode |= S_IFCHR;
		break;
	case ICBTAG_FILE_TYPE_FIFO:
		init_special_inode(inode, inode->i_mode | S_IFIFO, 0);
		break;
	case ICBTAG_FILE_TYPE_SOCKET:
		init_special_inode(inode, inode->i_mode | S_IFSOCK, 0);
		break;
	case ICBTAG_FILE_TYPE_SYMLINK:
		inode->i_mode = S_IFLNK | 0777;
		break;
	case ICBTAG_FILE_TYPE_MAIN:
		udf_debug("METADATA FILE-----\n");
		break;
	case ICBTAG_FILE_TYPE_MIRROR:
		udf_debug("METADATA MIRROR FILE-----\n");
		break;
	case ICBTAG_FILE_TYPE_BITMAP:
		udf_debug("METADATA BITMAP FILE-----\n");
		break;
	default:
		udf_err(inode->i_sb, "(ino %llu) failed unknown file type=%u\n",
			inode->i_ino, fe->icbTag.fileType);
		goto out;
	}
	if (S_ISCHR(inode->i_mode) || S_ISBLK(inode->i_mode)) {
		struct deviceSpec *dsea =
			(struct deviceSpec *)udf_get_extendedattr(inode, 12, 1);
		if (dsea && le32_to_cpu(dsea->attrLength) >= sizeof(*dsea)) {
			init_special_inode(inode, inode->i_mode,
				MKDEV(le32_to_cpu(dsea->majorDeviceIdent),
				      le32_to_cpu(dsea->minorDeviceIdent)));
			/* Developer ID ??? */
		} else
			goto out;
	}
	ret = 0;
out:
	if (!ret) {
		inode->entry = bh;
		return 0;
	}
	brelse(bh);
	return ret;
}

static void __udf_clear_extent_cache(struct inode *inode)
{
	struct udf_inode_info *iinfo = UDF_I(inode);

	if (iinfo->cached_extent.lstart != -1) {
		brelse(iinfo->cached_extent.epos.bh);
		iinfo->cached_extent.lstart = -1;
	}
}

void udf_clear_extent_cache(struct inode *inode)
{
	struct udf_inode_info *iinfo = UDF_I(inode);

	spin_lock(&iinfo->i_extent_cache_lock);
	__udf_clear_extent_cache(inode);
	spin_unlock(&iinfo->i_extent_cache_lock);
}

static int udf_read_extent_cache(struct inode *inode, int64_t bcount,
				 int64_t *lbcount, struct extent_position *pos)
{
	struct udf_inode_info *iinfo = UDF_I(inode);
	int ret = 0;

	spin_lock(&iinfo->i_extent_cache_lock);
	if ((iinfo->cached_extent.lstart <= bcount) &&
	    (iinfo->cached_extent.lstart != -1)) {
		/* Cache hit */
		*lbcount = iinfo->cached_extent.lstart;
		memcpy(pos, &iinfo->cached_extent.epos,
		       sizeof(struct extent_position));
		if (pos->bh)
			get_bh(pos->bh);
		ret = 1;
	}
	spin_unlock(&iinfo->i_extent_cache_lock);
	return ret;
}

static void udf_update_extent_cache(struct inode *inode, int64_t estart,
				    struct extent_position *pos)
{
	struct udf_inode_info *iinfo = UDF_I(inode);

	spin_lock(&iinfo->i_extent_cache_lock);
	/* Invalidate previously cached extent */
	__udf_clear_extent_cache(inode);
	if (pos->bh)
		get_bh(pos->bh);
	memcpy(&iinfo->cached_extent.epos, pos, sizeof(*pos));
	iinfo->cached_extent.lstart = estart;
	switch (iinfo->i_alloc_type) {
	case ICBTAG_FLAG_AD_SHORT:
		iinfo->cached_extent.epos.offset -= sizeof(struct short_ad);
		break;
	case ICBTAG_FLAG_AD_LONG:
		iinfo->cached_extent.epos.offset -= sizeof(struct long_ad);
		break;
	}
	spin_unlock(&iinfo->i_extent_cache_lock);
}

int inode_bmap(struct inode *inode, sector_t block, struct extent_position *pos,
	       struct kernel_lb_addr *eloc, uint32_t *elen, sector_t *offset,
	       int8_t *etype)
{
	unsigned char blocksize_bits = inode->i_sb->s_blocksize_bits;
	int64_t lbcount = 0, bcount = (int64_t) block << blocksize_bits;
	struct udf_inode_info *iinfo;
	int err = 0;
	struct sevenzip_udf_walk walk = {0};

	iinfo = UDF_I(inode);
	if (!udf_read_extent_cache(inode, bcount, &lbcount, pos)) {
		pos->offset = 0;
		pos->block = iinfo->i_location;
		pos->bh = NULL;
	}
	*elen = 0;
	do {
		err = sevenzip_udf_next_aext(&walk, inode, pos, eloc, elen, etype);
		if (err <= 0) {
			if (err == 0) {
				*offset = (bcount - lbcount) >> blocksize_bits;
				iinfo->i_lenExtents = lbcount;
			}
			return err;
		}
		lbcount += *elen;
	} while (lbcount <= bcount);
	/* update extent cache */
	udf_update_extent_cache(inode, lbcount - *elen, pos);
	*offset = (bcount + *elen - lbcount) >> blocksize_bits;

	return 1;
}


static int udf_alloc_i_data(struct inode *inode, size_t size)
{
	struct udf_inode_info *iinfo = UDF_I(inode);
	iinfo->i_data = kmalloc(size, GFP_KERNEL);
	if (!iinfo->i_data)
		return -ENOMEM;
	return 0;
}


static umode_t udf_convert_permissions(struct fileEntry *fe)
{
	umode_t mode;
	uint32_t permissions;
	uint32_t flags;

	permissions = le32_to_cpu(fe->permissions);
	flags = le16_to_cpu(fe->icbTag.flags);

	mode =	((permissions) & 0007) |
		((permissions >> 2) & 0070) |
		((permissions >> 4) & 0700) |
		((flags & ICBTAG_FLAG_SETUID) ? S_ISUID : 0) |
		((flags & ICBTAG_FLAG_SETGID) ? S_ISGID : 0) |
		((flags & ICBTAG_FLAG_STICKY) ? S_ISVTX : 0);

	return mode;
}

struct inode *__udf_iget(struct super_block *sb, struct kernel_lb_addr *ino,
			 bool hidden_inode)
{
	unsigned long block = udf_inode_block(sb, ino);
	struct inode *inode = iget_locked(sb, block);
	int err;

	if (!inode)
		return ERR_PTR(-ENOMEM);

	if (!(inode_state_read_once(inode) & I_NEW)) {
		if (UDF_I(inode)->i_hidden != hidden_inode) {
			iput(inode);
			return ERR_PTR(-EFSCORRUPTED);
		}
		return inode;
	}

	memcpy(&UDF_I(inode)->i_location, ino, sizeof(struct kernel_lb_addr));
	err = udf_read_inode(inode, hidden_inode);
	if (err < 0) {
		iget_failed(inode);
		return ERR_PTR(err);
	}
	unlock_new_inode(inode);

	return inode;
}

// SPDX-License-Identifier: GPL-2.0-only
/*
 * super.c
 *
 * PURPOSE
 *  Super block routines for the OSTA-UDF(tm) filesystem.
 *
 * DESCRIPTION
 *  OSTA-UDF(tm) = Optical Storage Technology Association
 *  Universal Disk Format.
 *
 *  This code is based on version 2.00 of the UDF specification,
 *  and revision 3 of the ECMA 167 standard [equivalent to ISO 13346].
 *    http://www.osta.org/
 *    https://www.ecma.ch/
 *    https://www.iso.org/
 *
 * COPYRIGHT
 *  (C) 1998 Dave Boynton
 *  (C) 1998-2004 Ben Fennema
 *  (C) 2000 Stelias Computing Inc
 *
 * HISTORY
 *
 *  09/24/98 dgb  changed to allow compiling outside of kernel, and
 *                added some debugging.
 *  10/01/98 dgb  updated to allow (some) possibility of compiling w/2.0.34
 *  10/16/98      attempting some multi-session support
 *  10/17/98      added freespace count for "df"
 *  11/11/98 gr   added novrs option
 *  11/26/98 dgb  added fileset,anchor mount options
 *  12/06/98 blf  really hosed things royally. vat/sparing support. sequenced
 *                vol descs. rewrote option handling based on isofs
 *  12/20/98      find the free space bitmap (if it exists)
 */

#include "formats/udf/udfdecl.h"

static int udf_describe_sparable_map(struct super_block *sb,
                                     struct udf_part_map *map, struct sparablePartitionMap *spm)
{
    struct udf_sparing_data *sdata = &map->s_type_specific.s_sparing;
	map->s_partition_type = UDF_SPARABLE_MAP15;
	sdata->s_packet_len = le16_to_cpu(spm->packetLength);
	if (!is_power_of_2(sdata->s_packet_len)) {
		udf_err(sb, "error loading logical volume descriptor: "
			"Invalid packet length %u\n",
			(unsigned)sdata->s_packet_len);
		return -EIO;
	}
	if (spm->numSparingTables > 4) {
		udf_err(sb, "error loading logical volume descriptor: "
			"Too many sparing tables (%d)\n",
			(int)spm->numSparingTables);
		return -EIO;
	}
	if (le32_to_cpu(spm->sizeSparingTable) > sb->s_blocksize) {
		udf_err(sb, "error loading logical volume descriptor: "
			"Too big sparing table size (%u)\n",
			le32_to_cpu(spm->sizeSparingTable));
		return -EIO;
	}

    return 0;
}

static int udf_load_sparable_map(struct super_block *sb,
				 struct udf_part_map *map,
				 struct sparablePartitionMap *spm)
{
	uint32_t loc;
	uint16_t ident;
	struct sparingTable *st;
	struct udf_sparing_data *sdata = &map->s_type_specific.s_sparing;
	int i;
	struct buffer_head *bh;

    int error = udf_describe_sparable_map(sb, map, spm);
    if (error)
        return error;

	for (i = 0; i < spm->numSparingTables; i++) {
		loc = le32_to_cpu(spm->locSparingTable[i]);
		bh = udf_read_tagged(sb, loc, loc, &ident);
		if (!bh)
			continue;

		st = (struct sparingTable *)bh->b_data;
		if (ident != 0 ||
		    strncmp((const char *)st->sparingIdent.ident, UDF_ID_SPARING,
			    strlen(UDF_ID_SPARING)) ||
		    struct_size(st, mapEntry,
				le16_to_cpu(st->reallocationTableLen)) >
							sb->s_blocksize) {
			brelse(bh);
			continue;
		}

		sdata->s_spar_map[i] = bh;
	}
	map->s_partition_func = udf_get_pblock_spar15;
	return 0;
}

int sevenzip_udf_load_sparable(struct super_block *sb, struct udf_part_map *map,
                               struct sparablePartitionMap *raw)
{
    return udf_load_sparable_map(sb, map, raw);
}

static void udf_find_vat_block(struct super_block *sb, int p_index,
			       int type1_index, sector_t start_block)
{
	struct udf_sb_info *sbi = UDF_SB(sb);
	struct udf_part_map *map = &sbi->s_partmaps[p_index];
	sector_t vat_block;
	struct kernel_lb_addr ino;
	struct inode *inode;

	/*
	 * VAT file entry is in the last recorded block. Some broken disks have
	 * it a few blocks before so try a bit harder...
	 */
	ino.partitionReferenceNum = type1_index;
	for (vat_block = start_block;
	     vat_block >= map->s_partition_root &&
	     vat_block >= (start_block > 3 ? start_block - 3 : 0); vat_block--) {
		ino.logicalBlockNum = vat_block - map->s_partition_root;
		inode = udf_iget_special(sb, &ino);
		if (!IS_ERR(inode)) {
			sbi->s_vat_inode = inode;
			break;
		}
		if (!vat_block)
			break;
	}
}

static int udf_load_vat(struct super_block *sb, int p_index, int type1_index)
{
	struct udf_sb_info *sbi = UDF_SB(sb);
	struct udf_part_map *map = &sbi->s_partmaps[p_index];
	struct buffer_head *bh = NULL;
	struct udf_inode_info *vati;
	struct virtualAllocationTable20 *vat20;
	sector_t blocks = sb_bdev_nr_blocks(sb);

	udf_find_vat_block(sb, p_index, type1_index, sbi->s_last_block);
	if (!sbi->s_vat_inode &&
	    sbi->s_last_block != blocks - 1) {
		pr_notice("Failed to read VAT inode from the last recorded block (%lu), retrying with the last block of the device (%lu).\n",
			  (unsigned long)sbi->s_last_block,
			  (unsigned long)blocks - 1);
		udf_find_vat_block(sb, p_index, type1_index, blocks - 1);
	}
	if (!sbi->s_vat_inode)
		return -EIO;
	if (sbi->s_vat_inode->i_size > (uint64_t)UINT32_MAX * 4)
		return -EFSCORRUPTED;

	if (map->s_partition_type == UDF_VIRTUAL_MAP15) {
		map->s_type_specific.s_virtual.s_start_offset = 0;
		if (sbi->s_vat_inode->i_size < 36) {
			udf_err(sb, "Too short VAT inode size %lld\n",
				sbi->s_vat_inode->i_size);
			return -EFSCORRUPTED;
		}
		map->s_type_specific.s_virtual.s_num_entries =
			(sbi->s_vat_inode->i_size - 36) >> 2;
	} else if (map->s_partition_type == UDF_VIRTUAL_MAP20) {
		if (sbi->s_vat_inode->i_size < sizeof(__le16))
			return -EFSCORRUPTED;
		vati = UDF_I(sbi->s_vat_inode);
		if (vati->i_alloc_type != ICBTAG_FLAG_AD_IN_ICB) {
			int err = 0;

			bh = udf_bread(sbi->s_vat_inode, 0, 0, &err);
			if (!bh) {
				if (!err)
					err = -EFSCORRUPTED;
				return err;
			}
			vat20 = (struct virtualAllocationTable20 *)bh->b_data;
		} else {
			vat20 = (struct virtualAllocationTable20 *)
							vati->i_data;
		}

		map->s_type_specific.s_virtual.s_start_offset =
			le16_to_cpu(vat20->lengthHeader);
		if ((map->s_type_specific.s_virtual.s_start_offset & 3) ||
		    map->s_type_specific.s_virtual.s_start_offset
		    > sbi->s_vat_inode->i_size) {
			udf_err(sb, "Corrupted VAT header length %u (VAT inode size %lld)\n",
				map->s_type_specific.s_virtual.s_start_offset,
				sbi->s_vat_inode->i_size);
			brelse(bh);
			return -EFSCORRUPTED;
		}
		map->s_type_specific.s_virtual.s_num_entries =
			(sbi->s_vat_inode->i_size -
				map->s_type_specific.s_virtual.
					s_start_offset) >> 2;
		brelse(bh);
	}
	return 0;
}

int sevenzip_udf_load_vat(struct super_block *sb, int virtual_ref, int backing_ref)
{
    return udf_load_vat(sb, virtual_ref, backing_ref);
}

struct inode *udf_find_metadata_inode_efe(struct super_block *sb,
					u32 meta_file_loc, u32 partition_ref)
{
	struct kernel_lb_addr addr;
	struct inode *metadata_fe;

	addr.logicalBlockNum = meta_file_loc;
	addr.partitionReferenceNum = partition_ref;

	metadata_fe = udf_iget_special(sb, &addr);

	if (IS_ERR(metadata_fe)) {
		udf_warn(sb, "metadata inode efe not found\n");
		return metadata_fe;
	}
	if (UDF_I(metadata_fe)->i_alloc_type != ICBTAG_FLAG_AD_SHORT) {
		udf_warn(sb, "metadata inode efe does not have short allocation descriptors!\n");
		iput(metadata_fe);
		return ERR_PTR(-EIO);
	}

	return metadata_fe;
}

static int udf_load_metadata_files(struct super_block *sb, int partition,
				   int type1_index)
{
	struct udf_sb_info *sbi = UDF_SB(sb);
	struct udf_part_map *map;
	struct udf_meta_data *mdata;
	struct kernel_lb_addr addr;
	struct inode *fe;

	map = &sbi->s_partmaps[partition];
	mdata = &map->s_type_specific.s_metadata;
	mdata->s_phys_partition_ref = type1_index;

	/* metadata address */
	udf_debug("Metadata file location: block = %u part = %u\n",
		  mdata->s_meta_file_loc, mdata->s_phys_partition_ref);

	fe = udf_find_metadata_inode_efe(sb, mdata->s_meta_file_loc,
					 mdata->s_phys_partition_ref);
	if (IS_ERR(fe)) {
		/* mirror file entry */
		udf_debug("Mirror metadata file location: block = %u part = %u\n",
			  mdata->s_mirror_file_loc, mdata->s_phys_partition_ref);

		fe = udf_find_metadata_inode_efe(sb, mdata->s_mirror_file_loc,
						 mdata->s_phys_partition_ref);

		if (IS_ERR(fe)) {
			udf_err(sb, "Both metadata and mirror metadata inode efe can not found\n");
			return PTR_ERR(fe);
		}
		mdata->s_mirror_fe = fe;
	} else
		mdata->s_metadata_fe = fe;


	/*
	 * bitmap file entry
	 * Note:
	 * Load only if bitmap file location differs from 0xFFFFFFFF (DCN-5102)
	*/
	if (mdata->s_bitmap_file_loc != 0xFFFFFFFF) {
		addr.logicalBlockNum = mdata->s_bitmap_file_loc;
		addr.partitionReferenceNum = mdata->s_phys_partition_ref;

		udf_debug("Bitmap file location: block = %u part = %u\n",
			  addr.logicalBlockNum, addr.partitionReferenceNum);

		fe = udf_iget_special(sb, &addr);
		if (IS_ERR(fe)) {
			if (sb_rdonly(sb))
				udf_warn(sb, "bitmap inode efe not found but it's ok since the disc is mounted read-only\n");
			else {
				udf_err(sb, "bitmap inode efe not found and attempted read-write mount\n");
				return PTR_ERR(fe);
			}
		} else
			mdata->s_bitmap_fe = fe;
	}

	udf_debug("udf_load_metadata_files Ok\n");
	return 0;
}

int sevenzip_udf_load_metadata(struct super_block *sb, int partition, int backing)
{
    return udf_load_metadata_files(sb, partition, backing);
}

enum { VDS_POS_PRIMARY_VOL_DESC, VDS_POS_UNALLOC_SPACE_DESC,
       VDS_POS_LOGICAL_VOL_DESC, VDS_POS_IMP_USE_VOL_DESC, VDS_POS_LENGTH };
#define VSD_FIRST_SECTOR_OFFSET 32768
#define VSD_MAX_SECTOR_OFFSET 0x800000
#define UDF_MAX_TD_NESTING 64
#define UDF_MAX_LVID_NESTING 1000

/* Selected descriptors are projected through the existing archive reader. */
static int udf_load_pvoldesc(struct super_block *, sector_t);
static int udf_load_logicalvol(struct super_block *, sector_t, struct kernel_lb_addr *);
static int udf_load_partdesc(struct super_block *, sector_t);
static void udf_sb_free_partitions(struct super_block *);

struct logicalVolIntegrityDescImpUse *udf_sb_lvidiu(struct super_block *sb)
{
	struct logicalVolIntegrityDesc *lvid;
	unsigned int partnum;
	unsigned int offset;

	if (!UDF_SB(sb)->s_lvid_bh)
		return NULL;
	lvid = (struct logicalVolIntegrityDesc *)UDF_SB(sb)->s_lvid_bh->b_data;
	partnum = le32_to_cpu(lvid->numOfPartitions);
	/* The offset is to skip freeSpaceTable and sizeTable arrays */
	offset = partnum * 2 * sizeof(uint32_t);
	return (struct logicalVolIntegrityDescImpUse *)
					(((uint8_t *)(lvid + 1)) + offset);
}

static int identify_vsd(const struct volStructDesc *vsd)
{
	int ret = 0;

	if (!memcmp(vsd->stdIdent, VSD_STD_ID_CD001, VSD_STD_ID_LEN)) {
		switch (vsd->structType) {
		case 0:
			udf_debug("ISO9660 Boot Record found\n");
			break;
		case 1:
			udf_debug("ISO9660 Primary Volume Descriptor found\n");
			break;
		case 2:
			udf_debug("ISO9660 Supplementary Volume Descriptor found\n");
			break;
		case 3:
			udf_debug("ISO9660 Volume Partition Descriptor found\n");
			break;
		case 255:
			udf_debug("ISO9660 Volume Descriptor Set Terminator found\n");
			break;
		default:
			udf_debug("ISO9660 VRS (%u) found\n", vsd->structType);
			break;
		}
	} else if (!memcmp(vsd->stdIdent, VSD_STD_ID_BEA01, VSD_STD_ID_LEN))
		; /* ret = 0 */
	else if (!memcmp(vsd->stdIdent, VSD_STD_ID_NSR02, VSD_STD_ID_LEN))
		ret = 1;
	else if (!memcmp(vsd->stdIdent, VSD_STD_ID_NSR03, VSD_STD_ID_LEN))
		ret = 1;
	else if (!memcmp(vsd->stdIdent, VSD_STD_ID_BOOT2, VSD_STD_ID_LEN))
		; /* ret = 0 */
	else if (!memcmp(vsd->stdIdent, VSD_STD_ID_CDW02, VSD_STD_ID_LEN))
		; /* ret = 0 */
	else {
		/* TEA01 or invalid id : end of volume recognition area */
		ret = -1;
	}

	return ret;
}

static int udf_check_vsd(struct super_block *sb)
{
	struct volStructDesc *vsd = NULL;
	int64_t sector = VSD_FIRST_SECTOR_OFFSET;
	int sectorsize;
	struct buffer_head *bh = NULL;
	int nsr = 0;
	struct udf_sb_info *sbi;
	int64_t session_offset;

	sbi = UDF_SB(sb);
	if (sb->s_blocksize < sizeof(struct volStructDesc))
		sectorsize = sizeof(struct volStructDesc);
	else
		sectorsize = sb->s_blocksize;

	session_offset = (int64_t)sbi->s_session << sb->s_blocksize_bits;
	sector += session_offset;

	udf_debug("Starting at sector %u (%lu byte sectors)\n",
		  (unsigned int)(sector >> sb->s_blocksize_bits),
		  sb->s_blocksize);
	/* Process the sequence (if applicable). The hard limit on the sector
	 * offset is arbitrary, hopefully large enough so that all valid UDF
	 * filesystems will be recognised. There is no mention of an upper
	 * bound to the size of the volume recognition area in the standard.
	 *  The limit will prevent the code to read all the sectors of a
	 * specially crafted image (like a bluray disc full of CD001 sectors),
	 * potentially causing minutes or even hours of uninterruptible I/O
	 * activity. This actually happened with uninitialised SSD partitions
	 * (all 0xFF) before the check for the limit and all valid IDs were
	 * added */
	for (; !nsr && sector < VSD_MAX_SECTOR_OFFSET; sector += sectorsize) {
		/* Read a block */
		bh = sb_bread(sb, sector >> sb->s_blocksize_bits);
		if (!bh)
			break;

		vsd = (struct volStructDesc *)(bh->b_data +
					      (sector & (sb->s_blocksize - 1)));
		nsr = identify_vsd(vsd);
		/* Found NSR or end? */
		if (nsr) {
			brelse(bh);
			break;
		}
		/*
		 * Special handling for improperly formatted VRS (e.g., Win10)
		 * where components are separated by 2048 bytes even though
		 * sectors are 4K
		 */
		if (sb->s_blocksize == 4096) {
			nsr = identify_vsd(vsd + 1);
			/* Ignore unknown IDs... */
			if (nsr < 0)
				nsr = 0;
		}
		brelse(bh);
	}

	if (nsr > 0)
		return 1;
	else if (!bh && sector - session_offset == VSD_FIRST_SECTOR_OFFSET)
		return -1;
	else
		return 0;
}

static int udf_verify_domain_identifier(struct super_block *sb,
					struct regid *ident, char *dname)
{
	struct domainIdentSuffix *suffix;

	if (memcmp(ident->ident, UDF_ID_COMPLIANT, strlen(UDF_ID_COMPLIANT))) {
		udf_warn(sb, "Not OSTA UDF compliant %s descriptor.\n", dname);
		goto force_ro;
	}
	if (ident->flags & ENTITYID_FLAGS_DIRTY) {
		udf_warn(sb, "Possibly not OSTA UDF compliant %s descriptor.\n",
			 dname);
		goto force_ro;
	}
	suffix = (struct domainIdentSuffix *)ident->identSuffix;
	if ((suffix->domainFlags & DOMAIN_FLAGS_HARD_WRITE_PROTECT) ||
	    (suffix->domainFlags & DOMAIN_FLAGS_SOFT_WRITE_PROTECT)) {
		if (!sb_rdonly(sb)) {
			udf_warn(sb, "Descriptor for %s marked write protected."
				 " Forcing read only mount.\n", dname);
		}
		goto force_ro;
	}
	return 0;

force_ro:
	if (!sb_rdonly(sb))
		return -EACCES;
	UDF_SET_FLAG(sb, UDF_FLAG_RW_INCOMPAT);
	return 0;
}

static int udf_load_fileset(struct super_block *sb, struct fileSetDesc *fset,
			    struct kernel_lb_addr *root)
{
	int ret;

	ret = udf_verify_domain_identifier(sb, &fset->domainIdent, "file set");
	if (ret < 0)
		return ret;

	*root = lelb_to_cpu(fset->rootDirectoryICB.extLocation);
	UDF_SB(sb)->s_serial_number = le16_to_cpu(fset->descTag.tagSerialNum);

	udf_debug("Rootdir at block=%u, partition=%u\n",
		  root->logicalBlockNum, root->partitionReferenceNum);
	if (sb->descriptor_output)
		memcpy(sb->descriptor_output, fset, sb->s_blocksize);
	return 0;
}

static int udf_find_fileset(struct super_block *sb,
			    struct kernel_lb_addr *fileset,
			    struct kernel_lb_addr *root)
{
	struct buffer_head *bh;
	uint16_t ident;
	int ret;

	if (fileset->logicalBlockNum == 0xFFFFFFFF &&
	    fileset->partitionReferenceNum == 0xFFFF)
		return -EINVAL;

	bh = udf_read_ptagged(sb, fileset, 0, &ident);
	if (!bh)
		return -EIO;
	if (ident != TAG_IDENT_FSD) {
		brelse(bh);
		return -EINVAL;
	}

	udf_debug("Fileset at block=%u, partition=%u\n",
		  fileset->logicalBlockNum, fileset->partitionReferenceNum);

	UDF_SB(sb)->s_partition = fileset->partitionReferenceNum;
	ret = udf_load_fileset(sb, (struct fileSetDesc *)bh->b_data, root);
	brelse(bh);
	return ret;
}

static bool udf_lvid_valid(struct super_block *sb,
			   struct logicalVolIntegrityDesc *lvid)
{
	u32 parts, impuselen;

	parts = le32_to_cpu(lvid->numOfPartitions);
	impuselen = le32_to_cpu(lvid->lengthOfImpUse);
	if (impuselen < sizeof(struct logicalVolIntegrityDescImpUse) ||
	    parts >= sb->s_blocksize || impuselen >= sb->s_blocksize ||
	    sizeof(struct logicalVolIntegrityDesc) + impuselen +
	    2 * parts * sizeof(u32) > sb->s_blocksize)
		return false;
	return true;
}

static void udf_load_logicalvolint(struct super_block *sb, struct kernel_extent_ad loc)
{
	struct buffer_head *bh, *final_bh;
	uint16_t ident;
	struct udf_sb_info *sbi = UDF_SB(sb);
	struct logicalVolIntegrityDesc *lvid;
	int indirections = 0;
	struct kernel_extent_ad next;

	while (++indirections <= UDF_MAX_LVID_NESTING) {
		final_bh = NULL;
		while (loc.extLength > 0 &&
			(bh = udf_read_tagged(sb, loc.extLocation,
					loc.extLocation, &ident))) {
			if (ident != TAG_IDENT_LVID) {
				brelse(bh);
				break;
			}

			brelse(final_bh);
			final_bh = bh;

			loc.extLength = loc.extLength > sb->s_blocksize ?
				loc.extLength - sb->s_blocksize : 0;
			loc.extLocation++;
		}

		if (!final_bh)
			return;

		lvid = (struct logicalVolIntegrityDesc *)final_bh->b_data;
		next = leea_to_cpu(lvid->nextIntegrityExt);
		if (udf_lvid_valid(sb, lvid)) {
			brelse(sbi->s_lvid_bh);
			sbi->s_lvid_bh = final_bh;
		} else {
			udf_warn(sb, "Corrupted LVID (parts=%u, impuselen=%u), "
				 "ignoring.\n",
				 le32_to_cpu(lvid->numOfPartitions),
				 le32_to_cpu(lvid->lengthOfImpUse));
			brelse(final_bh);
		}

		if (next.extLength == 0)
			return;

		loc = next;
	}

	udf_warn(sb, "Too many LVID indirections (max %u), ignoring.\n",
		UDF_MAX_LVID_NESTING);
	brelse(sbi->s_lvid_bh);
	sbi->s_lvid_bh = NULL;
}

#define PART_DESC_ALLOC_STEP 32

struct part_desc_seq_scan_data {
	struct udf_vds_record rec;
	u32 partnum;
};

struct desc_seq_scan_data {
	struct udf_vds_record vds[VDS_POS_LENGTH];
	unsigned int size_part_descs;
	unsigned int num_part_descs;
	struct part_desc_seq_scan_data *part_descs_loc;
};


static struct udf_vds_record *handle_partition_descriptor(
				struct buffer_head *bh,
				struct desc_seq_scan_data *data)
{
	struct partitionDesc *desc = (struct partitionDesc *)bh->b_data;
	int partnum;
	int i;

	partnum = le16_to_cpu(desc->partitionNumber);
	for (i = 0; i < data->num_part_descs; i++)
		if (partnum == data->part_descs_loc[i].partnum)
			return &(data->part_descs_loc[i].rec);
	if (data->num_part_descs >= data->size_part_descs) {
		struct part_desc_seq_scan_data *new_loc;
		unsigned int new_size;

		new_size = data->num_part_descs + PART_DESC_ALLOC_STEP;
		new_loc = kzalloc_objs(*new_loc, new_size);
		if (!new_loc)
			return ERR_PTR(-ENOMEM);
		memcpy(new_loc, data->part_descs_loc,
		       data->size_part_descs * sizeof(*new_loc));
		kfree(data->part_descs_loc);
		data->part_descs_loc = new_loc;
		data->size_part_descs = new_size;
	}
	data->part_descs_loc[data->num_part_descs].partnum = partnum;
	return &(data->part_descs_loc[data->num_part_descs++].rec);
}

static struct udf_vds_record *get_volume_descriptor_record(uint16_t ident,
		struct buffer_head *bh, struct desc_seq_scan_data *data)
{
	switch (ident) {
	case TAG_IDENT_PVD: /* ISO 13346 3/10.1 */
		return &(data->vds[VDS_POS_PRIMARY_VOL_DESC]);
	case TAG_IDENT_IUVD: /* ISO 13346 3/10.4 */
		return &(data->vds[VDS_POS_IMP_USE_VOL_DESC]);
	case TAG_IDENT_LVD: /* ISO 13346 3/10.6 */
		return &(data->vds[VDS_POS_LOGICAL_VOL_DESC]);
	case TAG_IDENT_USD: /* ISO 13346 3/10.8 */
		return &(data->vds[VDS_POS_UNALLOC_SPACE_DESC]);
	case TAG_IDENT_PD: /* ISO 13346 3/10.5 */
		return handle_partition_descriptor(bh, data);
	}
	return NULL;
}

static noinline int udf_process_sequence(
		struct super_block *sb,
		sector_t block, sector_t lastblock,
		struct kernel_lb_addr *fileset)
{
	struct buffer_head *bh = NULL;
	struct udf_vds_record *curr;
	struct generic_desc *gd;
	struct volDescPtr *vdp;
	bool done = false;
	uint32_t vdsn;
	uint16_t ident;
	int ret;
	unsigned int indirections = 0;
	struct desc_seq_scan_data data;
	unsigned int i;

	memset(data.vds, 0, sizeof(struct udf_vds_record) * VDS_POS_LENGTH);
	data.size_part_descs = PART_DESC_ALLOC_STEP;
	data.num_part_descs = 0;
	data.part_descs_loc = kzalloc_objs(*data.part_descs_loc,
					   data.size_part_descs);
	if (!data.part_descs_loc)
		return -ENOMEM;

	/*
	 * Read the main descriptor sequence and find which descriptors
	 * are in it.
	 */
	for (; (!done && block <= lastblock); block++) {
		bh = udf_read_tagged(sb, block, block, &ident);
		if (!bh)
			break;

		/* Process each descriptor (ISO 13346 3/8.3-8.4) */
		gd = (struct generic_desc *)bh->b_data;
		vdsn = le32_to_cpu(gd->volDescSeqNum);
		switch (ident) {
		case TAG_IDENT_VDP: /* ISO 13346 3/10.3 */
			if (++indirections > UDF_MAX_TD_NESTING) {
				udf_err(sb, "too many Volume Descriptor "
					"Pointers (max %u supported)\n",
					UDF_MAX_TD_NESTING);
				brelse(bh);
				ret = -EIO;
				goto out;
			}

			vdp = (struct volDescPtr *)bh->b_data;
			block = le32_to_cpu(vdp->nextVolDescSeqExt.extLocation);
			lastblock = le32_to_cpu(
				vdp->nextVolDescSeqExt.extLength) >>
				sb->s_blocksize_bits;
			lastblock += block - 1;
			/* For loop is going to increment 'block' again */
			block--;
			break;
		case TAG_IDENT_PVD: /* ISO 13346 3/10.1 */
		case TAG_IDENT_IUVD: /* ISO 13346 3/10.4 */
		case TAG_IDENT_LVD: /* ISO 13346 3/10.6 */
		case TAG_IDENT_USD: /* ISO 13346 3/10.8 */
		case TAG_IDENT_PD: /* ISO 13346 3/10.5 */
			curr = get_volume_descriptor_record(ident, bh, &data);
			if (IS_ERR(curr)) {
				brelse(bh);
				ret = PTR_ERR(curr);
				goto out;
			}
			/* Descriptor we don't care about? */
			if (!curr)
				break;
			if (vdsn >= curr->volDescSeqNum) {
				curr->volDescSeqNum = vdsn;
				curr->block = block;
			}
			break;
		case TAG_IDENT_TD: /* ISO 13346 3/10.9 */
			done = true;
			break;
		}
		brelse(bh);
	}
	/*
	 * Now read interesting descriptors again and process them
	 * in a suitable order
	 */
	if (!data.vds[VDS_POS_PRIMARY_VOL_DESC].block) {
		udf_err(sb, "Primary Volume Descriptor not found!\n");
		ret = -EAGAIN;
		goto out;
	}
	ret = udf_load_pvoldesc(sb, data.vds[VDS_POS_PRIMARY_VOL_DESC].block);
	if (ret < 0)
		goto out;

	if (data.vds[VDS_POS_LOGICAL_VOL_DESC].block) {
		ret = udf_load_logicalvol(sb,
				data.vds[VDS_POS_LOGICAL_VOL_DESC].block,
				fileset);
		if (ret < 0)
			goto out;
	}

	/* Now handle prevailing Partition Descriptors */
	for (i = 0; i < data.num_part_descs; i++) {
		ret = udf_load_partdesc(sb, data.part_descs_loc[i].rec.block);
		if (ret < 0)
			goto out;
	}
	ret = 0;
out:
	kfree(data.part_descs_loc);
	return ret;
}

static int udf_load_sequence(struct super_block *sb, struct buffer_head *bh,
			     struct kernel_lb_addr *fileset)
{
	struct anchorVolDescPtr *anchor;
	sector_t main_s, main_e, reserve_s, reserve_e;
	int ret;

	anchor = (struct anchorVolDescPtr *)bh->b_data;

	/* Locate the main sequence */
	main_s = le32_to_cpu(anchor->mainVolDescSeqExt.extLocation);
	main_e = le32_to_cpu(anchor->mainVolDescSeqExt.extLength);
	main_e = main_e >> sb->s_blocksize_bits;
	main_e += main_s - 1;

	/* Locate the reserve sequence */
	reserve_s = le32_to_cpu(anchor->reserveVolDescSeqExt.extLocation);
	reserve_e = le32_to_cpu(anchor->reserveVolDescSeqExt.extLength);
	reserve_e = reserve_e >> sb->s_blocksize_bits;
	reserve_e += reserve_s - 1;

	/* Process the main & reserve sequences */
	/* responsible for finding the PartitionDesc(s) */
	ret = udf_process_sequence(sb, main_s, main_e, fileset);
	if (ret != -EAGAIN)
		return ret;
	udf_sb_free_partitions(sb);
	ret = udf_process_sequence(sb, reserve_s, reserve_e, fileset);
	if (ret < 0) {
		udf_sb_free_partitions(sb);
		/* No sequence was OK, return -EIO */
		if (ret == -EAGAIN)
			ret = -EIO;
	}
	return ret;
}

static int udf_check_anchor_block(struct super_block *sb, sector_t block,
				  struct kernel_lb_addr *fileset)
{
	struct buffer_head *bh;
	uint16_t ident;
	int ret;

	bh = udf_read_tagged(sb, block, block, &ident);
	if (!bh)
		return -EAGAIN;
	if (ident != TAG_IDENT_AVDP) {
		brelse(bh);
		return -EAGAIN;
	}
	sb->seen_anchor = 1;
	ret = udf_load_sequence(sb, bh, fileset);
	if (!ret)
		sb->anchor_block = (uint32_t)block;
	brelse(bh);
	return ret;
}

static int udf_scan_anchors(struct super_block *sb, udf_pblk_t *lastblock,
			    struct kernel_lb_addr *fileset)
{
	udf_pblk_t last[6];
	int i;
	struct udf_sb_info *sbi = UDF_SB(sb);
	int last_count = 0;
	int ret;

	/* First try user provided anchor */
	if (sbi->s_anchor) {
		ret = udf_check_anchor_block(sb, sbi->s_anchor, fileset);
		if (ret != -EAGAIN)
			return ret;
	}
	/*
	 * according to spec, anchor is in either:
	 *     block 256
	 *     lastblock-256
	 *     lastblock
	 *  however, if the disc isn't closed, it could be 512.
	 */
	ret = udf_check_anchor_block(sb, sbi->s_session + 256, fileset);
	if (ret != -EAGAIN)
		return ret;
	/*
	 * The trouble is which block is the last one. Drives often misreport
	 * this so we try various possibilities.
	 */
	last[last_count++] = *lastblock;
	if (*lastblock >= 1)
		last[last_count++] = *lastblock - 1;
	last[last_count++] = *lastblock + 1;
	if (*lastblock >= 2)
		last[last_count++] = *lastblock - 2;
	if (*lastblock >= 150)
		last[last_count++] = *lastblock - 150;
	if (*lastblock >= 152)
		last[last_count++] = *lastblock - 152;

	for (i = 0; i < last_count; i++) {
		if (last[i] >= sb_bdev_nr_blocks(sb))
			continue;
		ret = udf_check_anchor_block(sb, last[i], fileset);
		if (ret != -EAGAIN) {
			if (!ret)
				*lastblock = last[i];
			return ret;
		}
		if (last[i] < 256)
			continue;
		ret = udf_check_anchor_block(sb, last[i] - 256, fileset);
		if (ret != -EAGAIN) {
			if (!ret)
				*lastblock = last[i];
			return ret;
		}
	}

	/* Finally try block 512 in case media is open */
	return udf_check_anchor_block(sb, sbi->s_session + 512, fileset);
}

static int udf_load_vrs(struct super_block *sb, struct udf_options *uopt,
			int silent, struct kernel_lb_addr *fileset)
{
	struct udf_sb_info *sbi = UDF_SB(sb);
	int nsr = 0;
	int ret;

	if (!sb_set_blocksize(sb, uopt->blocksize)) {
		if (!silent)
			udf_warn(sb, "Bad block size\n");
		return -EINVAL;
	}
	sbi->s_last_block = uopt->lastblock;
	if (!UDF_QUERY_FLAG(sb, UDF_FLAG_NOVRS)) {
		/* Check that it is NSR02 compliant */
		nsr = udf_check_vsd(sb);
		if (!nsr) {
			if (!silent)
				udf_warn(sb, "No VRS found\n");
			return -EINVAL;
		}
		if (nsr == -1)
			udf_debug("Failed to read sector at offset %d. "
				  "Assuming open disc. Skipping validity "
				  "check\n", VSD_FIRST_SECTOR_OFFSET);
		if (!sbi->s_last_block)
			sbi->s_last_block = udf_get_last_block(sb);
	} else {
		udf_debug("Validity check skipped because of novrs option\n");
	}

	/* Look for anchor block and load Volume Descriptor Sequence */
	sbi->s_anchor = uopt->anchor;
	ret = udf_scan_anchors(sb, &sbi->s_last_block, fileset);
	if (ret < 0) {
		if (!silent && ret == -EAGAIN)
			udf_warn(sb, "No anchor found\n");
		return ret;
	}
	return 0;
}

int sevenzip_udf_load_vrs(struct super_block *sb, struct udf_options *options,
                          struct kernel_lb_addr *fileset)
{
    return udf_load_vrs(sb, options, 1, fileset);
}
int sevenzip_udf_check_vsd(struct super_block *sb)
{
    return udf_check_vsd(sb);
}
int sevenzip_udf_find_fileset(struct super_block *sb, struct kernel_lb_addr *fileset,
                              struct kernel_lb_addr *root)
{
    return udf_find_fileset(sb, fileset, root);
}

/* Archive projection replaces VFS publication by the native volume loaders. */
static int udf_load_selected(struct super_block *sb, sector_t block, uint16_t expected,
                             struct kernel_lb_addr *fileset)
{
    struct buffer_head *bh;
    uint16_t ident;
    int result;
    bh = udf_read_tagged(sb, block, (uint32_t)block, &ident);
    if (!bh)
        return -EAGAIN;
    if (ident != expected) {
        brelse(bh);
        return expected == TAG_IDENT_PD ? 0 : -EIO;
    }
    result = sb->descriptor(sb->context, ident, (uint32_t)block, bh->b_data, sb->s_blocksize,
                            &sb->info.s_flags, (uint32_t)sb->info.s_last_block);
    if (!result && ident == TAG_IDENT_LVD) {
        struct logicalVolDesc *lvd = (struct logicalVolDesc *)bh->b_data;
        struct long_ad location;
        memcpy(&location, lvd->logicalVolContentsUse, sizeof(location));
        *fileset = lelb_to_cpu(location.extLocation);
        if (lvd->integritySeqExt.extLength)
            udf_load_logicalvolint(sb, leea_to_cpu(lvd->integritySeqExt));
        if (!UDF_SB(sb)->s_lvid_bh)
            UDF_SET_FLAG(sb, UDF_FLAG_RW_INCOMPAT);
    }
    brelse(bh);
    return result ? -EIO : 0;
}
static int udf_load_pvoldesc(struct super_block *sb, sector_t block)
{
    return udf_load_selected(sb, block, TAG_IDENT_PVD, NULL);
}
static int udf_load_logicalvol(struct super_block *sb, sector_t block, struct kernel_lb_addr *fileset)
{
    return udf_load_selected(sb, block, TAG_IDENT_LVD, fileset);
}
static int udf_load_partdesc(struct super_block *sb, sector_t block)
{
    return udf_load_selected(sb, block, TAG_IDENT_PD, NULL);
}
static void udf_sb_free_partitions(struct super_block *sb)
{
    sb->reset(sb->context);
}

static int udf_sb_alloc_partition_maps(struct super_block *sb, u32 count)
{
	struct udf_sb_info *sbi = UDF_SB(sb);

	sbi->s_partmaps = kzalloc_objs(*sbi->s_partmaps, count);
	if (!sbi->s_partmaps) {
		sbi->s_partitions = 0;
		return -ENOMEM;
	}

	sbi->s_partitions = count;
	return 0;
}

/* udf_load_logicalvol's table parser; tagged reads and volume publication stay
 * with the existing discovery/projection services. Sparing I/O follows PD setup. */
static int udf_decode_partition_maps(struct super_block *sb, struct logicalVolDesc *lvd)
{
    struct udf_sb_info *sbi = UDF_SB(sb);
    struct genericPartitionMap *gpm;
    unsigned int table_len, part_map_count;
    int i, offset, ret;
    uint8_t type;
	table_len = le32_to_cpu(lvd->mapTableLength);
	if (table_len > sb->s_blocksize - sizeof(*lvd)) {
		udf_err(sb, "error loading logical volume descriptor: "
			"Partition table too long (%u > %lu)\n", table_len,
			sb->s_blocksize - sizeof(*lvd));
		ret = -EIO;
		goto out;
	}

	ret = udf_verify_domain_identifier(sb, &lvd->domainIdent,
					   "logical volume");
	if (ret)
		goto out;

	part_map_count = le32_to_cpu(lvd->numPartitionMaps);
	if (part_map_count > table_len / sizeof(struct genericPartitionMap1)) {
		udf_err(sb, "error loading logical volume descriptor: "
			"Too many partition maps (%u > %u)\n", part_map_count,
			table_len / (unsigned)sizeof(struct genericPartitionMap1));
		ret = -EIO;
		goto out;
	}
	ret = udf_sb_alloc_partition_maps(sb, part_map_count);
	if (ret)
		goto out;

	for (i = 0, offset = 0;
	     i < sbi->s_partitions && offset < table_len;
	     i++, offset += gpm->partitionMapLength) {
		struct udf_part_map *map = &sbi->s_partmaps[i];
        unsigned available = sb->s_blocksize - sizeof(*lvd) - offset;
        if (available < 2) {
            ret = -EFSCORRUPTED;
            goto out;
        }
		gpm = (struct genericPartitionMap *)
				&(lvd->partitionMaps[offset]);
		type = gpm->partitionMapType;
        if ((type == 1 && available < sizeof(struct genericPartitionMap1)) ||
            (type == 2 && available < 28)) {
            ret = -EFSCORRUPTED;
            goto out;
        }
        map->s_raw = (const uint8_t *)gpm;
        map->s_raw_len = available < 64 ? available : 64;
		if (type == 1) {
			struct genericPartitionMap1 *gpm1 =
				(struct genericPartitionMap1 *)gpm;
			map->s_partition_type = UDF_TYPE1_MAP15;
			map->s_volumeseqnum = le16_to_cpu(gpm1->volSeqNum);
			map->s_partition_num = le16_to_cpu(gpm1->partitionNum);
			map->s_partition_func = NULL;
		} else if (type == 2) {
			struct udfPartitionMap2 *upm2 =
						(struct udfPartitionMap2 *)gpm;
			if (!strncmp((const char *)upm2->partIdent.ident, UDF_ID_VIRTUAL,
						strlen(UDF_ID_VIRTUAL))) {
                if (available < 40) {
                    ret = -EFSCORRUPTED;
                    goto out;
                }
				u16 suf =
					le16_to_cpu(((__le16 *)upm2->partIdent.
							identSuffix)[0]);
				if (suf < 0x0200) {
					map->s_partition_type =
							UDF_VIRTUAL_MAP15;
					map->s_partition_func =
							udf_get_pblock_virt15;
				} else {
					map->s_partition_type =
							UDF_VIRTUAL_MAP20;
					map->s_partition_func =
							udf_get_pblock_virt20;
				}
			} else if (!strncmp((const char *)upm2->partIdent.ident,
						UDF_ID_SPARABLE,
						strlen(UDF_ID_SPARABLE))) {
                if (available < 48 || ((struct sparablePartitionMap *)gpm)->numSparingTables > 4 ||
                    available < 48U + 4U * ((struct sparablePartitionMap *)gpm)->numSparingTables) {
                    ret = -EFSCORRUPTED;
                    goto out;
                }
				ret = udf_describe_sparable_map(sb, map,
					(struct sparablePartitionMap *)gpm);
				if (ret < 0)
					goto out;
			} else if (!strncmp((const char *)upm2->partIdent.ident,
						UDF_ID_METADATA,
						strlen(UDF_ID_METADATA))) {
                if (available < 59) {
                    ret = -EFSCORRUPTED;
                    goto out;
                }
				struct udf_meta_data *mdata =
					&map->s_type_specific.s_metadata;
				struct metadataPartitionMap *mdm =
						(struct metadataPartitionMap *)
						&(lvd->partitionMaps[offset]);
				udf_debug("Parsing Logical vol part %d type %u  id=%s\n",
					  i, type, UDF_ID_METADATA);

				map->s_partition_type = UDF_METADATA_MAP25;
				map->s_partition_func = udf_get_pblock_meta25;

				mdata->s_meta_file_loc   =
					le32_to_cpu(mdm->metadataFileLoc);
				mdata->s_mirror_file_loc =
					le32_to_cpu(mdm->metadataMirrorFileLoc);
				mdata->s_bitmap_file_loc =
					le32_to_cpu(mdm->metadataBitmapFileLoc);
				mdata->s_alloc_unit_size =
					le32_to_cpu(mdm->allocUnitSize);
				mdata->s_align_unit_size =
					le16_to_cpu(mdm->alignUnitSize);
				if (mdm->flags & 0x01)
					mdata->s_flags |= MF_DUPLICATE_MD;

				udf_debug("Metadata Ident suffix=0x%x\n",
					  le16_to_cpu(*(__le16 *)
						      mdm->partIdent.identSuffix));
				udf_debug("Metadata part num=%u\n",
					  le16_to_cpu(mdm->partitionNum));
				udf_debug("Metadata part alloc unit size=%u\n",
					  le32_to_cpu(mdm->allocUnitSize));
				udf_debug("Metadata file loc=%u\n",
					  le32_to_cpu(mdm->metadataFileLoc));
				udf_debug("Mirror file loc=%u\n",
					  le32_to_cpu(mdm->metadataMirrorFileLoc));
				udf_debug("Bitmap file loc=%u\n",
					  le32_to_cpu(mdm->metadataBitmapFileLoc));
				udf_debug("Flags: %d %u\n",
					  mdata->s_flags, mdm->flags);
			} else {
				udf_debug("Unknown ident: %s\n",
					  upm2->partIdent.ident);
				continue;
			}
			map->s_volumeseqnum = le16_to_cpu(upm2->volSeqNum);
			map->s_partition_num = le16_to_cpu(upm2->partitionNum);
		}
		udf_debug("Partition (%d:%u) type %u on volume %u\n",
			  i, map->s_partition_num, type, map->s_volumeseqnum);
	}

    ret = 0;
out:
    return ret;
}

int sevenzip_udf_maps(const void *data, uint32_t size, struct sevenzip_udf_map **output,
                      uint32_t *count, uint32_t *flags)
{
    struct super_block sb = {0};
    unsigned i;
    int result;
    *output = NULL;
    *count = 0;
    if (!data || size < sizeof(struct logicalVolDesc))
        return -EFSCORRUPTED;
    sb.s_blocksize = size;
    sb.info.s_flags = *flags;
    result = udf_decode_partition_maps(&sb, (struct logicalVolDesc *)data);
    *flags = sb.info.s_flags;
    if (!result) {
        struct sevenzip_udf_map *maps = calloc(sb.info.s_partitions, sizeof(*maps));
        if (!maps) result = -ENOMEM;
        else {
            for (i = 0; i < sb.info.s_partitions; ++i) {
                struct udf_part_map *source = &sb.info.s_partmaps[i];
                maps[i].kind = source->s_partition_type;
                maps[i].volume_sequence = source->s_volumeseqnum;
                maps[i].partition_number = source->s_partition_num;
                if (source->s_partition_type == UDF_METADATA_MAP25)
                    maps[i].metadata_file = source->s_type_specific.s_metadata.s_meta_file_loc;
                if (source->s_raw) {
                    maps[i].type = source->s_raw[0];
                    maps[i].bytes = (uint8_t)source->s_raw_len;
                    memcpy(maps[i].data, source->s_raw, source->s_raw_len);
                }
            }
            *output = maps;
            *count = sb.info.s_partitions;
        }
    }
    free(sb.info.s_partmaps);
    return result;
}
void sevenzip_udf_maps_free(struct sevenzip_udf_map *maps) { free(maps); }

/* The two first-match scans in udf_load_partdesc, without VFS publication. */
void sevenzip_udf_map_indices(const struct sevenzip_udf_map *maps, uint32_t count,
                              uint16_t number, uint32_t *physical, uint32_t *dependent)
{
    unsigned i;
    *physical = *dependent = count;
    for (i = 0; i < count; ++i) {
        if (maps[i].partition_number == number &&
            (maps[i].kind == UDF_TYPE1_MAP15 || maps[i].kind == UDF_SPARABLE_MAP15))
            break;
    }
    if (i >= count) return;
    *physical = i;
    for (i = 0; i < count; ++i) {
        if (maps[i].partition_number == number &&
            (maps[i].kind == UDF_VIRTUAL_MAP15 || maps[i].kind == UDF_VIRTUAL_MAP20 ||
             maps[i].kind == UDF_METADATA_MAP25))
            break;
    }
    *dependent = i;
}

int udf_compute_nr_groups(struct super_block *sb, u32 partition)
{
	struct udf_part_map *map = &UDF_SB(sb)->s_partmaps[partition];
	return DIV_ROUND_UP(map->s_partition_len +
			    (sizeof(struct spaceBitmapDesc) << 3),
			    sb->s_blocksize * 8);
}

static struct udf_bitmap *udf_sb_alloc_bitmap(struct super_block *sb, u32 index)
{
	struct udf_bitmap *bitmap;
	int nr_groups = udf_compute_nr_groups(sb, index);

	bitmap = calloc(1, struct_size(bitmap, s_block_bitmap, nr_groups));
	if (!bitmap)
		return NULL;

	bitmap->s_nr_groups = nr_groups;
	return bitmap;
}

static int check_partition_desc(struct super_block *sb,
				struct partitionDesc *p,
				struct udf_part_map *map)
{
	bool umap, utable, fmap, ftable;
	struct partitionHeaderDesc *phd;

	switch (le32_to_cpu(p->accessType)) {
	case PD_ACCESS_TYPE_READ_ONLY:
	case PD_ACCESS_TYPE_WRITE_ONCE:
	case PD_ACCESS_TYPE_NONE:
		goto force_ro;
	}

	/* No Partition Header Descriptor? */
	if (strcmp((const char *)p->partitionContents.ident, PD_PARTITION_CONTENTS_NSR02) &&
	    strcmp((const char *)p->partitionContents.ident, PD_PARTITION_CONTENTS_NSR03))
		goto force_ro;

	phd = (struct partitionHeaderDesc *)p->partitionContentsUse;
	utable = phd->unallocSpaceTable.extLength;
	umap = phd->unallocSpaceBitmap.extLength;
	ftable = phd->freedSpaceTable.extLength;
	fmap = phd->freedSpaceBitmap.extLength;

	/* No allocation info? */
	if (!utable && !umap && !ftable && !fmap)
		goto force_ro;

	/* We don't support blocks that require erasing before overwrite */
	if (ftable || fmap)
		goto force_ro;
	/* UDF 2.60: 2.3.3 - no mixing of tables & bitmaps, no VAT. */
	if (utable && umap)
		goto force_ro;

	if (map->s_partition_type == UDF_VIRTUAL_MAP15 ||
	    map->s_partition_type == UDF_VIRTUAL_MAP20 ||
	    map->s_partition_type == UDF_METADATA_MAP25)
		goto force_ro;

	return 0;
force_ro:
	if (!sb_rdonly(sb))
		return -EACCES;
	UDF_SET_FLAG(sb, UDF_FLAG_RW_INCOMPAT);
	return 0;
}

static int udf_fill_partdesc_info(struct super_block *sb,
		struct partitionDesc *p, int p_index)
{
	struct udf_part_map *map;
	struct udf_sb_info *sbi = UDF_SB(sb);
	struct partitionHeaderDesc *phd;
	u32 sum;
	int err;

	map = &sbi->s_partmaps[p_index];

	map->s_partition_len = le32_to_cpu(p->partitionLength); /* blocks */
	map->s_partition_root = le32_to_cpu(p->partitionStartingLocation);
	if (udf_add_u32_overflow(map->s_partition_root, map->s_partition_len,
			       &sum)) {
		udf_err(sb, "Partition %d has invalid location %u + %u\n",
			p_index, map->s_partition_root, map->s_partition_len);
		return -EFSCORRUPTED;
	}

	if (p->accessType == cpu_to_le32(PD_ACCESS_TYPE_READ_ONLY))
		map->s_partition_flags |= UDF_PART_FLAG_READ_ONLY;
	if (p->accessType == cpu_to_le32(PD_ACCESS_TYPE_WRITE_ONCE))
		map->s_partition_flags |= UDF_PART_FLAG_WRITE_ONCE;
	if (p->accessType == cpu_to_le32(PD_ACCESS_TYPE_REWRITABLE))
		map->s_partition_flags |= UDF_PART_FLAG_REWRITABLE;
	if (p->accessType == cpu_to_le32(PD_ACCESS_TYPE_OVERWRITABLE))
		map->s_partition_flags |= UDF_PART_FLAG_OVERWRITABLE;

	udf_debug("Partition (%d type %x) starts at physical %u, block length %u\n",
		  p_index, map->s_partition_type,
		  map->s_partition_root, map->s_partition_len);

	err = check_partition_desc(sb, p, map);
	if (err)
		return err;

	/*
	 * Skip loading allocation info it we cannot ever write to the fs.
	 * This is a correctness thing as we may have decided to force ro mount
	 * to avoid allocation info we don't support.
	 */
	if (UDF_QUERY_FLAG(sb, UDF_FLAG_RW_INCOMPAT))
		return 0;

	phd = (struct partitionHeaderDesc *)p->partitionContentsUse;
	if (phd->unallocSpaceTable.extLength) {
		struct kernel_lb_addr loc = {
			.logicalBlockNum = le32_to_cpu(
				phd->unallocSpaceTable.extPosition),
			.partitionReferenceNum = p_index,
		};
		struct inode *inode;

		inode = udf_iget_special(sb, &loc);
		if (IS_ERR(inode)) {
			udf_debug("cannot load unallocSpaceTable (part %d)\n",
				  p_index);
			return PTR_ERR(inode);
		}
		map->s_uspace.s_table = inode;
		map->s_partition_flags |= UDF_PART_FLAG_UNALLOC_TABLE;
		udf_debug("unallocSpaceTable (part %d) @ %llu\n",
			  p_index, map->s_uspace.s_table->i_ino);
	}

	if (phd->unallocSpaceBitmap.extLength) {
		struct udf_bitmap *bitmap = udf_sb_alloc_bitmap(sb, p_index);
		if (!bitmap)
			return -ENOMEM;
		map->s_uspace.s_bitmap = bitmap;
		bitmap->s_extPosition = le32_to_cpu(
				phd->unallocSpaceBitmap.extPosition);
		map->s_partition_flags |= UDF_PART_FLAG_UNALLOC_BITMAP;
		/* Check whether math over bitmap won't overflow. */
		if (udf_add_u32_overflow(map->s_partition_len,
				       sizeof(struct spaceBitmapDesc) << 3,
				       &sum)) {
			udf_err(sb, "Partition %d is too long (%u)\n", p_index,
				map->s_partition_len);
			return -EFSCORRUPTED;
		}
		udf_debug("unallocSpaceBitmap (part %d) @ %u\n",
			  p_index, bitmap->s_extPosition);
	}

	return 0;
}

/* Read-side PD setup; allocation buffers are owned for this validation call.
 * The caller keeps the native inode cache after the temporary file closes. */
int sevenzip_udf_partition(void *context, sevenzip_udf_open_file open_file,
    sevenzip_udf_close_file close_file, uint32_t block_size, uint32_t partitions,
    uint32_t index, uint32_t kind, const void *raw, uint32_t *flags)
{
    struct super_block sb = {0};
    struct partitionDesc descriptor;
    struct udf_part_map *map;
    int result;
    if (index >= partitions || block_size < 512 || block_size > 4096 ||
        !is_power_of_2(block_size))
        return -EINVAL;
    sb.info.s_partmaps = calloc(partitions, sizeof(*map));
    if (!sb.info.s_partmaps)
        return -ENOMEM;
    sb.info.s_partitions = partitions;
    sb.info.s_flags = *flags;
    sb.context = context;
    sb.open_file = open_file;
    sb.close_file = close_file;
    sb.s_blocksize = block_size;
    map = &sb.info.s_partmaps[index];
    map->s_partition_type = kind;
    memcpy(&descriptor, raw, sizeof(descriptor));
    result = udf_fill_partdesc_info(&sb, &descriptor, (int)index);
    *flags = sb.info.s_flags;
    if (map->s_partition_flags & UDF_PART_FLAG_UNALLOC_TABLE)
        iput(map->s_uspace.s_table);
    if (map->s_partition_flags & UDF_PART_FLAG_UNALLOC_BITMAP)
        free(map->s_uspace.s_bitmap);
    if (sb.io_error < -4095)
        result = sb.io_error;
    free(sb.info.s_partmaps);
    return result;
}

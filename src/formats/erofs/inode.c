// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2017-2018 HUAWEI, Inc.
 *             https://www.huawei.com/
 * Copyright (C) 2021, Alibaba Cloud
 */
#include "formats/erofs/internal.h"

static int erofs_fill_symlink(struct inode *inode, void *bptr, unsigned int ofs)
{
	struct erofs_inode *vi = EROFS_I(inode);
	char *link;
	loff_t end;

	ofs += vi->xattr_isize;
	/* check whether the symlink data is small enough to be inlined */
	if (vi->datalayout == EROFS_INODE_FLAT_INLINE &&
	    !check_add_overflow(ofs, inode->i_size, &end) &&
	    end <= i_blocksize(inode)) {
		link = kmemdup_nul((char *)bptr + ofs, inode->i_size, GFP_KERNEL);
		if (!link)
			return -ENOMEM;
		if (unlikely(!inode->i_size || strlen(link) != inode->i_size)) {
			erofs_err(inode->i_sb, "invalid fast symlink size %llu @ nid %llu",
				  inode->i_size | 0ULL, vi->nid);
			kfree(link);
			return -EFSCORRUPTED;
		}
		inode_set_cached_link(inode, link, inode->i_size);
	}
	return 0;
}

int erofs_read_inode(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	erofs_blk_t blkaddr = erofs_blknr(sb, erofs_iloc(inode));
	unsigned int ofs = erofs_blkoff(sb, erofs_iloc(inode));
	bool in_mbox = erofs_inode_in_metabox(inode);
	struct erofs_buf buf = __EROFS_BUF_INITIALIZER;
	struct erofs_sb_info *sbi = EROFS_SB(sb);
	erofs_blk_t addrmask = BIT_ULL(48) - 1;
	struct erofs_inode *vi = EROFS_I(inode);
	struct erofs_inode_extended *die, copied;
	struct erofs_inode_compact *dic;
	unsigned int ifmt;
	void *ptr;
	int err = 0;

	ptr = erofs_read_metabuf(&buf, sb, erofs_pos(sb, blkaddr), in_mbox);
	if (IS_ERR(ptr)) {
		err = PTR_ERR(ptr);
		erofs_err(sb, "failed to read inode meta block (nid: %llu): %d",
			  vi->nid, err);
		goto err_out;
	}

	dic = (void *)((char *)ptr + ofs);
	ifmt = le16_to_cpu(dic->i_format);
	if (ifmt & ~EROFS_I_ALL) {
		erofs_err(sb, "unsupported i_format %u of nid %llu",
			  ifmt, vi->nid);
		err = -EOPNOTSUPP;
		goto err_out;
	}

	vi->datalayout = erofs_inode_datalayout(ifmt);
	if (vi->datalayout >= EROFS_INODE_DATALAYOUT_MAX) {
		erofs_err(sb, "unsupported datalayout %u of nid %llu",
			  vi->datalayout, vi->nid);
		err = -EOPNOTSUPP;
		goto err_out;
	}

	switch (erofs_inode_version(ifmt)) {
	case EROFS_INODE_LAYOUT_EXTENDED:
		vi->inode_isize = sizeof(struct erofs_inode_extended);
		/* check if the extended inode acrosses block boundary */
		if (ofs + vi->inode_isize <= sb->s_blocksize) {
			ofs += vi->inode_isize;
			die = (struct erofs_inode_extended *)dic;
			copied.i_u = die->i_u;
			copied.i_nb = die->i_nb;
		} else {
			const unsigned int gotten = sb->s_blocksize - ofs;

			memcpy(&copied, dic, gotten);
			ptr = erofs_read_metabuf(&buf, sb,
					erofs_pos(sb, blkaddr + 1), in_mbox);
			if (IS_ERR(ptr)) {
				err = PTR_ERR(ptr);
				erofs_err(sb, "failed to read inode payload block (nid: %llu): %d",
					  vi->nid, err);
				goto err_out;
			}
			ofs = vi->inode_isize - gotten;
			memcpy((u8 *)&copied + gotten, ptr, ofs);
			die = &copied;
		}
		vi->xattr_isize = erofs_xattr_ibody_size(die->i_xattr_icount);

		inode->i_mode = le16_to_cpu(die->i_mode);
		i_uid_write(inode, le32_to_cpu(die->i_uid));
		i_gid_write(inode, le32_to_cpu(die->i_gid));
		set_nlink(inode, le32_to_cpu(die->i_nlink));
		inode_set_mtime(inode, le64_to_cpu(die->i_mtime),
				le32_to_cpu(die->i_mtime_nsec));

		inode->i_size = le64_to_cpu(die->i_size);
		break;
	case EROFS_INODE_LAYOUT_COMPACT:
		vi->inode_isize = sizeof(struct erofs_inode_compact);
		ofs += vi->inode_isize;
		vi->xattr_isize = erofs_xattr_ibody_size(dic->i_xattr_icount);

		inode->i_mode = le16_to_cpu(dic->i_mode);
		copied.i_u = dic->i_u;
		i_uid_write(inode, le16_to_cpu(dic->i_uid));
		i_gid_write(inode, le16_to_cpu(dic->i_gid));
		if (!S_ISDIR(inode->i_mode) &&
		    ((ifmt >> EROFS_I_NLINK_1_BIT) & 1)) {
			set_nlink(inode, 1);
			copied.i_nb = dic->i_nb;
		} else {
			set_nlink(inode, le16_to_cpu(dic->i_nb.nlink));
			copied.i_nb.startblk_hi = 0;
			addrmask = BIT_ULL(32) - 1;
		}
		if (sbi->epoch > INT64_MAX - (s64)le32_to_cpu(dic->i_mtime)) {
			err = -EFSCORRUPTED;
			goto err_out;
		}
		inode_set_mtime(inode, sbi->epoch + le32_to_cpu(dic->i_mtime),
				sbi->fixed_nsec);

		inode->i_size = le32_to_cpu(dic->i_size);
		break;
	default:
		erofs_err(sb, "unsupported on-disk inode version %u of nid %llu",
			  erofs_inode_version(ifmt), vi->nid);
		err = -EOPNOTSUPP;
		goto err_out;
	}

	if (unlikely(inode->i_size < 0)) {
		erofs_err(sb, "negative i_size @ nid %llu", vi->nid);
		err = -EFSCORRUPTED;
		goto err_out;
	}

	if (IS_ENABLED(CONFIG_EROFS_FS_POSIX_ACL) &&
	    erofs_inode_has_noacl(inode, ptr, ofs))
		cache_no_acl(inode);

	switch (inode->i_mode & S_IFMT) {
	case S_IFDIR:
		vi->dot_omitted = (ifmt >> EROFS_I_DOT_OMITTED_BIT) & 1;
		fallthrough;
	case S_IFREG:
	case S_IFLNK:
		vi->startblk = le32_to_cpu(copied.i_u.startblk_lo) |
			((u64)le16_to_cpu(copied.i_nb.startblk_hi) << 32);
		if (vi->datalayout == EROFS_INODE_FLAT_PLAIN &&
		    !((vi->startblk ^ EROFS_NULL_ADDR) & addrmask))
			vi->startblk = EROFS_NULL_ADDR;

		if(S_ISLNK(inode->i_mode)) {
			err = erofs_fill_symlink(inode, ptr, ofs);
			if (err)
				goto err_out;
		}
		break;
	case S_IFCHR:
	case S_IFBLK:
		inode->i_rdev = new_decode_dev(le32_to_cpu(copied.i_u.rdev));
		break;
	case S_IFIFO:
	case S_IFSOCK:
		inode->i_rdev = 0;
		break;
	default:
		erofs_err(sb, "bogus i_mode (%o) @ nid %llu", inode->i_mode,
			  vi->nid);
		err = -EFSCORRUPTED;
		goto err_out;
	}

	if (!erofs_inode_is_data_compressed(vi->datalayout)) {
		inode->i_blocks = round_up(inode->i_size, sb->s_blocksize) >> 9;
	} else if (!IS_ENABLED(CONFIG_EROFS_FS_ZIP) || !sbi->available_compr_algs) {
		erofs_err(sb, "compressed inode (nid %llu) is invalid in a plain filesystem",
			  vi->nid);
		err = -EFSCORRUPTED;
		goto err_out;
	} else {
		inode->i_blocks = (le32_to_cpu(copied.i_u.blocks_lo) |
			((u64)le16_to_cpu(copied.i_nb.blocks_hi) << 32)) <<
				  (sb->s_blocksize_bits - 9);
	}

	if (vi->datalayout == EROFS_INODE_CHUNK_BASED) {
		/* fill chunked inode summary info */
		vi->chunkformat = le16_to_cpu(copied.i_u.c.format);
		if (vi->chunkformat & ~EROFS_CHUNK_FORMAT_ALL) {
			erofs_err(sb, "unsupported chunk format %x of nid %llu",
				  vi->chunkformat, vi->nid);
			err = -EOPNOTSUPP;
			goto err_out;
		}
		vi->chunkbits = sb->s_blocksize_bits +
			(vi->chunkformat & EROFS_CHUNK_FORMAT_BLKBITS_MASK);
	}
	inode_set_atime_to_ts(inode,
			      inode_set_ctime_to_ts(inode, inode_get_mtime(inode)));

	inode->i_flags &= ~S_DAX;
	if (test_opt(&sbi->opt, DAX_ALWAYS) && S_ISREG(inode->i_mode) &&
	    (vi->datalayout == EROFS_INODE_FLAT_PLAIN ||
	     vi->datalayout == EROFS_INODE_CHUNK_BASED))
		inode->i_flags |= S_DAX;
err_out:
	erofs_put_metabuf(&buf);
	return err;
}

// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/ext4/inode.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/inode.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  64-bit file support on 64-bit platforms by Jakub Jelinek
 *	(jj@sunsite.ms.mff.cuni.cz)
 *
 *  Assorted race fixes, rewrite of ext4_get_block() by Al Viro, 2000
 */

#include "ext4.h"
#include "xattr.h"

static __u32 ext4_inode_csum(struct inode *inode, struct ext4_inode *raw,
			      struct ext4_inode_info *ei)
{
	__u32 csum;
	__u16 dummy_csum = 0;
	int offset = offsetof(struct ext4_inode, i_checksum_lo);
	unsigned int csum_size = sizeof(dummy_csum);

	csum = ext4_chksum(ei->i_csum_seed, (__u8 *)raw, offset);
	csum = ext4_chksum(csum, (__u8 *)&dummy_csum, csum_size);
	offset += csum_size;
	csum = ext4_chksum(csum, (__u8 *)raw + offset,
			   EXT4_GOOD_OLD_INODE_SIZE - offset);

	if (EXT4_INODE_SIZE(inode->i_sb) > EXT4_GOOD_OLD_INODE_SIZE) {
		offset = offsetof(struct ext4_inode, i_checksum_hi);
		csum = ext4_chksum(csum, (__u8 *)raw + EXT4_GOOD_OLD_INODE_SIZE,
				   offset - EXT4_GOOD_OLD_INODE_SIZE);
		if (EXT4_FITS_IN_INODE(raw, ei, i_checksum_hi)) {
			csum = ext4_chksum(csum, (__u8 *)&dummy_csum,
					   csum_size);
			offset += csum_size;
		}
		csum = ext4_chksum(csum, (__u8 *)raw + offset,
				   EXT4_INODE_SIZE(inode->i_sb) - offset);
	}

	return csum;
}

int ext4_inode_csum_verify(struct inode *inode, struct ext4_inode *raw,
				  struct ext4_inode_info *ei)
{
	__u32 provided, calculated;

	if (EXT4_SB(inode->i_sb)->s_es->s_creator_os !=
	    cpu_to_le32(EXT4_OS_LINUX) ||
	    !ext4_has_feature_metadata_csum(inode->i_sb))
		return 1;

	provided = le16_to_cpu(raw->i_checksum_lo);
	calculated = ext4_inode_csum(inode, raw, ei);
	if (EXT4_INODE_SIZE(inode->i_sb) > EXT4_GOOD_OLD_INODE_SIZE &&
	    EXT4_FITS_IN_INODE(raw, ei, i_checksum_hi))
		provided |= ((__u32)le16_to_cpu(raw->i_checksum_hi)) << 16;
	else
		calculated &= 0xFFFF;

	return provided == calculated;
}

int check_igot_inode(struct inode *inode, ext4_iget_flags flags,
			    const char *function, unsigned int line)
{
	const char *err_str;

	if (flags & EXT4_IGET_EA_INODE) {
		if (!(EXT4_I(inode)->i_flags & EXT4_EA_INODE_FL)) {
			err_str = "missing EA_INODE flag";
			goto error;
		}
		if (ext4_test_inode_state(inode, EXT4_STATE_XATTR) ||
		    EXT4_I(inode)->i_file_acl) {
			err_str = "ea_inode with extended attributes";
			goto error;
		}
	} else {
		if ((EXT4_I(inode)->i_flags & EXT4_EA_INODE_FL)) {
			/*
			 * open_by_handle_at() could provide an old inode number
			 * that has since been reused for an ea_inode; this does
			 * not indicate filesystem corruption
			 */
			if (flags & EXT4_IGET_HANDLE)
				return -ESTALE;
			err_str = "unexpected EA_INODE flag";
			goto error;
		}
	}
	if (is_bad_inode(inode) && !(flags & EXT4_IGET_BAD)) {
		err_str = "unexpected bad inode w/o EXT4_IGET_BAD";
		goto error;
	}
	return 0;

error:
	ext4_error_inode(inode, function, line, 0, "%s", err_str);
	return -EFSCORRUPTED;
}

int ext4_iget_extra_inode(struct inode *inode,
					 struct ext4_inode *raw_inode,
					 struct ext4_inode_info *ei)
{
	__le32 *magic = (__le32 *)((u8 *)raw_inode +
			EXT4_GOOD_OLD_INODE_SIZE + ei->i_extra_isize);

	if (EXT4_INODE_HAS_XATTR_SPACE(inode)  &&
	    *magic == cpu_to_le32(EXT4_XATTR_MAGIC)) {
		int err;

		err = xattr_check_inode(inode, IHDR(inode, raw_inode),
					ITAIL(inode, raw_inode));
		if (err)
			return err;

		ext4_set_inode_state(inode, EXT4_STATE_XATTR);
		err = ext4_find_inline_data_nolock(inode);
		if (!err && ext4_has_inline_data(inode))
			ext4_set_inode_state(inode, EXT4_STATE_MAY_INLINE_DATA);
		return err;
	} else
		EXT4_I(inode)->i_inline_off = 0;
	return 0;
}

int ext4_inode_is_fast_symlink(struct inode *inode)
{
	if (!ext4_has_feature_ea_inode(inode->i_sb)) {
		int ea_blocks = EXT4_I(inode)->i_file_acl ?
				EXT4_CLUSTER_SIZE(inode->i_sb) >> 9 : 0;

		if (ext4_has_inline_data(inode))
			return 0;

		return (S_ISLNK(inode->i_mode) && inode->i_blocks - ea_blocks == 0);
	}
	return S_ISLNK(inode->i_mode) && inode->i_size &&
	       (inode->i_size < EXT4_N_BLOCKS * 4);
}

u64 ext4_inode_blocks(struct ext4_inode *raw_inode,
				  struct ext4_inode_info *ei, struct inode *inode)
{
	u64 i_blocks ;
	struct super_block *sb = inode->i_sb;

	if (ext4_has_feature_huge_file(sb)) {
		/* we are using combined 48 bit field */
		i_blocks = ((u64)le16_to_cpu(raw_inode->i_blocks_high)) << 32 |
					le32_to_cpu(raw_inode->i_blocks_lo);
		if (ext4_test_inode_flag(inode, EXT4_INODE_HUGE_FILE)) {
			/* i_blocks represent file system block size */
			return i_blocks  << (inode->i_blkbits - 9);
		} else {
			return i_blocks;
		}
	} else {
		return le32_to_cpu(raw_inode->i_blocks_lo);
	}
}

/* Read-side __ext4_iget boundary. The archive already owns the decoded inode;
 * VFS allocation/cache/operation binding and orphan recovery are not performed. */
int __ext4_iget(struct inode *inode, ext4_iget_flags flags,
               const char *function, unsigned int line)
{
    struct super_block *sb = inode->i_sb;
    struct ext4_super_block *es = EXT4_SB(sb)->s_es;
    unsigned long ino = inode->i_ino;
    loff_t size;
    int ret;
	if ((!(flags & EXT4_IGET_SPECIAL) && is_special_ino(sb, ino)) ||
	    (ino < EXT4_ROOT_INO) ||
	    (ino > le32_to_cpu(es->s_inodes_count))) {
		if (flags & EXT4_IGET_HANDLE)
			return -ESTALE;
		__ext4_error(sb, function, line, false, EFSCORRUPTED, 0,
			     "inode #%lu: comm %s: iget: illegal inode #",
			     ino, current->comm);
		return -EFSCORRUPTED;
	}

	if (inode->i_nlink == 0) {
		if ((inode->i_mode == 0 || flags & EXT4_IGET_SPECIAL ||
		     !(EXT4_SB(inode->i_sb)->s_mount_state & EXT4_ORPHAN_FS)) &&
		    ino != EXT4_BOOT_LOADER_INO) {
			/* this inode is deleted or unallocated */
			if (flags & EXT4_IGET_SPECIAL) {
				ext4_error_inode(inode, function, line, 0,
						 "iget: special inode unallocated");
				ret = -EFSCORRUPTED;
			} else
				ret = -ESTALE;
			goto bad_inode;
		}
		/* The only unlinked inodes we let through here have
		 * valid i_mode and are being read by the orphan
		 * recovery code: that's fine, we're about to complete
		 * the process of deleting those.
		 * OR it is the EXT4_BOOT_LOADER_INO which is
		 * not initialized on a new filesystem. */
	}
	size = i_size_read(inode);
	if (size < 0 || size > ext4_get_maxbytes(inode)) {
		ext4_error_inode(inode, function, line, 0,
				 "iget: bad i_size value: %lld", size);
		ret = -EFSCORRUPTED;
		goto bad_inode;
	}
    /* Known VFS file kinds retain their disk metadata without binding a
     * platform operation table or creating a device node. */
    if (S_ISREG(inode->i_mode) || S_ISDIR(inode->i_mode) ||
        S_ISLNK(inode->i_mode) || S_ISCHR(inode->i_mode) ||
        S_ISBLK(inode->i_mode) || S_ISFIFO(inode->i_mode) || S_ISSOCK(inode->i_mode)) {
    } else if (ino == EXT4_BOOT_LOADER_INO) {
        make_bad_inode(inode);
    } else {
        ret = -EFSCORRUPTED;
        ext4_error_inode(inode, function, line, 0,
                         "iget: bogus i_mode (%o)", inode->i_mode);
        goto bad_inode;
    }
	if (IS_CASEFOLDED(inode) && !ext4_has_feature_casefold(inode->i_sb)) {
		ext4_error_inode(inode, function, line, 0,
				 "casefold flag without casefold feature");
		ret = -EFSCORRUPTED;
		goto bad_inode;
	}

    return check_igot_inode(inode, flags, function, line);
bad_inode:
    return ret;
}

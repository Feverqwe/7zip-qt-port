// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/ext4/dir.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/dir.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  ext4 directory handling functions
 *
 *  Big-endian to little-endian byte-swapping/bitmaps by
 *        David S. Miller (davem@caip.rutgers.edu), 1995
 *
 * Hash Tree Directory indexing (c) 2001  Daniel Phillips
 *
 */

#include "formats/ext/ext4.h"

static bool is_fake_dir_entry(struct ext4_dir_entry_2 *de)
{
	/* Check if . or .. , or skip if namelen is 0 */
	if ((de->name_len > 0) && (de->name_len <= 2) && (de->name[0] == '.') &&
	    (de->name[1] == '.' || de->name[1] == '\0'))
		return true;
	/* Check if this is a csum entry */
	if (de->file_type == EXT4_FT_DIR_CSUM)
		return true;
	return false;
}

/*
 * Return 0 if the directory entry is OK, and 1 if there is a problem
 *
 * Note: this is the opposite of what ext2 and ext3 historically returned...
 *
 * bh passed here can be an inode block or a dir data block, depending
 * on the inode inline data flag.
 */
int __ext4_check_dir_entry(const char *function, unsigned int line,
			   struct inode *dir, struct file *filp,
			   struct ext4_dir_entry_2 *de,
			   struct buffer_head *bh, char *buf, int size,
			   unsigned int offset)
{
	const char *error_msg = NULL;
	const int rlen = ext4_rec_len_from_disk(de->rec_len,
						dir->i_sb->s_blocksize);
	const int next_offset = ((char *) de - buf) + rlen;
	bool fake = is_fake_dir_entry(de);
	bool has_csum = ext4_has_feature_metadata_csum(dir->i_sb);

	if (unlikely(rlen < ext4_dir_rec_len(1, fake ? NULL : dir)))
		error_msg = "rec_len is smaller than minimal";
	else if (unlikely(rlen % 4 != 0))
		error_msg = "rec_len % 4 != 0";
	else if (unlikely(rlen < ext4_dir_rec_len(de->name_len,
							fake ? NULL : dir)))
		error_msg = "rec_len is too small for name_len";
	else if (unlikely(next_offset > size))
		error_msg = "directory entry overrun";
	else if (unlikely(next_offset > size - ext4_dir_rec_len(1,
						  has_csum ? NULL : dir) &&
			  next_offset != size))
		error_msg = "directory entry too close to block end";
	else if (unlikely(le32_to_cpu(de->inode) >
			le32_to_cpu(EXT4_SB(dir->i_sb)->s_es->s_inodes_count)))
		error_msg = "inode out of bounds";
	else if (unlikely(next_offset == size && de->name_len == 1 &&
			  de->name[0] == '.'))
		error_msg = "'.' directory cannot be the last in data block";
	else
		return 0;

	if (filp)
		ext4_error_file(filp, function, line, bh->b_blocknr,
				"bad entry in directory: %s - offset=%u, "
				"inode=%u, rec_len=%d, size=%d fake=%d",
				error_msg, offset, le32_to_cpu(de->inode),
				rlen, size, fake);
	else
		ext4_error_inode(dir, function, line, bh->b_blocknr,
				"bad entry in directory: %s - offset=%u, "
				"inode=%u, rec_len=%d, size=%d fake=%d",
				 error_msg, offset, le32_to_cpu(de->inode),
				 rlen, size, fake);

	return 1;
}

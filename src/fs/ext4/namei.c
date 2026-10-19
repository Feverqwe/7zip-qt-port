// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/ext4/namei.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/namei.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Big-endian to little-endian byte-swapping/bitmaps by
 *        David S. Miller (davem@caip.rutgers.edu), 1995
 *  Directory entry file type support and forward compatibility hooks
 *	for B-tree directories by Theodore Ts'o (tytso@mit.edu), 1998
 *  Hash Tree Directory indexing (c)
 *	Daniel Phillips, 2001
 *  Hash Tree Directory indexing porting
 *	Christopher Li, 2002
 *  Hash Tree Directory indexing cleanup
 *	Theodore Ts'o, 2002
 */

#include "ext4.h"
struct dx_entry
{
	__le32 hash;
	__le32 block;
};

struct dx_countlimit
{
	__le16 limit;
	__le16 count;
};

struct fake_dirent
{
	__le32 inode;
	__le16 rec_len;
	u8 name_len;
	u8 file_type;
};

struct dx_root
{
	struct fake_dirent dot;
	char dot_name[4];
	struct fake_dirent dotdot;
	char dotdot_name[4];
	struct dx_root_info
	{
		__le32 reserved_zero;
		u8 hash_version;
		u8 info_length; /* 8 */
		u8 indirect_levels;
		u8 unused_flags;
	}
	info;
	struct dx_entry	entries[];
};

struct dx_tail {
	u32 dt_reserved;
	__le32 dt_checksum;	/* crc32c(uuid+inum+dirblock) */
};

#define warn_no_space_for_csum(inode) __warn_no_space_for_csum((inode), __func__, __LINE__)

/* Walk through a dirent block to find a checksum "dirent" at the tail */
static struct ext4_dir_entry_tail *get_dirent_tail(struct inode *inode,
						   struct buffer_head *bh)
{
	struct ext4_dir_entry_tail *t;
	int blocksize = EXT4_BLOCK_SIZE(inode->i_sb);

#ifdef PARANOID
	struct ext4_dir_entry *d, *top;

	d = (struct ext4_dir_entry *)bh->b_data;
	top = (struct ext4_dir_entry *)(bh->b_data +
		(blocksize - sizeof(struct ext4_dir_entry_tail)));
	while (d < top && ext4_rec_len_from_disk(d->rec_len, blocksize))
		d = (struct ext4_dir_entry *)(((char *)d) +
		    ext4_rec_len_from_disk(d->rec_len, blocksize));

	if (d != top)
		return NULL;

	t = (struct ext4_dir_entry_tail *)d;
#else
	t = EXT4_DIRENT_TAIL(bh->b_data, EXT4_BLOCK_SIZE(inode->i_sb));
#endif

	if (t->det_reserved_zero1 ||
	    (ext4_rec_len_from_disk(t->det_rec_len, blocksize) !=
	     sizeof(struct ext4_dir_entry_tail)) ||
	    t->det_reserved_zero2 ||
	    t->det_reserved_ft != EXT4_FT_DIR_CSUM)
		return NULL;

	return t;
}

static __le32 ext4_dirblock_csum(struct inode *inode, void *dirent, int size)
{
	struct ext4_inode_info *ei = EXT4_I(inode);
	__u32 csum;

	csum = ext4_chksum(ei->i_csum_seed, (__u8 *)dirent, size);
	return cpu_to_le32(csum);
}

#define warn_no_space_for_csum(inode)					\
	__warn_no_space_for_csum((inode), __func__, __LINE__)

static void __warn_no_space_for_csum(struct inode *inode, const char *func,
				     unsigned int line)
{
	__ext4_warning_inode(inode, func, line,
		"No space for directory leaf checksum. Please run e2fsck -D.");
}

int ext4_dirblock_csum_verify(struct inode *inode, struct buffer_head *bh)
{
	struct ext4_dir_entry_tail *t;

	if (!ext4_has_feature_metadata_csum(inode->i_sb))
		return 1;

	t = get_dirent_tail(inode, bh);
	if (!t) {
		warn_no_space_for_csum(inode);
		return 0;
	}

	if (t->det_checksum != ext4_dirblock_csum(inode, bh->b_data,
						  (char *)t - bh->b_data))
		return 0;

	return 1;
}

static struct dx_countlimit *get_dx_countlimit(struct inode *inode,
					       struct ext4_dir_entry *dirent,
					       int *offset)
{
	struct ext4_dir_entry *dp;
	struct dx_root_info *root;
	int count_offset;
	int blocksize = EXT4_BLOCK_SIZE(inode->i_sb);
	unsigned int rlen = ext4_rec_len_from_disk(dirent->rec_len, blocksize);

	if (rlen == blocksize)
		count_offset = 8;
	else if (rlen == 12) {
		dp = (struct ext4_dir_entry *)(((char *)dirent) + 12);
		if (ext4_rec_len_from_disk(dp->rec_len, blocksize) != blocksize - 12)
			return NULL;
		root = (struct dx_root_info *)(((char *)dp + 12));
		if (root->reserved_zero ||
		    root->info_length != sizeof(struct dx_root_info))
			return NULL;
		count_offset = 32;
	} else
		return NULL;

	if (offset)
		*offset = count_offset;
	return (struct dx_countlimit *)(((char *)dirent) + count_offset);
}

static __le32 ext4_dx_csum(struct inode *inode, struct ext4_dir_entry *dirent,
			   int count_offset, int count, struct dx_tail *t)
{
	struct ext4_inode_info *ei = EXT4_I(inode);
	__u32 csum;
	int size;
	__u32 dummy_csum = 0;
	int offset = offsetof(struct dx_tail, dt_checksum);

	size = count_offset + (count * sizeof(struct dx_entry));
	csum = ext4_chksum(ei->i_csum_seed, (__u8 *)dirent, size);
	csum = ext4_chksum(csum, (__u8 *)t, offset);
	csum = ext4_chksum(csum, (__u8 *)&dummy_csum, sizeof(dummy_csum));

	return cpu_to_le32(csum);
}

int ext4_dx_csum_verify(struct inode *inode,
			       struct ext4_dir_entry *dirent)
{
	struct dx_countlimit *c;
	struct dx_tail *t;
	int count_offset, limit, count;

	if (!ext4_has_feature_metadata_csum(inode->i_sb))
		return 1;

	c = get_dx_countlimit(inode, dirent, &count_offset);
	if (!c) {
		EXT4_ERROR_INODE(inode, "dir seems corrupt?  Run e2fsck -D.");
		return 0;
	}
	limit = le16_to_cpu(c->limit);
	count = le16_to_cpu(c->count);
	/* The VFS htree parser normally validates this before checksum lookup. */
	if (count == 0 || count > limit)
		return 0;
	if (count_offset + (limit * sizeof(struct dx_entry)) >
	    EXT4_BLOCK_SIZE(inode->i_sb) - sizeof(struct dx_tail)) {
		warn_no_space_for_csum(inode);
		return 0;
	}
	t = (struct dx_tail *)(((struct dx_entry *)c) + limit);

	if (t->dt_checksum != ext4_dx_csum(inode, dirent, count_offset,
					    count, t))
		return 0;
	return 1;
}

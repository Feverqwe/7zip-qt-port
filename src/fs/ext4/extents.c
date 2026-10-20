// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2003-2006, Cluster File Systems, Inc, info@clusterfs.com
 * Written by Alex Tomas <alex@clusterfs.com>
 *
 * Architecture independence:
 *   Copyright (c) 2005, Bull S.A.
 *   Written by Pierre Peiffer <pierre.peiffer@bull.net>
 */

/*
 * Extents support for EXT4
 *
 * TODO:
 *   - ext4*_error() should be used in some situations
 *   - analyze all BUG()/BUG_ON(), use -EIO where appropriate
 *   - smart tree reduction
 */

#include "ext4.h"
#include "ext4_extents.h"

static __le32 ext4_extent_block_csum(struct inode *inode,
				     struct ext4_extent_header *eh)
{
	struct ext4_inode_info *ei = EXT4_I(inode);
	__u32 csum;

	csum = ext4_chksum(ei->i_csum_seed, (__u8 *)eh,
			   EXT4_EXTENT_TAIL_OFFSET(eh));
	return cpu_to_le32(csum);
}

int ext4_extent_block_csum_verify(struct inode *inode,
					 struct ext4_extent_header *eh)
{
	struct ext4_extent_tail *et;

	if (!ext4_has_feature_metadata_csum(inode->i_sb))
		return 1;

	et = find_ext4_extent_tail(eh);
	if (et->et_checksum != ext4_extent_block_csum(inode, eh))
		return 0;
	return 1;
}

static inline int ext4_ext_space_block(struct inode *inode, int check)
{
	int size;

	size = (inode->i_sb->s_blocksize - sizeof(struct ext4_extent_header))
			/ sizeof(struct ext4_extent);
#ifdef AGGRESSIVE_TEST
	if (!check && size > 6)
		size = 6;
#endif
	return size;
}

static inline int ext4_ext_space_block_idx(struct inode *inode, int check)
{
	int size;

	size = (inode->i_sb->s_blocksize - sizeof(struct ext4_extent_header))
			/ sizeof(struct ext4_extent_idx);
#ifdef AGGRESSIVE_TEST
	if (!check && size > 5)
		size = 5;
#endif
	return size;
}

static inline int ext4_ext_space_root(struct inode *inode, int check)
{
	int size;

	size = sizeof(EXT4_I(inode)->i_data);
	size -= sizeof(struct ext4_extent_header);
	size /= sizeof(struct ext4_extent);
#ifdef AGGRESSIVE_TEST
	if (!check && size > 3)
		size = 3;
#endif
	return size;
}

static inline int ext4_ext_space_root_idx(struct inode *inode, int check)
{
	int size;

	size = sizeof(EXT4_I(inode)->i_data);
	size -= sizeof(struct ext4_extent_header);
	size /= sizeof(struct ext4_extent_idx);
#ifdef AGGRESSIVE_TEST
	if (!check && size > 4)
		size = 4;
#endif
	return size;
}

static int
ext4_ext_max_entries(struct inode *inode, int depth)
{
	int max;

	if (depth == ext_depth(inode)) {
		if (depth == 0)
			max = ext4_ext_space_root(inode, 1);
		else
			max = ext4_ext_space_root_idx(inode, 1);
	} else {
		if (depth == 0)
			max = ext4_ext_space_block(inode, 1);
		else
			max = ext4_ext_space_block_idx(inode, 1);
	}

	return max;
}

static int ext4_valid_extent(struct inode *inode, struct ext4_extent *ext)
{
	ext4_fsblk_t block = ext4_ext_pblock(ext);
	int len = ext4_ext_get_actual_len(ext);
	ext4_lblk_t lblock = le32_to_cpu(ext->ee_block);

	/*
	 * We allow neither:
	 *  - zero length
	 *  - overflow/wrap-around
	 */
	if (lblock + len <= lblock)
		return 0;
	return ext4_inode_block_valid(inode, block, len);
}

static int ext4_valid_extent_idx(struct inode *inode,
				struct ext4_extent_idx *ext_idx)
{
	ext4_fsblk_t block = ext4_idx_pblock(ext_idx);

	return ext4_inode_block_valid(inode, block, 1);
}

static int ext4_valid_extent_entries(struct inode *inode,
				     struct ext4_extent_header *eh,
				     ext4_lblk_t lblk, ext4_fsblk_t *pblk,
				     int depth)
{
	unsigned short entries;
	ext4_lblk_t lblock = 0;
	ext4_lblk_t cur = 0;

	if (eh->eh_entries == 0)
		return 1;

	entries = le16_to_cpu(eh->eh_entries);

	if (depth == 0) {
		/* leaf entries */
		struct ext4_extent *ext = EXT_FIRST_EXTENT(eh);

		/*
		 * The logical block in the first entry should equal to
		 * the number in the index block.
		 */
		if (depth != ext_depth(inode) &&
		    lblk != le32_to_cpu(ext->ee_block))
			return 0;
		while (entries) {
			if (!ext4_valid_extent(inode, ext))
				return 0;

			/* Check for overlapping extents */
			lblock = le32_to_cpu(ext->ee_block);
			if (lblock < cur) {
				*pblk = ext4_ext_pblock(ext);
				return 0;
			}
			cur = lblock + ext4_ext_get_actual_len(ext);
			ext++;
			entries--;
		}
	} else {
		struct ext4_extent_idx *ext_idx = EXT_FIRST_INDEX(eh);

		/*
		 * The logical block in the first entry should equal to
		 * the number in the parent index block.
		 */
		if (depth != ext_depth(inode) &&
		    lblk != le32_to_cpu(ext_idx->ei_block))
			return 0;
		while (entries) {
			if (!ext4_valid_extent_idx(inode, ext_idx))
				return 0;

			/* Check for overlapping index extents */
			lblock = le32_to_cpu(ext_idx->ei_block);
			if (lblock < cur) {
				*pblk = ext4_idx_pblock(ext_idx);
				return 0;
			}
			ext_idx++;
			entries--;
			cur = lblock + 1;
		}
	}
	return 1;
}

int __ext4_ext_check(const char *function, unsigned int line,
			    struct inode *inode, struct ext4_extent_header *eh,
			    int depth, ext4_fsblk_t pblk, ext4_lblk_t lblk)
{
	const char *error_msg;
	int max = 0, err = -EFSCORRUPTED;

	if (unlikely(eh->eh_magic != EXT4_EXT_MAGIC)) {
		error_msg = "invalid magic";
		goto corrupted;
	}
	if (unlikely(le16_to_cpu(eh->eh_depth) != depth)) {
		error_msg = "unexpected eh_depth";
		goto corrupted;
	}
	if (unlikely(eh->eh_max == 0)) {
		error_msg = "invalid eh_max";
		goto corrupted;
	}
	max = ext4_ext_max_entries(inode, depth);
	if (unlikely(le16_to_cpu(eh->eh_max) > max)) {
		error_msg = "too large eh_max";
		goto corrupted;
	}
	if (unlikely(le16_to_cpu(eh->eh_entries) > le16_to_cpu(eh->eh_max))) {
		error_msg = "invalid eh_entries";
		goto corrupted;
	}
	if (unlikely((eh->eh_entries == 0) && (depth > 0))) {
		error_msg = "eh_entries is 0 but eh_depth is > 0";
		goto corrupted;
	}
	if (!ext4_valid_extent_entries(inode, eh, lblk, &pblk, depth)) {
		error_msg = "invalid extent entries";
		goto corrupted;
	}
	if (unlikely(depth > 32)) {
		error_msg = "too large eh_depth";
		goto corrupted;
	}
	/* Verify checksum on non-root extent tree nodes */
	if (ext_depth(inode) != depth &&
	    !ext4_extent_block_csum_verify(inode, eh)) {
		error_msg = "extent tree corrupted";
		err = -EFSBADCRC;
		goto corrupted;
	}
	return 0;

corrupted:
	ext4_error_inode_err(inode, function, line, 0, -err,
			     "pblk %llu bad header/extent: %s - magic %x, "
			     "entries %u, max %u(%u), depth %u(%u)",
			     (unsigned long long) pblk, error_msg,
			     le16_to_cpu(eh->eh_magic),
			     le16_to_cpu(eh->eh_entries),
			     le16_to_cpu(eh->eh_max),
			     max, le16_to_cpu(eh->eh_depth), depth);
	return err;
}

#define ext4_ext_check(inode, eh, depth, pblk) \
	__ext4_ext_check(__func__, __LINE__, (inode), (eh), (depth), (pblk), 0)

int ext4_ext_check_inode(struct inode *inode)
{
	return ext4_ext_check(inode, ext_inode_hdr(inode), ext_depth(inode), 0);
}

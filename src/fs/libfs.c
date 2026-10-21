// SPDX-License-Identifier: GPL-2.0-only
/*
 *	fs/libfs.c
 *	Library for filesystems writers.
 */

#include "ext4/ext4.h"
/* Archive streams use 64-bit offsets on every host; no native page cache. */
typedef u64 pgoff_t;
#define PAGE_SHIFT 16
static inline bool ext_check_shl_overflow(u64 value, unsigned shift, u64 *result)
{
	if (shift >= 64 || value > (UINT64_MAX >> shift))
		return true;
	*result = value << shift;
	return false;
}
#define check_shl_overflow(value, shift, result) ext_check_shl_overflow(value, shift, result)

/**
 * generic_check_addressable - Check addressability of file system
 * @blocksize_bits:	log of file system block size
 * @num_blocks:		number of blocks in file system
 *
 * Determine whether a file system with @num_blocks blocks (and a
 * block size of 2**@blocksize_bits) is addressable by the sector_t
 * and page cache of the system.  Return 0 if so and -EFBIG otherwise.
 */
int generic_check_addressable(unsigned blocksize_bits, u64 num_blocks)
{
	u64 last_fs_block = num_blocks - 1;
	u64 last_fs_page, max_bytes;

	if (check_shl_overflow(num_blocks, blocksize_bits, &max_bytes))
		return -EFBIG;

	last_fs_page = (max_bytes >> PAGE_SHIFT) - 1;

	if (unlikely(num_blocks == 0))
		return 0;

	if (blocksize_bits < 9)
		return -EINVAL;

	if ((last_fs_block > (sector_t)(~0ULL) >> (blocksize_bits - 9)) ||
	    (last_fs_page > (pgoff_t)(~0ULL))) {
		return -EFBIG;
	}
	return 0;
}

void kfree_link(void *p)
{
	kfree(p);
}

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 * Selected read-only bitmap helpers; allocation/free extent trees omitted.
 */
#include "formats/ntfs/ntfs_fs.h"
#include "formats/ntfs/ntfsbitmap.h"

/* Fixed 64-bit kernel word service also on LLP64 hosts. */
#define ulong u64
#define BITS_PER_LONG 64
#define BITMAP_LAST_WORD_MASK(nbits) (UINT64_MAX >> (-(nbits) & (BITS_PER_LONG - 1)))
typedef __le64 bitmap_ulong;
#define ul_to_cpu(x) le64_to_cpu(x)
unsigned long __sw_hweight64(uint64_t);
#define hweight_long(word) __sw_hweight64(word)

size_t sevenzip_ntfs_bitmap_size(size_t bits)
{
	return ntfs3_bitmap_size(bits);
}

unsigned int ntfs_bitmap_weight_le(const void *bitmap, int bits)
{
	const ulong *bmp = bitmap;
	unsigned int k, lim = bits / BITS_PER_LONG;
	unsigned int w = 0;

	for (k = 0; k < lim; k++)
		w += hweight_long(bmp[k]);

	if (bits % BITS_PER_LONG) {
		w += hweight_long(ul_to_cpu(((bitmap_ulong *)bitmap)[k]) &
				  BITMAP_LAST_WORD_MASK(bits));
	}

	return w;
}

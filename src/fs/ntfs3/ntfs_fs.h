/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved. */
#ifndef SEVENZIP_NTFS_BITMAP_READ_TYPES_H
#define SEVENZIP_NTFS_BITMAP_READ_TYPES_H
#include "ntfscompat.h"
#define BITS_TO_U64(bits) (((bits) + 63) / 64)

/* NTFS uses quad aligned bitmaps. */
static inline size_t ntfs3_bitmap_size(size_t bits)
{
	return BITS_TO_U64(bits) * sizeof(u64);
}
#endif

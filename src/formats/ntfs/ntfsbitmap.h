// SPDX-License-Identifier: MIT
#ifndef SEVENZIP_NTFS_BITMAP_H
#define SEVENZIP_NTFS_BITMAP_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
size_t sevenzip_ntfs_bitmap_size(size_t bits);
unsigned int ntfs_bitmap_weight_le(const void *bitmap, int bits);
#ifdef __cplusplus
}
#endif
#endif

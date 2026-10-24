/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2008 Oracle. All rights reserved. */
/* Contiguous-buffer services for the Linux Btrfs decompression paths. */
#ifndef SEVENZIP_BTRFS_COMPRESSION_H
#define SEVENZIP_BTRFS_COMPRESSION_H
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define BTRFS_MAX_COMPRESSED (128U << 10)
#define BTRFS_MAX_UNCOMPRESSED (128U << 10)
#ifndef EUCLEAN
#define EUCLEAN 990
#endif
int zlib_decompress(const void *, size_t, void *, size_t, int);
int lzo_decompress(const void *, size_t, void *, size_t, unsigned int, int);
int zstd_decompress(const void *, size_t, void *, size_t, int);
#endif

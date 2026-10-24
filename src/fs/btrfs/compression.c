// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2008 Oracle. All rights reserved.
 * Linux compression_decompress read-side dispatch and buffer boundary. */
#include "compression.h"
#include "../../btrfsreader.h"
#include "fs.h"

int btrfs_decode_native(void *context, unsigned int type, unsigned int sectorsize,
                        int inline_extent, const void *input, size_t srclen, void *output,
                        size_t destlen) {
    (void)context;
    if (!input || !output || !srclen || !destlen || srclen > BTRFS_MAX_COMPRESSED ||
        destlen > BTRFS_MAX_UNCOMPRESSED || sectorsize < 4096 || sectorsize > 65536 ||
        (sectorsize & (sectorsize - 1)) || (inline_extent && destlen > sectorsize))
        return -EINVAL;
    /* Linux btrfs_decompress_bio zero-fills the remainder after a successful
     * decode. The extent's ram_bytes can include the last sector's padding. */
    memset(output, 0, destlen);
    switch (type) {
    case BTRFS_COMPRESS_ZLIB:
        return zlib_decompress(input, srclen, output, destlen, inline_extent);
    case BTRFS_COMPRESS_LZO:
        return lzo_decompress(input, srclen, output, destlen, sectorsize, inline_extent);
    case BTRFS_COMPRESS_ZSTD:
        return zstd_decompress(input, srclen, output, destlen, inline_extent);
    default:
        return -EINVAL;
    }
}

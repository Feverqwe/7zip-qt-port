// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2008 Oracle. All rights reserved.
 * Based on jffs2 zlib code: Copyright 2001-2007 Red Hat, Inc.
 * Created by David Woodhouse <dwmw2@infradead.org>
 * Linux zlib_decompress/decompress_bio with contiguous input/output buffers. */
#include "compression.h"
#include <zlib.h>

int zlib_decompress(const void *input, size_t srclen, void *output, size_t destlen,
                    int inline_extent) {
    const unsigned char *data_in = input;
    z_stream strm = {0};
    int wbits = 15;
    strm.next_in = (Bytef *)data_in;
    strm.avail_in = (unsigned int)srclen;
    strm.next_out = output;
    strm.avail_out = (unsigned int)destlen;
    /* Linux skips Adler32 for the normal non-dictionary zlib wrapper;
     * Btrfs verifies the on-disk sectors before decompression. */
    if (srclen > 2 && !(data_in[1] & 0x20) && ((data_in[0] & 0x0f) == Z_DEFLATED) &&
        !(((data_in[0] << 8) + data_in[1]) % 31)) {
        wbits = -((data_in[0] >> 4) + 8);
        strm.next_in += 2;
        strm.avail_in -= 2;
    }
    int ret = inflateInit2(&strm, wbits);
    if (ret != Z_OK)
        return ret == Z_MEM_ERROR ? -ENOMEM : -EILSEQ;
    ret = inflate(&strm, Z_FINISH);
    const size_t to_copy = strm.total_out;
    inflateEnd(&strm);
    if (ret == Z_MEM_ERROR)
        return -ENOMEM;
    /* The bio path may stop once the requested output is full, before
     * consuming the extent's sector padding or zlib trailer. */
    return ((!inline_extent && ret == Z_STREAM_END) ||
            (to_copy == destlen && (ret == Z_STREAM_END || ret == Z_OK || ret == Z_BUF_ERROR)))
               ? 0
               : -EILSEQ;
}

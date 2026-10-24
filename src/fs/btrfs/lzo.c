// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2008 Oracle. All rights reserved.
 * Linux lzo_decompress and lzo_decompress_bio, with contiguous buffers. */
#include "../../include/linux/lzo.h"
#include "compression.h"
#define LZO_LEN 4

static uint32_t get_unaligned_le32(const void *p) {
    const uint8_t *b = p;
    return (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

int lzo_decompress(const void *input, size_t srclen, void *output, size_t destlen,
                   unsigned int sectorsize, int inline_extent) {
    const uint8_t *data_in = input;
    uint8_t *dest = output;
    if (srclen <= 2 * LZO_LEN)
        return -EUCLEAN;
    const uint32_t len_in = get_unaligned_le32(data_in);
    if (len_in > srclen || len_in < 2 * LZO_LEN ||
        (inline_extent ? len_in != srclen
                       : (((size_t)len_in + sectorsize - 1) & ~((size_t)sectorsize - 1)) < srclen))
        return -EUCLEAN;
    uint8_t *buf = malloc(lzo1x_worst_compress(sectorsize));
    if (!buf)
        return -ENOMEM;
    uint32_t cur_in = LZO_LEN;
    size_t cur_out = 0;
    int ret = 0;
    while (cur_in < len_in) {
        if (len_in - cur_in < LZO_LEN || sectorsize - (cur_in % sectorsize) < LZO_LEN) {
            ret = -EUCLEAN;
            break;
        }
        const uint32_t seg_len = get_unaligned_le32(data_in + cur_in);
        cur_in += LZO_LEN;
        if (!seg_len || seg_len > lzo1x_worst_compress(sectorsize) || seg_len > len_in - cur_in ||
            (inline_extent && (cur_in != 2 * LZO_LEN || seg_len != srclen - 2 * LZO_LEN))) {
            ret = -EUCLEAN;
            break;
        }
        size_t out_len = lzo1x_worst_compress(sectorsize);
        ret = lzo1x_decompress_safe(data_in + cur_in, seg_len, buf, &out_len);
        if (ret != LZO_E_OK || !out_len || out_len > sectorsize) {
            ret = -EILSEQ;
            break;
        }
        const size_t count = out_len < destlen - cur_out ? out_len : destlen - cur_out;
        memcpy(dest + cur_out, buf, count);
        cur_out += count;
        cur_in += seg_len;
        if (cur_out == destlen)
            break;
        const uint32_t sector_bytes_left = sectorsize - (cur_in % sectorsize);
        if (sector_bytes_left < LZO_LEN)
            cur_in += sector_bytes_left;
    }
    free(buf);
    return ret ? ret : (!inline_extent || cur_out == destlen) ? 0 : -EILSEQ;
}

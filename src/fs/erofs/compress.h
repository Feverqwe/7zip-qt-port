/* SPDX-License-Identifier: GPL-2.0-only */
/* Contiguous userspace pages at the Linux EROFS decompression boundary. */
#ifndef SEVENZIP_EROFS_COMPRESS_H
#define SEVENZIP_EROFS_COMPRESS_H
#include "internal.h"
struct z_erofs_decompress_req {
    struct super_block *sb;
    struct page **in, **out;
    unsigned int inpages, outpages;
    unsigned int pageofs_in, pageofs_out;
    unsigned int inputsize, outputsize, alg;
    bool partial_decoding;
};
static inline void memcpy_to_page(struct page *page, unsigned int offset, const void *data,
                                  size_t size) {
    memcpy(page->data + offset, data, size);
}
#endif

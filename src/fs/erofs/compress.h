/* SPDX-License-Identifier: GPL-2.0-only */
/* Contiguous userspace pages at the Linux EROFS decompression boundary. */
#ifndef SEVENZIP_EROFS_COMPRESS_H
#define SEVENZIP_EROFS_COMPRESS_H
#include "internal.h"
struct page { u8 *data; };
struct z_erofs_decompress_req {
    struct super_block *sb;
    struct page **in, **out;
    unsigned int inpages, outpages;
    unsigned int pageofs_in, pageofs_out;
    unsigned int inputsize, outputsize, alg;
    bool partial_decoding;
};
#define PAGE_MASK (~((u64)PAGE_SIZE - 1))
static inline void *kmap_local_page(struct page *page) { return page->data; }
#define kunmap_local(p) ((void)(p))
static inline void memcpy_to_page(struct page *page, unsigned int offset,
                                   const void *data, size_t size) {
    memcpy(page->data + offset, data, size);
}
static inline const void *memchr_inv(const void *data, int value, size_t size) {
    const u8 *p = data;
    while (size--) {
        if (*p != (u8)value) return p;
        ++p;
    }
    return NULL;
}
#endif

/* SPDX-License-Identifier: GPL-2.0-only */
/* Page-mapping services for one bounded process-memory scatterlist segment. */
#ifndef SEVENZIP_SCATTER_READ_H
#define SEVENZIP_SCATTER_READ_H
#include "../crypto/skciphercompat.h"
#define PAGE_SIZE SEVENZIP_CRYPTO_PAGE_SIZE
#define PAGE_SHIFT 12
#define PAGE_ALIGN(size) ALIGN(size, PAGE_SIZE)
/* Archive requests own one linear memory span, without chained/DMA pages. */
static inline struct scatterlist *sg_next(struct scatterlist *sg) {
    (void)sg;
    return NULL;
}
static inline void *kmap_atomic(struct page *page) { return page->data; }
static inline void *kmap(struct page *page) { return page->data; }
static inline void kunmap_atomic(const void *addr) { (void)addr; }
static inline void kunmap(struct page *page) { (void)page; }
static inline bool pagefault_disabled(void) { return true; }
#endif

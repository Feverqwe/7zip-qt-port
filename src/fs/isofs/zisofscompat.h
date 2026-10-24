// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_ZISOFS_COMPAT_H
#define SEVENZIP_ZISOFS_COMPAT_H
#include "../../zisofsreader.h"
#include <errno.h>
#include <zlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#define PAGE_SHIFT 12
#define PAGE_SIZE (1U << PAGE_SHIFT)
#define PAGE_MASK (~(uint64_t)(PAGE_SIZE - 1))
#define GFP_KERNEL 0
#define kzalloc_objs(obj, count) calloc((count), sizeof(obj))
#define kfree(ptr) free(ptr)
#define min_t(type, a, b) ((type)(a) < (type)(b) ? (type)(a) : (type)(b))
#define max_t(type, a, b) ((type)(a) > (type)(b) ? (type)(a) : (type)(b))
#define BUG_ON(condition) ((void)0)
#define WARN_ON(condition) ((void)0)
#define printk(...) ((void)0)
#define KERN_DEBUG ""
#define mutex_lock(lock) ((void)0)
#define mutex_unlock(lock) ((void)0)
#define flush_dcache_page(page) ((void)0)
#define kunmap_local(data) ((void)0)
#define kmap_local_page(page) ((page)->data)
#define SetPageUptodate(page) ((page)->uptodate = 1)
#define page_offset(page) ((page)->index << PAGE_SHIFT)
#define unlock_page(page) ((void)0)
#define put_page(page) ((void)0)
#define folio_unlock(folio) ((void)0)
#define folio_end_read(folio, ok) ((folio)->page->uptodate = !!(ok))
typedef int64_t zisofs_loff_t;
typedef uint64_t zisofs_blkcnt_t;
typedef uint64_t zisofs_pgoff_t;
#define loff_t zisofs_loff_t
#define blkcnt_t zisofs_blkcnt_t
#define pgoff_t zisofs_pgoff_t
typedef uint32_t __le32;
struct page {
    uint64_t index;
    unsigned char *data;
    int uptodate;
};
struct sevenzip_zisofs {
    uint64_t i_size, packed_size, table_end, cache_start;
    unsigned i_blkbits;
    unsigned char i_format_parm[2];
    sevenzip_zisofs_read_fn read;
    void *context;
    z_stream stream;
    struct page cache[32];
    unsigned cache_count;
    unsigned char *cache_data;
    unsigned char sink[PAGE_SIZE];
};
#define inode sevenzip_zisofs
#define ISOFS_I(inode) (inode)
#define ISOFS_BUFFER_BITS(inode) ((inode)->i_blkbits)
#define ISOFS_BUFFER_SIZE(inode) (1U << (inode)->i_blkbits)
/* Linux include/linux/zlib.h bound, retained for accepted packed ranges.
 * Library compressBound() is tighter and would reject existing padded blocks. */
static inline unsigned long zisofs_deflate_bound(unsigned long s) {
    return s + ((s + 7) >> 3) + ((s + 63) >> 6) + 11;
}
struct address_space {
    struct inode *host;
};
struct file {
    struct inode *node;
};
struct folio {
    uint64_t index;
    struct page *page;
};
#define file_inode(file) ((file)->node)
struct buffer_head {
    unsigned char *b_data;
    int valid;
};
static inline uint32_t le32_to_cpu(uint32_t value) {
    const unsigned char *p = (const unsigned char *)&value;
    return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static inline void memzero_page(struct page *page, unsigned offset, unsigned size) {
    memset(page->data + offset, 0, size);
}
static inline struct page *grab_cache_page_nowait(struct address_space *mapping, uint64_t index) {
    struct inode *node = mapping->host;
    if (index < node->cache_start || index - node->cache_start >= node->cache_count)
        return NULL;
    return &node->cache[index - node->cache_start];
}
static inline struct buffer_head *isofs_bread(struct inode *node, uint64_t index) {
    unsigned block_size = ISOFS_BUFFER_SIZE(node);
    uint64_t offset = index << node->i_blkbits;
    size_t size;
    struct buffer_head *bh;
    if (offset >= node->packed_size)
        return NULL;
    bh = (struct buffer_head *)calloc(1, sizeof(*bh));
    if (!bh)
        return NULL;
    bh->b_data = (unsigned char *)calloc(1, block_size);
    if (!bh->b_data) {
        free(bh);
        return NULL;
    }
    size = (size_t)min_t(uint64_t, block_size, node->packed_size - offset);
    bh->valid = node->read(node->context, offset, bh->b_data, size) == 0;
    if (!bh->valid) {
        free(bh->b_data);
        free(bh);
        return NULL;
    }
    return bh;
}
static inline void brelse(struct buffer_head *bh) {
    if (bh) {
        free(bh->b_data);
        free(bh);
    }
}
#define buffer_uptodate(bh) ((bh)->valid)
#define wait_on_buffer(bh) ((void)0)
#define bh_read_batch(count, bhs) ((void)0)
static inline int isofs_get_blocks(struct inode *node, uint64_t first, struct buffer_head **bhs,
                                   unsigned count) {
    unsigned i;
    for (i = 0; i < count; ++i) {
        bhs[i] = isofs_bread(node, first + i);
        if (!bhs[i])
            break;
    }
    return (int)i;
}
#endif

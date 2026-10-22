/* SPDX-License-Identifier: GPL-2.0-only */
/* Read-only userspace services for Linux fs/cramfs/inode.c. */
#ifndef SEVENZIP_CRAMFS_INTERNAL_H
#define SEVENZIP_CRAMFS_INTERNAL_H
#include "../../cramfsreader.h"
#include <stdbool.h>
#include <string.h>
#include <limits.h>
typedef uint32_t u32;
#define PAGE_SHIFT 12
#define PAGE_SIZE (1u << PAGE_SHIFT)
#define CRAMFS_BLK_FLAG_UNCOMPRESSED (1u << 31)
#define CRAMFS_BLK_FLAG_DIRECT_PTR (1u << 30)
#define CRAMFS_BLK_FLAGS (CRAMFS_BLK_FLAG_UNCOMPRESSED | CRAMFS_BLK_FLAG_DIRECT_PTR)
#define CRAMFS_BLK_DIRECT_PTR_SHIFT 2
#define OFFSET(inode) ((inode)->offset)
#define unlikely(x) (x)
#define mutex_lock(...) ((void)0)
#define mutex_unlock(...) ((void)0)
#define flush_dcache_folio(...) ((void)0)
#define kunmap_local(...) ((void)0)
#define pr_err(...) ((void)0)
#define offset_in_page(x) ((x) & (PAGE_SIZE - 1))
struct super_block {
    const struct cramfs_source *source;
    bool error;
    unsigned char zero[2 * PAGE_SIZE];
};
struct inode {
    struct super_block *i_sb;
    u32 i_size, offset;
};
struct address_space {
    struct inode *host;
};
struct folio {
    struct address_space *mapping;
    u32 index;
    unsigned char *data;
    bool success;
};
struct file { int unused; };
static inline void *kmap_local_folio(struct folio *folio, unsigned int offset) {
    return folio->data + offset;
}
static inline void folio_end_read(struct folio *folio, bool success) { folio->success = success; }
const void *cramfs_read(struct super_block *, u32, u32);
u32 cramfs_read_u32(struct super_block *, u32);
uint16_t cramfs_read_u16(struct super_block *, u32);
int cramfs_read_folio(struct file *, struct folio *);
int cramfs_map_inode_block(struct inode *, u32, struct cramfs_extent *);
#endif

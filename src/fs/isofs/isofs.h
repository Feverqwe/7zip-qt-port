// SPDX-License-Identifier: GPL-2.0-only
/* Archive boundary for Linux Rock Ridge readers; no mutable VFS objects. */
#ifndef SEVENZIP_ISOFS_COMPAT_H
#define SEVENZIP_ISOFS_COMPAT_H
#include "../../isoreader.h"
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;
typedef uint8_t __u8;
#ifndef __counted_by
#define __counted_by(member)
#endif
#ifdef _MSC_VER
#define __attribute__(value)
#endif
#include "../../include/uapi/linux/iso_fs.h"
typedef struct iso_directory_record sevenzip_iso_disk_record;
#define inode sevenzip_iso_inode
#define iso_directory_record sevenzip_iso_record
#define GFP_KERNEL 0
#define PAGE_SIZE 4096
#ifdef NAME_MAX
#undef NAME_MAX
#endif
#define NAME_MAX 255
#define likely(x) (x)
#define unlikely(x) (x)
#define printk(...) ((void)0)
#define KERN_NOTICE ""
#define KERN_DEBUG ""
#define KERN_CONT ""
#define KERN_WARNING ""
#define KERN_ERR ""
#define kmalloc(size, flags) malloc(size)
#define kfree(ptr) free(ptr)
#define set_nlink(inode, value) ((inode)->i_nlink = (value))
#define i_uid_write(inode, value) ((inode)->i_uid = (value))
#define i_gid_write(inode, value) ((inode)->i_gid = (value))
#define inode_set_ctime_to_ts(inode, value) ((inode)->ctime = (value))
#define inode_set_mtime_to_ts(inode, value) ((inode)->mtime = (value))
#define inode_set_atime_to_ts(inode, value) ((inode)->atime = (value))
#define inode_get_ctime(inode) ((inode)->ctime)
#define inode_get_mtime(inode) ((inode)->mtime)
#define inode_get_atime(inode) ((inode)->atime)
#define MKDEV(major, minor) (((uint32_t)(major) << 20) | (uint32_t)(minor))
#define ISOFS_I(inode) (inode)
enum isofs_file_format { isofs_file_normal, isofs_file_sparse, isofs_file_compressed };
struct super_block {
    struct sevenzip_iso_state *state;
    int s_rock, s_rock_offset, s_nocompress;
    uint32_t s_nzones, block_size;
    int error;
};
#define ISOFS_SB(sb) (sb)
struct buffer_head {
    unsigned char *b_data;
};
static inline unsigned int isonum_711(const u8 *p) { return *p; }
static inline unsigned int isonum_721(const u8 *p) { return p[0] | (unsigned)p[1] << 8; }
static inline unsigned int isonum_733(const u8 *p) {
    return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static inline struct buffer_head *sb_bread(struct super_block *sb, unsigned long block) {
    struct buffer_head *bh;
    if (block >= sb->s_nzones)
        return NULL;
    bh = (struct buffer_head *)malloc(sizeof(*bh));
    if (!bh)
        return NULL;
    bh->b_data = (unsigned char *)malloc(sb->block_size);
    if (!bh->b_data || sb->state->read(sb->state->context, (uint64_t)block * sb->block_size,
                                       bh->b_data, sb->block_size)) {
        free(bh->b_data);
        free(bh);
        sb->error = -EIO;
        return NULL;
    }
    return bh;
}
static inline void put_bh(struct buffer_head *bh) {
    if (bh) {
        free(bh->b_data);
        free(bh);
    }
}
#define brelse(bh) put_bh(bh)
static inline struct inode *isofs_iget_reloc(struct super_block *sb, unsigned block,
                                             unsigned offset) {
    struct inode *node = (struct inode *)calloc(1, sizeof(*node));
    (void)offset;
    if (!node)
        return NULL;
    if (sb->state->relocate(sb->state->context, block, node)) {
        free(node);
        return NULL;
    }
    node->i_sb = sb;
    return node;
}
#define IS_ERR(ptr) (!(ptr))
#define PTR_ERR(ptr) (-EIO)
#define iput(ptr) free(ptr)
#define ISO_DATE_HIGH_SIERRA (1 << 0)
#define ISO_DATE_LONG_FORM (1 << 1)
struct timespec64 iso_date(u8 *, int);
int64_t mktime64(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int,
                 unsigned int);
#endif

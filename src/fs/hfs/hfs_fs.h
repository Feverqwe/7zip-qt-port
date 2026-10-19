/* SPDX-License-Identifier: GPL-2.0 */
/*
 * linux/fs/hfs/hfs_fs.h, read-side platform adaptation.
 * Copyright (C) 1995-1997 Paul H. Hargrove
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 */
#ifndef SEVENZIP_HFS_FS_H
#define SEVENZIP_HFS_FS_H
#include "../../hfsclassicreader.h"
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef u16 __be16;
typedef u32 __be32;
typedef u64 __be64;
#define __packed
#define struct_group_attr(name, attr, ...) union { struct { __VA_ARGS__ }; struct { __VA_ARGS__ } name; }
#pragma pack(push, 1)
#include "../../../third_party/linux/include/linux/hfs_common.h"
#pragma pack(pop)
#undef struct_group_attr
#define likely(x) (x)
#define GFP_KERNEL 0
#define kzalloc(n, flags) calloc(1, (n))
#define kfree(p) free(p)
#define mutex_lock_nested(lock, subclass) ((void)0)
#define mutex_unlock(lock) ((void)0)
#define pr_debug(...) ((void)0)
#define pr_err(...) ((void)0)
#define ERR_PTR(error) ((void *)(intptr_t)(error))
#define PTR_ERR(p) ((int)(intptr_t)(p))
#define IS_ERR(p) ((uintptr_t)(p) >= (uintptr_t)-4095)
static inline u16 be16_to_cpu(u16 v)
{
    const u8 *p = (const u8 *)&v;
    return (u16)((u16)p[0] << 8 | p[1]);
}
static inline u32 be32_to_cpu(u32 v)
{
    const u8 *p = (const u8 *)&v;
    return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3];
}
static inline u16 cpu_to_be16(u16 v)
{
    const u8 p[2] = {(u8)(v >> 8), (u8)v};
    u16 out;
    memcpy(&out, p, sizeof(out));
    return out;
}
static inline u32 cpu_to_be32(u32 v)
{
    const u8 p[4] = {(u8)(v >> 24), (u8)(v >> 16), (u8)(v >> 8), (u8)v};
    u32 out;
    memcpy(&out, p, sizeof(out));
    return out;
}
struct hfs_btree;
struct hfs_find_data;
struct hfs_inode_info {
    hfs_extent_rec first_extents, cached_extents;
    u32 first_blocks, cached_start, cached_blocks, alloc_blocks;
    int resource;
};
struct super_block;
struct inode {
    struct super_block *i_sb;
    u32 i_ino, i_size, i_mode;
    struct hfs_inode_info info;
};
struct hfs_sb_info {
    struct hfs_mdb mdb;
    u32 alloc_blksz, fs_ablocks, fs_div, fs_start;
    struct hfs_btree *ext_tree, *cat_tree;
    sevenzip_hfs_classic_read_fn read;
    void *context;
    u64 size;
    int error;
};
struct super_block {
    struct hfs_sb_info *s_fs_info;
};
#define HFS_I(inode) (&(inode)->info)
#define HFS_SB(sb) ((sb)->s_fs_info)
#define HFS_IS_RSRC(inode) (HFS_I(inode)->resource)
static inline bool hfs_is_valid_cnid(u32 cnid, u8 type)
{
    if (likely(cnid >= HFS_FIRSTUSER_CNID))
        return true;
    switch (cnid) {
    case HFS_ROOT_CNID:
        return type == HFS_CDR_DIR;
    case HFS_EXT_CNID:
    case HFS_CAT_CNID:
        return type == HFS_CDR_FIL;
    default:
        return false;
    }
}
int hfs_mdb_get(struct super_block *sb);
int hfs_inode_read_fork(struct inode *inode, const struct hfs_extent *ext,
                        __be32 log_size, __be32 phys_size);
int hfs_read_inode(struct inode *inode, const hfs_cat_rec *rec, int resource);
int hfs_get_block(struct inode *inode, u32 ablock, u32 *dblock);
int hfs_read_fork(struct inode *inode, u64 offset, void *data, size_t size);
int hfs_read_bytes(struct super_block *sb, u64 offset, void *data, size_t size);
int hfs_strcmp(const unsigned char *, unsigned, const unsigned char *, unsigned);
int hfs_mac2asc(u8 *out, const struct hfs_name *in);
int hfs_cat_keycmp(const btree_key *, const btree_key *);
int hfs_cat_find_brec(struct super_block *, u32, struct hfs_find_data *);
int hfs_ext_keycmp(const btree_key *, const btree_key *);
int hfs_ext_read_extent(struct inode *inode, u16 block);
u16 hfs_ext_find_block(struct hfs_extent *ext, u16 off);
#endif

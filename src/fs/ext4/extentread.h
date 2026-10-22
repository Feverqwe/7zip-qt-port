/* SPDX-License-Identifier: GPL-2.0-only */
/* Archive-owned buffers and export services for Linux's extent-tree walk. */
#ifndef SEVENZIP_EXT4_EXTENT_READ_H
#define SEVENZIP_EXT4_EXTENT_READ_H
#include "../../extreaderprivate.h"
#include "blockread.h"
typedef unsigned int gfp_t;
#define __GFP_MOVABLE 0
#define __GFP_NOFAIL 0
#define EXT4_EX_NOCACHE 0x40000000
#define EXT4_EX_FORCE_CACHE 0x20000000
#define EXT4_EX_NOFAIL 0x10000000
#define EXTENT_STATUS_WRITTEN 1
#define EXTENT_STATUS_UNWRITTEN 2
#define EXTENT_STATUS_HOLE 8
#define EXT4_STATE_EXT_PRECACHED 7
#define KUNIT_STATIC_STUB_REDIRECT(...) ((void)0)
#define trace_ext4_ext_load_extent(...) ((void)0)
#define ext4_check_map_extents_env(...) ((void)0)
#define kzalloc_objs(type, count, flags) calloc((count), sizeof(type))
#define IS_ERR_OR_NULL(pointer) (!(pointer) || IS_ERR(pointer))
struct ext_extent_owner {
    struct super_block sb;
    struct inode inode;
    const struct ext_extent_services *services;
    u64 leaf_limit;
    int error;
};
struct ext_extent_buffer {
    struct buffer_head bh;
    struct super_block *sb;
};
static struct buffer_head *sb_getblk_gfp(struct super_block *sb, u64 physical, gfp_t flags)
{
    struct ext_extent_buffer *buffer = calloc(1, sizeof(*buffer) + sb->s_blocksize);
    (void)flags;
    if (!buffer) return NULL;
    buffer->sb = sb;
    buffer->bh.b_data = (char *)(buffer + 1);
    buffer->bh.b_size = sb->s_blocksize;
    buffer->bh.b_blocknr = physical;
    buffer->bh.allocated = true;
    return &buffer->bh;
}
static int bh_uptodate_or_lock(struct buffer_head *bh) { (void)bh; return 0; }
static int ext4_read_bh(struct buffer_head *bh, int flags, void *end, bool wait)
{
    struct ext_extent_owner *owner = (struct ext_extent_owner *)((struct ext_extent_buffer *)bh)->sb;
    (void)flags; (void)end; (void)wait;
    if (!ext4_inode_block_valid(&owner->inode, bh->b_blocknr, 1)) return -EFSCORRUPTED;
    return owner->services->read(owner->services->context, bh->b_blocknr, bh->b_data, bh->b_size);
}
#define put_bh(bh) brelse(bh)
static void ext4_es_cache_extent(struct inode *inode, u32 logical, u32 count,
                                 u64 physical, unsigned status)
{
    struct ext_extent_owner *owner = (struct ext_extent_owner *)inode->i_sb;
    if (owner->error || status == EXTENT_STATUS_HOLE) return;
    /* Retain the archive's parent boundary check in addition to Linux's
     * per-block validation; no hole is published before the next valid range. */
    if ((u64)logical + count > owner->leaf_limit) {
        owner->error = -EFSCORRUPTED;
        return;
    }
    owner->error = owner->services->emit(owner->services->context, logical, physical, count,
                                        status == EXTENT_STATUS_WRITTEN);
}
#endif

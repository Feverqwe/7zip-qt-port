/* SPDX-License-Identifier: GPL-2.0-only */
/* Synchronous immutable-image buffers for the original indirect lookup. */
#ifndef SEVENZIP_EXT4_INDIRECT_READ_H
#define SEVENZIP_EXT4_INDIRECT_READ_H
#include "../../extreaderprivate.h"
#include "blockread.h"
typedef void handle_t;
#define EXT4_ADDR_PER_BLOCK(sb) ((sb)->s_blocksize / sizeof(__le32))
#define EXT4_ADDR_PER_BLOCK_BITS(sb) ((sb)->s_blocksize_bits - 2)
#define EXT4_IND_BLOCK EXT4_NDIR_BLOCKS
#define EXT4_DIND_BLOCK (EXT4_IND_BLOCK + 1)
#define EXT4_TIND_BLOCK (EXT4_DIND_BLOCK + 1)
#define EXT4_GET_BLOCKS_CREATE 0x0001
/* Kernel buffer-state bits are unnecessary for this private lookup result. */
#define EXT4_MAP_MAPPED 1
#define EXT4_MAP_BOUNDARY 2
#define umin(a, b) ((u64)(a) < (u64)(b) ? (a) : (b))
#define trace_ext4_ind_map_blocks_enter(...) ((void)0)
#define trace_ext4_ind_map_blocks_exit(...) ((void)0)
#define BUFFER_TRACE(...) ((void)0)
#define ASSERT(condition) do { if (!(condition)) return -EFSCORRUPTED; } while (0)
#define ext4_warning(...) ((void)0)
struct ext_indirect_buffer;
struct ext_indirect {
    struct super_block sb;
    struct ext_indirect_services services;
    struct inode inode;
    journal_t journal;
    struct ext_indirect_buffer *buffers[3];
};
struct ext_indirect_buffer {
    struct buffer_head bh;
    struct super_block *sb;
    unsigned refs;
    bool validated;
};
static struct buffer_head *sb_getblk(struct super_block *sb, u32 block)
{
    struct ext_indirect *owner = (struct ext_indirect *)sb;
    struct ext_indirect_buffer *buffer;
    unsigned i, slot = 3;
    for (i = 0; i < 3; ++i) {
        buffer = owner->buffers[i];
        if (buffer && buffer->bh.b_blocknr == block) {
            ++buffer->refs;
            return &buffer->bh;
        }
        if (!buffer || !buffer->refs) slot = i;
    }
    if (slot == 3) return NULL;
    buffer = owner->buffers[slot];
    if (!buffer) {
        buffer = calloc(1, sizeof(*buffer) + sb->s_blocksize);
        if (!buffer) return NULL;
        owner->buffers[slot] = buffer;
    }
    buffer->sb = sb;
    buffer->bh.b_data = (char *)(buffer + 1);
    buffer->bh.b_size = sb->s_blocksize;
    buffer->bh.b_blocknr = block;
    buffer->refs = 1;
    buffer->validated = false;
    return &buffer->bh;
}
static int bh_uptodate_or_lock(struct buffer_head *bh)
{
    return ((struct ext_indirect_buffer *)bh)->validated;
}
static int ext4_check_indirect_blockref(struct inode *inode, struct buffer_head *bh)
{
    int error = ext4_check_blockref(__func__, __LINE__, inode, (__le32 *)bh->b_data,
                                    EXT4_ADDR_PER_BLOCK(inode->i_sb));
    if (!error) ((struct ext_indirect_buffer *)bh)->validated = true;
    return error;
}
static void ext_indirect_release(struct buffer_head *bh)
{
    if (bh) --((struct ext_indirect_buffer *)bh)->refs;
}
#define brelse(bh) ext_indirect_release(bh)
static int ext4_read_bh(struct buffer_head *bh, int flags, void *end, bool wait)
{
    struct ext_indirect_buffer *buffer = (struct ext_indirect_buffer *)bh;
    const struct ext_indirect_services *services =
        &((struct ext_indirect *)buffer->sb)->services;
    (void)flags; (void)end; (void)wait;
    if (bh->b_blocknr >= ext4_blocks_count(EXT4_SB(buffer->sb)->s_es)) return -EIO;
    return services->read(services->context, bh->b_blocknr, bh->b_data, bh->b_size);
}
#define put_bh(bh) brelse(bh)
#endif

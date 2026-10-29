/* SPDX-License-Identifier: GPL-2.0-only */
/* Eager archive services for the original ext4 xattr read bodies. */
#ifndef SEVENZIP_EXT4_XATTR_READ_H
#define SEVENZIP_EXT4_XATTR_READ_H
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define kmalloc_objs(type, count, flags) calloc((count), sizeof(type))
#define ERR_PTR(error) ((void *)(intptr_t)(error))
#define PTR_ERR(pointer) ((int)(intptr_t)(pointer))
#define IS_ERR(pointer) ((uintptr_t)(pointer) >= (uintptr_t)-4095)
#define EA_INODE_CACHE(inode) NULL
#define EA_BLOCK_CACHE(inode) NULL
#define ext4_xattr_block_cache_insert(cache, bh) ((void)0)
#define mb_cache_entry_create(...) ((void)0)
#define ext4_xattr_inode_set_class(inode) ((void)0)
#define inode_lock_nested(inode, subclass) ((void)0)
#define inode_unlock(inode) ((void)0)
#define I_MUTEX_XATTR 0
#define S_NOQUOTA 0
#define inode_get_atime_sec(inode) ((inode)->i_atime_sec)
#define inode_get_mtime_sec(inode) ((inode)->i_mtime_sec)
#define inode_get_ctime_sec(inode) ((inode)->i_ctime_sec)
#define inode_peek_iversion_raw(inode) ((inode)->i_version)
#define inode_set_ctime(inode, seconds, nanos) ((inode)->i_ctime_sec = (seconds))
#define inode_set_iversion_raw(inode, value) ((inode)->i_version = (u32)(value))
#define i_size_read(inode) ((inode)->i_size)
#define ext4_forced_shutdown(sb) false
#define ext4_error(...) ((void)0)
#define ext4_warning_inode(...) ((void)0)
#define pr_warn_once(...) ((void)0)
#define ea_idebug(...) ((void)0)
#define ea_bdebug(...) ((void)0)
#define REQ_PRIO 0

struct mb_cache;
struct inode *ext_xattr_iget(struct super_block *, unsigned long, ext4_iget_flags);
#define ext4_iget(sb, ino, flags) ext_xattr_iget(sb, ino, flags)
void ext4_put_ea_inode(struct inode *);

static struct buffer_head *ext_xattr_alloc_block(struct super_block *sb, u64 number) {
    struct buffer_head *bh = calloc(1, sizeof(*bh) + sb->s_blocksize);
    if (bh) {
        bh->b_data = (char *)(bh + 1);
        bh->b_size = sb->s_blocksize;
        bh->b_blocknr = number;
        bh->allocated = true;
    }
    return bh;
}

static struct buffer_head *ext4_sb_bread(struct super_block *sb, u64 block, int flags) {
    const struct ext_xattr_services *services = EXT4_SB(sb)->xattr_services;
    struct buffer_head *bh;
    int result;
    (void)flags;
    if (!services || !services->read || block > (UINT64_MAX >> sb->s_blocksize_bits))
        return ERR_PTR(-EIO);
    bh = ext_xattr_alloc_block(sb, block);
    if (!bh)
        return ERR_PTR(-ENOMEM);
    result =
        services->read(services->context, 0, block << sb->s_blocksize_bits, bh->b_data, bh->b_size);
    if (result) {
        brelse(bh);
        return ERR_PTR(result);
    }
    return bh;
}

static int ext4_bread_batch(struct inode *inode, u64 first, unsigned count, bool wait,
                            struct buffer_head **blocks) {
    const struct ext_xattr_services *services = EXT4_SB(inode->i_sb)->xattr_services;
    unsigned i;
    int result = -EIO;
    (void)wait;
    if (!services || !services->read)
        return -EIO;
    memset(blocks, 0, count * sizeof(*blocks));
    for (i = 0; i < count; ++i) {
        u64 offset = (first + i) << inode->i_blkbits;
        size_t bytes = inode->i_sb->s_blocksize;
        if (offset >= inode->i_size)
            goto fail;
        if (bytes > inode->i_size - offset)
            bytes = (size_t)(inode->i_size - offset);
        blocks[i] = ext_xattr_alloc_block(inode->i_sb, first + i);
        if (!blocks[i]) {
            result = -ENOMEM;
            goto fail;
        }
        result = services->read(services->context, inode->i_ino, offset, blocks[i]->b_data, bytes);
        if (result)
            goto fail;
    }
    return 0;
fail:
    for (i = 0; i < count; ++i)
        brelse(blocks[i]);
    return result ? result : -EIO;
}
#endif

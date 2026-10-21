// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/ext4/symlink.c
 *
 * Only fast symlinks left here - the rest is done by generic code. AV, 1999
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/symlink.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  ext4 symlink handling code
 */

#include "../../extreaderprivate.h"
#include "../../include/linux/delayed_call.h"
#include "../../include/linux/namei.h"
#include "blockread.h"
struct dentry;
#define IS_ERR_OR_NULL(pointer) (!(pointer) || IS_ERR(pointer))
#define ERR_CAST(pointer) ((void *)(pointer))
#define EXT4_GET_BLOCKS_CACHED_NOWAIT 0
#define ext4_getblk(handle, inode, block, flags) NULL
#define ext4_buffer_uptodate(bh) false
static struct buffer_head *ext4_bread(void *handle, struct inode *inode, unsigned block, int flags)
{
    struct ext_node *node = (struct ext_node *)inode;
    struct buffer_head *bh;
    int error;
    (void)handle; (void)flags;
    if (block || !node->link_services || !node->link_services->read_block)
        return ERR_PTR(-EIO);
    bh = calloc(1, sizeof(*bh) + inode->i_sb->s_blocksize);
    if (!bh) return ERR_PTR(-ENOMEM);
    bh->b_data = (char *)(bh + 1);
    bh->b_size = inode->i_sb->s_blocksize;
    bh->allocated = true;
    error = node->link_services->read_block(node->link_services->context, bh->b_data, bh->b_size);
    if (error) { brelse(bh); return ERR_PTR(error); }
    return bh;
}

static void ext4_free_link(void *bh)
{
	brelse(bh);
}

static const char *ext4_get_link(struct dentry *dentry, struct inode *inode,
				 struct delayed_call *callback)
{
	struct buffer_head *bh;
	char *inline_link;

	/*
	 * Create a new inlined symlink is not supported, just provide a
	 * method to read the leftovers.
	 */
	if (ext4_has_inline_data(inode)) {
		if (!dentry)
			return ERR_PTR(-ECHILD);

		inline_link = ext4_read_inline_link(inode);
		if (!IS_ERR(inline_link))
			set_delayed_call(callback, kfree_link, inline_link);
		return inline_link;
	}

	if (!dentry) {
		bh = ext4_getblk(NULL, inode, 0, EXT4_GET_BLOCKS_CACHED_NOWAIT);
		if (IS_ERR_OR_NULL(bh))
			return ERR_PTR(-ECHILD);
		if (!ext4_buffer_uptodate(bh)) {
			brelse(bh);
			return ERR_PTR(-ECHILD);
		}
	} else {
		bh = ext4_bread(NULL, inode, 0, 0);
		if (IS_ERR(bh))
			return ERR_CAST(bh);
		if (!bh) {
			EXT4_ERROR_INODE(inode, "bad symlink.");
			return ERR_PTR(-EFSCORRUPTED);
		}
	}

	set_delayed_call(callback, ext4_free_link, bh);
	nd_terminate_link(bh->b_data, inode->i_size,
			  inode->i_sb->s_blocksize - 1);
	return bh->b_data;
}


int ext_linux_readlink(struct ext_node *node, const struct ext_link_services *services,
                        void *output, size_t capacity, size_t *size)
{
    struct inode *inode = &node->inode;
    struct delayed_call done = {0};
    const char *target;
    size_t length;
    int error = 0;
    *size = 0;
    if (!S_ISLNK(inode->i_mode)) return -EINVAL;
    if (inode->flags & 0x800U) return -EINVAL;
    node->link_services = services;
    if (ext4_inode_is_fast_symlink(inode))
        target = (const char *)EXT4_I(inode)->i_data;
    else
        target = ext4_get_link((struct dentry *)node, inode, &done);
    node->link_services = NULL;
    if (IS_ERR(target)) return PTR_ERR(target);
    length = strlen(target);
    if (length > capacity) error = -ERANGE;
    else { if (length) memcpy(output, target, length); *size = length; }
    do_delayed_call(&done);
    return error;
}

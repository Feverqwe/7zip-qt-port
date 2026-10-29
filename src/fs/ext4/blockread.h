/* SPDX-License-Identifier: GPL-2.0-only */
/* Immutable archive services for the original system-zone reader. */
#ifndef SEVENZIP_EXT4_BLOCK_READ_H
#define SEVENZIP_EXT4_BLOCK_READ_H
#include "../../include/linux/rbtree.h"
#include "ext4.h"
#define kzalloc_obj(type) calloc(1, sizeof(type))
#define cond_resched() ((void)0)
#define rcu_read_lock() ((void)0)
#define rcu_read_unlock() ((void)0)
#define rcu_dereference(x) (x)
#define rcu_dereference_protected(x, condition) (x)
#define call_rcu(head, callback) callback(head)
#define ERR_PTR(error) ((void *)(intptr_t)(error))
#define PTR_ERR(pointer) ((int)(intptr_t)(pointer))
#define IS_ERR(pointer) ((uintptr_t)(pointer) >= (uintptr_t)-4095)
#define EXT4_ERROR_INODE_ERR(inode, code, ...) ((inode)->error = -(code))
struct rcu_head {
    int unused;
};
struct ext4_system_blocks {
    struct rb_root root;
    struct rcu_head rcu;
};
struct ext4_map_blocks {
    ext4_fsblk_t m_pblk;
    ext4_lblk_t m_lblk;
    unsigned int m_len;
    unsigned int m_flags;
    u64 m_seq;
};
struct inode *ext4_iget(struct super_block *, unsigned long, ext4_iget_flags);
void iput(struct inode *);
int ext4_map_blocks(void *, struct inode *, struct ext4_map_blocks *, int);
#endif

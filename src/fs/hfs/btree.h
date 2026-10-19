/* SPDX-License-Identifier: GPL-2.0 */
/*
 * linux/fs/hfs/btree.h: read structures without VFS caches/locks.
 * Copyright (C) 2001 Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 */
#ifndef SEVENZIP_HFS_BTREE_H
#define SEVENZIP_HFS_BTREE_H
#include "hfs_fs.h"
typedef int (*btree_keycmp)(const btree_key *, const btree_key *);
enum hfs_btree_mutex_classes { CATALOG_BTREE_MUTEX, EXTENTS_BTREE_MUTEX, ATTR_BTREE_MUTEX };
struct hfs_btree {
    struct super_block *sb;
    struct inode inode;
    btree_keycmp keycmp;
    u32 cnid, root, leaf_count, leaf_head, leaf_tail, node_count, free_nodes, attributes;
    unsigned node_size, max_key_len, depth;
    int tree_lock;
};
struct hfs_bnode {
    struct hfs_btree *tree;
    u32 prev, this, next, parent;
    u16 num_recs;
    u8 type, height;
    u8 *data;
};
struct hfs_find_data {
    btree_key *key, *search_key;
    struct hfs_btree *tree;
    struct hfs_bnode *bnode;
    int record;
    int keyoffset, keylength, entryoffset, entrylength;
};
struct hfs_btree *hfs_btree_open(struct super_block *, u32, btree_keycmp);
void hfs_btree_close(struct hfs_btree *);
struct hfs_bnode *hfs_bnode_find(struct hfs_btree *, u32);
void hfs_bnode_put(struct hfs_bnode *);
void hfs_bnode_read(struct hfs_bnode *, void *, u32, u32);
u16 hfs_bnode_read_u16(struct hfs_bnode *, u32);
u8 hfs_bnode_read_u8(struct hfs_bnode *, u32);
u16 hfs_brec_lenoff(struct hfs_bnode *, u16, u16 *);
u16 hfs_brec_keylen(struct hfs_bnode *, u16);
int hfs_find_init(struct hfs_btree *, struct hfs_find_data *);
void hfs_find_exit(struct hfs_find_data *);
int __hfs_brec_find(struct hfs_bnode *, struct hfs_find_data *);
int hfs_brec_find(struct hfs_find_data *);
int hfs_brec_read(struct hfs_find_data *, void *, u32);
int hfs_brec_goto(struct hfs_find_data *, int);
#endif

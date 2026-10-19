// SPDX-License-Identifier: GPL-2.0
/*
 * linux/fs/hfs/btree.c
 * Copyright (C) 2001 Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 * hfs_btree_open/close: byte I/O replaces folio and inode cache services.
 */
#include "btree.h"

struct hfs_btree *hfs_btree_open(struct super_block *sb, u32 id, btree_keycmp keycmp)
{
    struct hfs_btree *tree = calloc(1, sizeof(*tree));
    struct hfs_btree_header_rec head;
    const struct hfs_mdb *mdb = &HFS_SB(sb)->mdb;
    struct hfs_bnode *node;
    u8 header[512];
    if (!tree) {
        HFS_SB(sb)->error = -ENOMEM;
        return NULL;
    }
    tree->sb = sb;
    tree->cnid = id;
    tree->keycmp = keycmp;
    tree->inode.i_sb = sb;
    tree->inode.i_ino = id;
    switch (id) {
    case HFS_EXT_CNID:
        if (hfs_inode_read_fork(&tree->inode, mdb->drXTExtRec, mdb->drXTFlSize, mdb->drXTFlSize) ||
            HFS_I(&tree->inode)->alloc_blocks > HFS_I(&tree->inode)->first_blocks)
            goto free_tree;
        break;
    case HFS_CAT_CNID:
        if (hfs_inode_read_fork(&tree->inode, mdb->drCTExtRec, mdb->drCTFlSize, mdb->drCTFlSize) ||
            !HFS_I(&tree->inode)->first_blocks)
            goto free_tree;
        break;
    default:
        goto free_tree;
    }
    if (hfs_read_fork(&tree->inode, 0, header, sizeof(header)))
        goto free_tree;
    memcpy(&head, header + sizeof(struct hfs_bnode_desc), sizeof(head));
    tree->root = be32_to_cpu(head.root);
    tree->leaf_count = be32_to_cpu(head.leaf_count);
    tree->leaf_head = be32_to_cpu(head.leaf_head);
    tree->leaf_tail = be32_to_cpu(head.leaf_tail);
    tree->node_count = be32_to_cpu(head.node_count);
    tree->free_nodes = be32_to_cpu(head.free_nodes);
    tree->attributes = be32_to_cpu(head.attributes);
    tree->node_size = be16_to_cpu(head.node_size);
    tree->max_key_len = be16_to_cpu(head.max_key_len);
    tree->depth = be16_to_cpu(head.depth);
    if (tree->node_size < 512 || (tree->node_size & (tree->node_size - 1)) ||
        !tree->node_count || tree->node_count > tree->inode.i_size / tree->node_size ||
        tree->free_nodes > tree->node_count || tree->depth > 255 ||
        tree->root >= tree->node_count || tree->leaf_head >= tree->node_count ||
        tree->leaf_tail >= tree->node_count)
        goto free_tree;
    switch (id) {
    case HFS_EXT_CNID:
        if (tree->max_key_len != HFS_MAX_EXT_KEYLEN)
            goto free_tree;
        break;
    case HFS_CAT_CNID:
        if (tree->max_key_len != HFS_MAX_CAT_KEYLEN)
            goto free_tree;
        break;
    }
    if ((tree->root == 0) != (tree->depth == 0) ||
        (tree->leaf_count == 0) != (tree->leaf_head == 0) ||
        (tree->leaf_head == 0) != (tree->leaf_tail == 0) ||
        (tree->root == 0) != (tree->leaf_count == 0))
        goto free_tree;
    node = hfs_bnode_find(tree, HFS_TREE_HEAD);
    if (IS_ERR(node)) {
        if (PTR_ERR(node) == -ENOMEM)
            HFS_SB(sb)->error = -ENOMEM;
        goto free_tree;
    }
    if (node->type != HFS_NODE_HEADER || node->num_recs != 3) {
        hfs_bnode_put(node);
        goto free_tree;
    }
    hfs_bnode_put(node);
    return tree;
free_tree:
    free(tree);
    return NULL;
}

void hfs_btree_close(struct hfs_btree *tree)
{
    free(tree);
}

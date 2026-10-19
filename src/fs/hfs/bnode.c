// SPDX-License-Identifier: GPL-2.0
/*
 * linux/fs/hfs/bnode.c
 * Copyright (C) 2001 Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 * hfs_bnode_find validation is retained; a bounded node buffer replaces pages,
 * wait queues and the shared hash cache. Invalid reads also set an I/O error.
 */
#include "btree.h"

void hfs_bnode_read(struct hfs_bnode *node, void *buf, u32 off, u32 len)
{
    memset(buf, 0, len);
    if (off > node->tree->node_size || len > node->tree->node_size - off) {
        HFS_SB(node->tree->sb)->error = -EIO;
        return;
    }
    if (len)
        memcpy(buf, node->data + off, len);
}

u16 hfs_bnode_read_u16(struct hfs_bnode *node, u32 off)
{
    __be16 data;
    hfs_bnode_read(node, &data, off, 2);
    return be16_to_cpu(data);
}

u8 hfs_bnode_read_u8(struct hfs_bnode *node, u32 off)
{
    u8 data;
    hfs_bnode_read(node, &data, off, 1);
    return data;
}

struct hfs_bnode *hfs_bnode_find(struct hfs_btree *tree, u32 num)
{
    struct hfs_bnode *node;
    struct hfs_bnode_desc desc;
    int i, rec_off, off, next_off;
    int entry_size, key_size;
    if (num >= tree->node_count)
        return ERR_PTR(-EIO);
    node = calloc(1, sizeof(*node));
    if (!node)
        return ERR_PTR(-ENOMEM);
    node->tree = tree;
    node->this = num;
    node->data = malloc(tree->node_size);
    if (!node->data) {
        free(node);
        return ERR_PTR(-ENOMEM);
    }
    if (hfs_read_fork(&tree->inode, (u64)num * tree->node_size, node->data, tree->node_size))
        goto node_error;
    memcpy(&desc, node->data, sizeof(desc));
    node->prev = be32_to_cpu(desc.prev);
    node->next = be32_to_cpu(desc.next);
    node->num_recs = be16_to_cpu(desc.num_recs);
    node->type = desc.type;
    node->height = desc.height;
    if (node->prev >= tree->node_count || node->next >= tree->node_count ||
        node->num_recs > (tree->node_size - sizeof(desc)) / 2 - 1)
        goto node_error;
    switch (node->type) {
    case HFS_NODE_HEADER:
    case HFS_NODE_MAP:
        if (node->height != 0)
            goto node_error;
        break;
    case HFS_NODE_LEAF:
        if (node->height != 1 || !node->num_recs)
            goto node_error;
        break;
    case HFS_NODE_INDEX:
        if (node->height <= 1 || node->height > tree->depth || !node->num_recs)
            goto node_error;
        break;
    default:
        goto node_error;
    }
    rec_off = tree->node_size - 2;
    off = hfs_bnode_read_u16(node, rec_off);
    if (off != sizeof(struct hfs_bnode_desc))
        goto node_error;
    for (i = 1; i <= node->num_recs; off = next_off, i++) {
        rec_off -= 2;
        next_off = hfs_bnode_read_u16(node, rec_off);
        if (next_off <= off || next_off > tree->node_size - (node->num_recs + 1) * 2 ||
            next_off & 1)
            goto node_error;
        entry_size = next_off - off;
        if (node->type != HFS_NODE_INDEX && node->type != HFS_NODE_LEAF)
            continue;
        key_size = hfs_bnode_read_u8(node, off) + 1;
        if (key_size >= entry_size)
            goto node_error;
        /* The original record helpers use the padded/maximal key length.
         * Validate it before their read into the fixed search-key buffer. */
        key_size = hfs_brec_keylen(node, (u16)(i - 1));
        if (!key_size || key_size > entry_size - (node->type == HFS_NODE_INDEX ? 4 : 1))
            goto node_error;
        if (tree->cnid == HFS_CAT_CNID &&
            (key_size < 8 || node->data[off + 6] > HFS_NAMELEN ||
             7 + node->data[off + 6] > key_size))
            goto node_error;
        if (tree->cnid == HFS_EXT_CNID && key_size != sizeof(struct hfs_ext_key))
            goto node_error;
    }
    return node;
node_error:
    hfs_bnode_put(node);
    return ERR_PTR(-EIO);
}

void hfs_bnode_put(struct hfs_bnode *node)
{
    if (node) {
        free(node->data);
        free(node);
    }
}

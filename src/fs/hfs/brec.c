// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/hfs/brec.c
 *
 * Copyright (C) 2001
 * Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 *
 * Handle individual btree records
 */

#include "btree.h"

u16 hfs_brec_lenoff(struct hfs_bnode *node, u16 rec, u16 *off)
{
	__be16 retval[2];
	u16 dataoff;

	dataoff = node->tree->node_size - (rec + 2) * 2;
	hfs_bnode_read(node, retval, dataoff, 4);
	*off = be16_to_cpu(retval[1]);
	return be16_to_cpu(retval[0]) - *off;
}

u16 hfs_brec_keylen(struct hfs_bnode *node, u16 rec)
{
	u16 retval, recoff;

	if (node->type != HFS_NODE_INDEX && node->type != HFS_NODE_LEAF)
		return 0;

	if ((node->type == HFS_NODE_INDEX) &&
	   !(node->tree->attributes & HFS_TREE_VARIDXKEYS)) {
		if (node->tree->attributes & HFS_TREE_BIGKEYS)
			retval = node->tree->max_key_len + 2;
		else
			retval = node->tree->max_key_len + 1;
	} else {
		recoff = hfs_bnode_read_u16(node, node->tree->node_size - (rec + 1) * 2);
		if (!recoff)
			return 0;
		if (node->tree->attributes & HFS_TREE_BIGKEYS) {
			retval = hfs_bnode_read_u16(node, recoff) + 2;
			if (retval > node->tree->max_key_len + 2) {
				pr_err("keylen %d too large\n", retval);
				retval = 0;
			}
		} else {
			retval = (hfs_bnode_read_u8(node, recoff) | 1) + 1;
			if (retval > node->tree->max_key_len + 1) {
				pr_err("keylen %d too large\n", retval);
				retval = 0;
			}
		}
	}
	return retval;
}

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2007 Oracle. All rights reserved.
 * Copyright (C) Qu Wenruo 2017. All rights reserved. */
/* Read-side types adapted from Linux fs/btrfs/ctree.h and extent_io.h. */
#ifndef SEVENZIP_BTRFS_CTREE_H
#define SEVENZIP_BTRFS_CTREE_H
#include "../../include/uapi/linux/btrfs_tree.h"
#include "compat.h"

struct btrfs_fs_info;
struct extent_buffer {
    struct btrfs_fs_info *fs_info;
    u64 start;
    u32 len;
    u8 *data;
};
struct btrfs_path {
    struct extent_buffer *nodes[BTRFS_MAX_LEVEL];
    int slots[BTRFS_MAX_LEVEL];
};
struct btrfs_root {
    struct btrfs_fs_info *fs_info;
    u64 bytenr, generation, objectid;
    u8 level;
    u64 dirid;
};
struct btrfs_tree_parent_check {
    u64 owner_root;
    u64 transid;
    struct btrfs_key first_key;
    u8 level;
    bool has_first_key;
};

/* Buffer/transport services are implemented by the stream adapter. */
struct extent_buffer *read_tree_block(struct btrfs_fs_info *, u64,
                                      const struct btrfs_tree_parent_check *);
void free_extent_buffer(struct extent_buffer *);
void read_extent_buffer(const struct extent_buffer *, void *, size_t, size_t);

struct btrfs_path *btrfs_alloc_path(void);
void btrfs_release_path(struct btrfs_path *);
void btrfs_free_path(struct btrfs_path *);
int btrfs_comp_cpu_keys(const struct btrfs_key *, const struct btrfs_key *);
int btrfs_bin_search(const struct extent_buffer *, int, const struct btrfs_key *, int *);
/* Only plain searches are available; nonzero ins_len/cow are rejected. */
int btrfs_search_slot(void *, struct btrfs_root *, const struct btrfs_key *, struct btrfs_path *,
                      int ins_len, int cow);
struct extent_buffer *btrfs_read_node_slot(struct extent_buffer *, int);
int btrfs_next_leaf(struct btrfs_root *, struct btrfs_path *);
int btrfs_search_slot_for_read(struct btrfs_root *, const struct btrfs_key *, struct btrfs_path *,
                               int find_higher, int return_any);

#include "accessors.h"
#define btrfs_item_ptr(leaf, slot, type) ((type *)(uintptr_t)btrfs_item_ptr_offset(leaf, slot))
static inline int btrfs_next_item(struct btrfs_root *root, struct btrfs_path *path) {
    if (++path->slots[0] >= (int)btrfs_header_nritems(path->nodes[0]))
        return btrfs_next_leaf(root, path);
    return 0;
}
#endif

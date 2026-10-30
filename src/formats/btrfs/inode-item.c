// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2007 Oracle. All rights reserved. */
#include "formats/btrfs/fs.h"

int btrfs_lookup_inode(void *trans, struct btrfs_root *root, struct btrfs_path *path,
                       struct btrfs_key *location, int mod) {
    int ins_len = mod < 0 ? -1 : 0;
    int cow = mod != 0;
    int ret;
    int slot;
    struct extent_buffer *leaf;
    struct btrfs_key found_key;

    ret = btrfs_search_slot(trans, root, location, path, ins_len, cow);
    if (ret > 0 && location->type == BTRFS_ROOT_ITEM_KEY && location->offset == (u64)-1 &&
        path->slots[0] != 0) {
        slot = path->slots[0] - 1;
        leaf = path->nodes[0];
        btrfs_item_key_to_cpu(leaf, &found_key, slot);
        if (found_key.objectid == location->objectid && found_key.type == location->type) {
            path->slots[0]--;
            return 0;
        }
    }
    return ret;
}

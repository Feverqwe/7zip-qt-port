// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2007 Oracle. All rights reserved. */
#include "formats/btrfs/fs.h"
#define btrfs_warn(...) ((void)0)

static void btrfs_read_root_item(struct extent_buffer *eb, int slot, struct btrfs_root_item *item) {
    u32 len;
    bool need_reset = false;

    len = btrfs_item_size(eb, slot);
    read_extent_buffer(eb, item, btrfs_item_ptr_offset(eb, slot), min_t(u32, len, sizeof(*item)));
    if (len < sizeof(*item))
        need_reset = true;
    if (!need_reset && btrfs_root_generation(item) != btrfs_root_generation_v2(item)) {
        if (btrfs_root_generation_v2(item) != 0) {
            btrfs_warn(eb->fs_info,
                       "mismatching generation and generation_v2 found in root item. This root was "
                       "probably mounted with an older kernel. Resetting all new fields.");
        }
        need_reset = true;
    }
    if (need_reset) {
        /* Clear all members from generation_v2 onwards. */
        memset((u8 *)item + offsetof(struct btrfs_root_item, generation_v2), 0,
               sizeof(*item) - offsetof(struct btrfs_root_item, generation_v2));
        /* No invented on-disk UUID for a legacy read-only root. */
    }
}

int btrfs_find_root(struct btrfs_root *root, const struct btrfs_key *search_key,
                    struct btrfs_path *path, struct btrfs_root_item *root_item,
                    struct btrfs_key *root_key) {
    struct btrfs_key found_key;
    struct extent_buffer *l;
    int ret;
    int slot;

    ret = btrfs_search_slot(NULL, root, search_key, path, 0, 0);
    if (ret < 0)
        return ret;

    if (search_key->offset != -1ULL) { /* the search key is exact */
        if (ret > 0)
            goto out;
    } else {
        /*
         * Key with offset -1 found, there would have to exist a root
         * with such id, but this is out of the valid range.
         */
        if (unlikely(ret == 0)) {
            ret = -EUCLEAN;
            goto out;
        }
        if (path->slots[0] == 0)
            goto out;
        path->slots[0]--;
        ret = 0;
    }

    l = path->nodes[0];
    slot = path->slots[0];

    btrfs_item_key_to_cpu(l, &found_key, slot);
    if (found_key.objectid != search_key->objectid || found_key.type != BTRFS_ROOT_ITEM_KEY) {
        ret = 1;
        goto out;
    }

    if (root_item) {
        if (btrfs_item_size(l, slot) != sizeof(*root_item) &&
            btrfs_item_size(l, slot) != btrfs_legacy_root_item_size()) {
            ret = -EUCLEAN;
            goto out;
        }
        memset(root_item, 0, sizeof(*root_item));
        btrfs_read_root_item(l, slot, root_item);
    }
    if (root_key)
        memcpy(root_key, &found_key, sizeof(found_key));
out:
    btrfs_release_path(path);
    return ret;
}

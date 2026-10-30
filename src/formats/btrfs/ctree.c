// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2007,2008 Oracle.  All rights reserved.
 * Read-only adaptation; locking/COW and folio services are replaced by
 * immutable stream-backed buffers. See docs/upstream-port-audit/btrfs-linux.md.
 */
#include "formats/btrfs/ctree.h"

struct btrfs_path *btrfs_alloc_path(void) { return calloc(1, sizeof(struct btrfs_path)); }

void btrfs_release_path(struct btrfs_path *p) {
    for (int i = 0; i < BTRFS_MAX_LEVEL; i++) {
        p->slots[i] = 0;
        if (!p->nodes[i])
            continue;
        free_extent_buffer(p->nodes[i]);
        p->nodes[i] = NULL;
    }
}

void btrfs_free_path(struct btrfs_path *p) {
    if (!p)
        return;
    btrfs_release_path(p);
    free(p);
}

int btrfs_comp_cpu_keys(const struct btrfs_key *k1, const struct btrfs_key *k2) {
    if (k1->objectid > k2->objectid)
        return 1;
    if (k1->objectid < k2->objectid)
        return -1;
    if (k1->type > k2->type)
        return 1;
    if (k1->type < k2->type)
        return -1;
    if (k1->offset > k2->offset)
        return 1;
    if (k1->offset < k2->offset)
        return -1;
    return 0;
}

int btrfs_bin_search(const struct extent_buffer *eb, int first_slot, const struct btrfs_key *key,
                     int *slot) {
    unsigned long p;
    int item_size;
    /*
     * Use unsigned types for the low and high slots, so that we get a more
     * efficient division in the search loop below.
     */
    u32 low = first_slot;
    u32 high = btrfs_header_nritems(eb);
    int ret;
    const int key_size = sizeof(struct btrfs_disk_key);

    if (unlikely(low > high)) {
        return -EINVAL;
    }

    if (btrfs_header_level(eb) == 0) {
        p = offsetof(struct btrfs_leaf, items);
        item_size = sizeof(struct btrfs_item);
    } else {
        p = offsetof(struct btrfs_node, ptrs);
        item_size = sizeof(struct btrfs_key_ptr);
    }

    while (low < high) {
        struct btrfs_disk_key unaligned;
        struct btrfs_disk_key *tmp = &unaligned;
        u32 mid = (low + high) / 2;

        read_extent_buffer(eb, tmp, p + mid * item_size, key_size);

        ret = btrfs_comp_keys(tmp, key);

        if (ret < 0)
            low = mid + 1;
        else if (ret > 0)
            high = mid;
        else {
            *slot = mid;
            return 0;
        }
    }
    *slot = low;
    return 1;
}

struct extent_buffer *btrfs_read_node_slot(struct extent_buffer *parent, int slot) {
    int level = btrfs_header_level(parent);
    struct btrfs_tree_parent_check check = {0};

    if (slot < 0 || slot >= btrfs_header_nritems(parent))
        return ERR_PTR(-ENOENT);

    if (level <= 0 || level >= BTRFS_MAX_LEVEL)
        return ERR_PTR(-EUCLEAN);

    check.level = level - 1;
    check.transid = btrfs_node_ptr_generation(parent, slot);
    check.owner_root = btrfs_header_owner(parent);
    check.has_first_key = true;
    btrfs_node_key_to_cpu(parent, &check.first_key, slot);

    return read_tree_block(parent->fs_info, btrfs_node_blockptr(parent, slot), &check);
}

static inline int search_for_key_slot(const struct extent_buffer *eb, int search_low_slot,
                                      const struct btrfs_key *key, int prev_cmp, int *slot) {
    /*
     * If a previous call to btrfs_bin_search() on a parent node returned an
     * exact match (prev_cmp == 0), we can safely assume the target key will
     * always be at slot 0 on lower levels, since each key pointer
     * (struct btrfs_key_ptr) refers to the lowest key accessible from the
     * subtree it points to. Thus we can skip searching lower levels.
     */
    if (prev_cmp == 0) {
        *slot = 0;
        return 0;
    }

    return btrfs_bin_search(eb, search_low_slot, key, slot);
}

/* Linux btrfs_search_slot plain-search branch. Immutable input removes COW,
 * transaction setup, locks, readahead and retries for concurrent tree changes. */
int btrfs_search_slot(void *trans, struct btrfs_root *root, const struct btrfs_key *key,
                      struct btrfs_path *p, int ins_len, int cow) {
    struct extent_buffer *b;
    struct btrfs_tree_parent_check check = {0};
    int prev_cmp = -1;
    int ret;

    if (!root || !p || !key || trans || ins_len || cow)
        return -EINVAL;
    btrfs_release_path(p);
    check.level = root->level;
    check.transid = root->generation;
    check.owner_root = root->objectid;
    b = read_tree_block(root->fs_info, root->bytenr, &check);
    if (IS_ERR(b))
        return PTR_ERR(b);

    while (b) {
        int slot;
        const int level = btrfs_header_level(b);
        if (level >= BTRFS_MAX_LEVEL) {
            free_extent_buffer(b);
            ret = -EUCLEAN;
            goto done;
        }
        p->nodes[level] = b;
        if (level == 0) {
            ret = search_for_key_slot(b, 0, key, prev_cmp, &p->slots[0]);
            goto done;
        }
        if (!btrfs_header_nritems(b)) {
            ret = -EUCLEAN;
            goto done;
        }
        ret = search_for_key_slot(b, 0, key, prev_cmp, &slot);
        if (ret < 0)
            goto done;
        prev_cmp = ret;
        if (ret && slot > 0)
            slot--;
        p->slots[level] = slot;
        b = btrfs_read_node_slot(b, slot);
        if (IS_ERR(b)) {
            ret = PTR_ERR(b);
            goto done;
        }
    }
    ret = 1;
done:
    if (ret < 0)
        btrfs_release_path(p);
    return ret;
}

/* Linux btrfs_next_old_leaf with time_seq == 0. The snapshot is immutable,
 * so the saved ancestor path remains valid without re-searching the last key. */
int btrfs_next_leaf(struct btrfs_root *root, struct btrfs_path *path) {
    struct extent_buffer *c, *next;
    int slot = 0, level = 1;
    int ret;
    (void)root;
    if (!path->nodes[0] || !btrfs_header_nritems(path->nodes[0]))
        return 1;
    while (level < BTRFS_MAX_LEVEL) {
        c = path->nodes[level];
        if (!c)
            return 1;
        slot = path->slots[level] + 1;
        if (slot >= (int)btrfs_header_nritems(c)) {
            level++;
            continue;
        }
        break;
    }
    if (level == BTRFS_MAX_LEVEL)
        return 1;
    c = path->nodes[level];
    for (int i = 0; i < level; i++) {
        free_extent_buffer(path->nodes[i]);
        path->nodes[i] = NULL;
    }
    next = btrfs_read_node_slot(c, slot);
    if (IS_ERR(next)) {
        ret = PTR_ERR(next);
        goto fail;
    }
    path->slots[level] = slot;
    while (1) {
        level--;
        path->nodes[level] = next;
        path->slots[level] = 0;
        if (!level)
            break;
        next = btrfs_read_node_slot(next, 0);
        if (IS_ERR(next)) {
            ret = PTR_ERR(next);
            goto fail;
        }
    }
    return 0;
fail:
    btrfs_release_path(path);
    return ret;
}

static int btrfs_prev_leaf(struct btrfs_root *root, struct btrfs_path *path) {
    struct btrfs_key key;
    struct btrfs_key orig_key;
    struct btrfs_disk_key found_key;
    int ret;

    btrfs_item_key_to_cpu(path->nodes[0], &key, 0);
    orig_key = key;

    if (key.offset > 0) {
        key.offset--;
    } else if (key.type > 0) {
        key.type--;
        key.offset = (u64)-1;
    } else if (key.objectid > 0) {
        key.objectid--;
        key.type = (u8)-1;
        key.offset = (u64)-1;
    } else {
        return 1;
    }

    btrfs_release_path(path);
    ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
    if (ret <= 0)
        return ret;

    /*
     * Previous key not found. Even if we were at slot 0 of the leaf we had
     * before releasing the path and calling btrfs_search_slot(), we now may
     * be in a slot pointing to the same original key - this can happen if
     * after we released the path, one of more items were moved from a
     * sibling leaf into the front of the leaf we had due to an insertion
     * (see push_leaf_right()).
     * If we hit this case and our slot is > 0 and just decrement the slot
     * so that the caller does not process the same key again, which may or
     * may not break the caller, depending on its logic.
     */
    if (path->slots[0] < btrfs_header_nritems(path->nodes[0])) {
        btrfs_item_key(path->nodes[0], &found_key, path->slots[0]);
        ret = btrfs_comp_keys(&found_key, &orig_key);
        if (ret == 0) {
            if (path->slots[0] > 0) {
                path->slots[0]--;
                return 0;
            }
            /*
             * At slot 0, same key as before, it means orig_key is
             * the lowest, leftmost, key in the tree. We're done.
             */
            return 1;
        }
    }

    btrfs_item_key(path->nodes[0], &found_key, 0);
    ret = btrfs_comp_keys(&found_key, &key);
    /*
     * We might have had an item with the previous key in the tree right
     * before we released our path. And after we released our path, that
     * item might have been pushed to the first slot (0) of the leaf we
     * were holding due to a tree balance. Alternatively, an item with the
     * previous key can exist as the only element of a leaf (big fat item).
     * Therefore account for these 2 cases, so that our callers (like
     * btrfs_previous_item) don't miss an existing item with a key matching
     * the previous key we computed above.
     */
    if (ret <= 0)
        return 0;
    return 1;
}

int btrfs_search_slot_for_read(struct btrfs_root *root, const struct btrfs_key *key,
                               struct btrfs_path *p, int find_higher, int return_any) {
    int ret;
    struct extent_buffer *leaf;

again:
    ret = btrfs_search_slot(NULL, root, key, p, 0, 0);
    if (ret <= 0)
        return ret;
    /*
     * a return value of 1 means the path is at the position where the
     * item should be inserted. Normally this is the next bigger item,
     * but in case the previous item is the last in a leaf, path points
     * to the first free slot in the previous leaf, i.e. at an invalid
     * item.
     */
    leaf = p->nodes[0];

    if (find_higher) {
        if (p->slots[0] >= btrfs_header_nritems(leaf)) {
            ret = btrfs_next_leaf(root, p);
            if (ret <= 0)
                return ret;
            if (!return_any)
                return 1;
            /*
             * no higher item found, return the next
             * lower instead
             */
            return_any = 0;
            find_higher = 0;
            btrfs_release_path(p);
            goto again;
        }
    } else {
        if (p->slots[0] == 0) {
            ret = btrfs_prev_leaf(root, p);
            if (ret < 0)
                return ret;
            if (!ret) {
                leaf = p->nodes[0];
                if (p->slots[0] == btrfs_header_nritems(leaf))
                    p->slots[0]--;
                return 0;
            }
            if (!return_any)
                return 1;
            /*
             * no lower item found, return the next
             * higher instead
             */
            return_any = 0;
            find_higher = 1;
            btrfs_release_path(p);
            goto again;
        } else {
            --p->slots[0];
        }
    }
    return 0;
}

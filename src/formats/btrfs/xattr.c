// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2007 Red Hat. All rights reserved. */
/* Linux btrfs_listxattr/getxattr read-side walk and packed name/data layout. */
#include "formats/btrfs/fs.h"

int btrfs_read_xattrs(struct btrfs_file *file, void *context, btrfs_xattr_emit emit) {
    if (!file || !emit)
        return -EINVAL;
    struct btrfs_root *root = &file->root;
    struct btrfs_key key = {file->inode.info.ino, BTRFS_XATTR_ITEM_KEY, 0}, found_key;
    struct btrfs_path path = {0};
    int ret = btrfs_search_slot_for_read(root, &key, &path, 1, 0);
    while (!ret) {
        struct extent_buffer *leaf = path.nodes[0];
        const int slot = path.slots[0];
        btrfs_item_key_to_cpu(leaf, &found_key, slot);
        if (found_key.objectid != key.objectid || found_key.type != BTRFS_XATTR_ITEM_KEY)
            break;
        const size_t item_size = btrfs_item_size(leaf, slot);
        const size_t start = btrfs_item_ptr_offset(leaf, slot);
        size_t cur = 0;
        while (cur < item_size) {
            /* The connected Linux directory-item checker validates the packed
             * collision chain, lengths, type and name hash before this call. */
            struct btrfs_dir_item *di = (struct btrfs_dir_item *)(uintptr_t)(start + cur);
            const u16 name_len = btrfs_dir_name_len(leaf, di);
            const u16 data_len = btrfs_dir_data_len(leaf, di);
            char name[BTRFS_NAME_LEN + 1];
            if (!name_len || name_len > BTRFS_NAME_LEN) {
                ret = -EUCLEAN;
                goto out;
            }
            read_extent_buffer(leaf, name, start + cur + sizeof(*di), name_len);
            if (memchr(name, 0, name_len)) {
                ret = -EUCLEAN;
                goto out;
            }
            name[name_len] = 0;
            ret = emit(context, name, leaf->data + start + cur + sizeof(*di) + name_len, data_len);
            if (ret)
                goto out;
            ret = btrfs_poll(root->fs_info);
            if (ret)
                goto out;
            cur += sizeof(*di) + name_len + data_len;
        }
        ret = btrfs_next_item(root, &path);
    }
    if (ret > 0)
        ret = 0;
out:
    btrfs_release_path(&path);
    return ret;
}

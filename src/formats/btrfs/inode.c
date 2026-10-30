// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2007 Oracle. All rights reserved.
 * Read-side inode and directory paths adapted to synchronous stream buffers. */
#include "formats/btrfs/extent_map.h"

/* Linux btrfs_read_locked_inode metadata branch. There is no delayed-inode
 * cache, VFS mapping, ACL cache, or mutation/sync state in this reader. */
int btrfs_read_locked_inode(struct btrfs_inode *inode) {
    struct btrfs_path path = {0};
    struct btrfs_key location = {inode->info.ino, BTRFS_INODE_ITEM_KEY, 0};
    int ret = btrfs_lookup_inode(NULL, inode->root, &path, &location, 0);
    if (ret) {
        if (ret > 0)
            ret = -ENOENT;
        goto out;
    }
    struct extent_buffer *leaf = path.nodes[0];
    struct btrfs_inode_item *item = btrfs_item_ptr(leaf, path.slots[0], struct btrfs_inode_item);
    struct btrfs_file_info *info = &inode->info;
    info->mode = btrfs_inode_mode(leaf, item);
    info->nlink = btrfs_inode_nlink(leaf, item);
    info->uid = btrfs_inode_uid(leaf, item);
    info->gid = btrfs_inode_gid(leaf, item);
    info->size = btrfs_inode_size(leaf, item);
    info->packed_size = btrfs_inode_nbytes(leaf, item);
    info->flags = btrfs_inode_flags(leaf, item);
    info->rdev = btrfs_inode_rdev(leaf, item);
    info->mtime = (s64)btrfs_timespec_sec(leaf, &item->mtime);
    info->nsec = btrfs_timespec_nsec(leaf, &item->mtime);
    info->atime = (s64)btrfs_timespec_sec(leaf, &item->atime);
    info->atime_nsec = btrfs_timespec_nsec(leaf, &item->atime);
    info->ctime = (s64)btrfs_timespec_sec(leaf, &item->ctime);
    info->ctime_nsec = btrfs_timespec_nsec(leaf, &item->ctime);
    info->otime = (s64)btrfs_timespec_sec(leaf, &item->otime);
    info->otime_nsec = btrfs_timespec_nsec(leaf, &item->otime);
    if (info->nsec >= 1000000000 || info->atime_nsec >= 1000000000 ||
        info->ctime_nsec >= 1000000000 || info->otime_nsec >= 1000000000)
        ret = -EUCLEAN;
out:
    btrfs_release_path(&path);
    return ret;
}

/* Linux fixup_tree_root_location: cross a subvolume only after validating
 * the parent root reference, its directory inode and exact directory name. */
static int fixup_tree_root_location(struct btrfs_inode *dir, const char *name, size_t name_len,
                                    struct btrfs_key *location, struct btrfs_root *sub_root) {
    struct btrfs_fs_info *fs_info = dir->root->fs_info;
    struct btrfs_path path = {0};
    struct btrfs_key key = {dir->root->objectid, BTRFS_ROOT_REF_KEY, location->objectid};
    int ret = btrfs_search_slot(NULL, &fs_info->tree_root, &key, &path, 0, 0);
    if (ret) {
        if (ret > 0)
            ret = -ENOENT;
        goto out;
    }
    struct extent_buffer *leaf = path.nodes[0];
    const int slot = path.slots[0];
    if (btrfs_item_size(leaf, slot) < sizeof(struct btrfs_root_ref)) {
        ret = -EUCLEAN;
        goto out;
    }
    const struct btrfs_root_ref *ref =
        (const struct btrfs_root_ref *)(leaf->data + btrfs_item_ptr_offset(leaf, slot));
    const u16 len = get_unaligned_le16(&ref->name_len);
    if (sizeof(*ref) + len != btrfs_item_size(leaf, slot) ||
        get_unaligned_le64(&ref->dirid) != dir->info.ino || len != name_len ||
        memcmp(ref + 1, name, name_len)) {
        ret = -EUCLEAN;
        goto out;
    }
    btrfs_release_path(&path);
    ret = btrfs_read_tree_root(fs_info, location->objectid, sub_root);
    if (!ret) {
        location->objectid = sub_root->dirid;
        location->type = BTRFS_INODE_ITEM_KEY;
        location->offset = 0;
    }
out:
    btrfs_release_path(&path);
    return ret;
}

/* Linux btrfs_real_readdir DIR_INDEX walk. Immutable buffers allow direct
 * callbacks, without delayed items or a VFS page-fault staging buffer. */
int btrfs_real_readdir(struct btrfs_inode *inode, void *context, btrfs_dir_emit emit) {
    struct btrfs_path path = {0};
    struct btrfs_root *root = inode->root;
    struct btrfs_key key = {inode->info.ino, BTRFS_DIR_INDEX_KEY, 0}, found_key;
    int ret = btrfs_search_slot_for_read(root, &key, &path, 1, 0);
    while (!ret) {
        struct extent_buffer *leaf = path.nodes[0];
        const int slot = path.slots[0];
        btrfs_item_key_to_cpu(leaf, &found_key, slot);
        if (found_key.objectid != key.objectid || found_key.type != BTRFS_DIR_INDEX_KEY)
            break;
        struct btrfs_dir_item *di = btrfs_item_ptr(leaf, slot, struct btrfs_dir_item);
        const u32 name_len = btrfs_dir_name_len(leaf, di);
        /* Linux creates one DIR_INDEX per name; collision chains belong to
         * DIR_ITEM and XATTR_ITEM. Reject an index that would hide an entry. */
        if (!name_len || btrfs_item_size(leaf, slot) != sizeof(*di) + name_len) {
            ret = -EUCLEAN;
            break;
        }
        char name[BTRFS_NAME_LEN];
        read_extent_buffer(leaf, name, (size_t)(uintptr_t)di + sizeof(*di), name_len);
        struct btrfs_key location;
        btrfs_dir_item_key_to_cpu(leaf, di, &location);
        u64 rootid = root->objectid;
        const u8 ftype = btrfs_dir_ftype(leaf, di);
        if (location.type == BTRFS_ROOT_ITEM_KEY) {
            struct btrfs_root child;
            ret = fixup_tree_root_location(inode, name, name_len, &location, &child);
            if (ret)
                break;
            rootid = child.objectid;
        } else if (location.type != BTRFS_INODE_ITEM_KEY) {
            ret = -EUCLEAN;
            break;
        }
        ret = emit(context, name, name_len, rootid, location.objectid, ftype);
        if (ret) {
            btrfs_release_path(&path);
            return ret;
        }
        if (!ret)
            ret = btrfs_poll(root->fs_info);
        if (ret)
            break;
        ret = btrfs_next_item(root, &path);
    }
    btrfs_release_path(&path);
    return ret > 0 ? 0 : ret;
}

/* Linux btrfs_get_extent, without the mutable extent cache or VFS folios.
 * The caller retains path until inline data has been consumed. */
#define ASSERT(x) ((void)(x))
#define btrfs_crit(...) ((void)0)
#define trace_btrfs_get_extent_show_fi_regular(...) ((void)0)
#define trace_btrfs_get_extent_show_fi_inline(...) ((void)0)
int btrfs_get_extent(struct btrfs_inode *inode, u64 start, u64 len, struct btrfs_path *path,
                     struct extent_map *em) {
    struct btrfs_root *root = inode->root;
    struct btrfs_fs_info *fs_info = root->fs_info;
    const u64 objectid = inode->info.ino;
    u64 extent_start = 0, extent_end = 0;
    int ret, extent_type = -1;
    struct btrfs_file_extent_item *item;
    struct extent_buffer *leaf;
    struct btrfs_key found_key;
    memset(em, 0, sizeof(*em));
    em->start = EXTENT_MAP_HOLE;
    em->disk_bytenr = EXTENT_MAP_HOLE;
    em->len = UINT64_MAX;
    ret = btrfs_lookup_file_extent(NULL, root, path, objectid, start, 0);
    if (ret < 0) {
        goto out;
    } else if (ret > 0) {
        if (path->slots[0] == 0)
            goto not_found;
        path->slots[0]--;
        ret = 0;
    }

    leaf = path->nodes[0];
    item = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_file_extent_item);
    btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
    if (found_key.objectid != objectid || found_key.type != BTRFS_EXTENT_DATA_KEY) {
        /*
         * If we backup past the first extent we want to move forward
         * and see if there is an extent in front of us, otherwise we'll
         * say there is a hole for our whole search range which can
         * cause problems.
         */
        extent_end = start;
        goto next;
    }

    extent_type = btrfs_file_extent_type(leaf, item);
    extent_start = found_key.offset;
    extent_end = btrfs_file_extent_end(path);
    if (extent_type == BTRFS_FILE_EXTENT_REG || extent_type == BTRFS_FILE_EXTENT_PREALLOC) {
        /* Only regular file could have regular/prealloc extent */
        if (unlikely(!S_ISREG(inode->info.mode))) {
            ret = -EUCLEAN;
            btrfs_crit(fs_info, "regular/prealloc extent found for non-regular inode %llu",
                       btrfs_ino(inode));
            goto out;
        }
        trace_btrfs_get_extent_show_fi_regular(inode, leaf, item, extent_start);
    } else if (extent_type == BTRFS_FILE_EXTENT_INLINE) {
        trace_btrfs_get_extent_show_fi_inline(inode, leaf, item, path->slots[0], extent_start);
    }
next:
    if (start >= extent_end) {
        path->slots[0]++;
        if (path->slots[0] >= btrfs_header_nritems(leaf)) {
            ret = btrfs_next_leaf(root, path);
            if (ret < 0)
                goto out;
            else if (ret > 0)
                goto not_found;

            leaf = path->nodes[0];
        }
        btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
        if (found_key.objectid != objectid || found_key.type != BTRFS_EXTENT_DATA_KEY)
            goto not_found;
        if (start + len <= found_key.offset)
            goto not_found;
        if (start > found_key.offset)
            goto next;

        /* New extent overlaps with existing one */
        em->start = start;
        em->len = found_key.offset - start;
        em->disk_bytenr = EXTENT_MAP_HOLE;
        goto insert;
    }

    btrfs_extent_item_to_extent_map(inode, path, item, em);

    if (extent_type == BTRFS_FILE_EXTENT_REG || extent_type == BTRFS_FILE_EXTENT_PREALLOC) {
        goto insert;
    } else if (extent_type == BTRFS_FILE_EXTENT_INLINE) {
        /*
         * Inline extent can only exist at file offset 0. This is
         * ensured by tree-checker and inline extent creation path.
         * Thus all members representing file offsets should be zero.
         */
        ASSERT(extent_start == 0);
        ASSERT(em->start == 0);

        /*
         * btrfs_extent_item_to_extent_map() should have properly
         * initialized em members already.
         *
         * Other members are not utilized for inline extents.
         */
        ASSERT(em->disk_bytenr == EXTENT_MAP_INLINE);
        ASSERT(em->len == fs_info->sectorsize);

        /* The stream adapter consumes inline bytes through the held path. */
        goto insert;
    }
not_found:
    em->start = start;
    em->len = len;
    em->disk_bytenr = EXTENT_MAP_HOLE;
insert:
    if (em->start > start || btrfs_extent_map_end(em) <= start) {
        ret = -EIO;
        goto out;
    }
    ret = 0;
out:
    return ret;
}

// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2007 Oracle. All rights reserved. */
#include "extent_map.h"
#define ASSERT(...) ((void)0)
#define btrfs_err(...) ((void)0)
#define btrfs_ino(i) ((i)->info.ino)
#define btrfs_root_id(r) ((r)->objectid)

int btrfs_lookup_file_extent(void *trans, struct btrfs_root *root, struct btrfs_path *path,
                             u64 objectid, u64 offset, int mod) {
    struct btrfs_key file_key;
    int ins_len = mod < 0 ? -1 : 0;
    int cow = mod != 0;

    file_key.objectid = objectid;
    file_key.type = BTRFS_EXTENT_DATA_KEY;
    file_key.offset = offset;

    return btrfs_search_slot(trans, root, &file_key, path, ins_len, cow);
}
u64 btrfs_file_extent_end(const struct btrfs_path *path) {
    const struct extent_buffer *leaf = path->nodes[0];
    const int slot = path->slots[0];
    struct btrfs_file_extent_item *fi;
    struct btrfs_key key;
    u64 end;

    btrfs_item_key_to_cpu(leaf, &key, slot);
    ASSERT(key.type == BTRFS_EXTENT_DATA_KEY);
    fi = btrfs_item_ptr(leaf, slot, struct btrfs_file_extent_item);

    if (btrfs_file_extent_type(leaf, fi) == BTRFS_FILE_EXTENT_INLINE)
        end = leaf->fs_info->sectorsize;
    else
        end = key.offset + btrfs_file_extent_num_bytes(leaf, fi);

    return end;
}

void btrfs_extent_item_to_extent_map(struct btrfs_inode *inode, const struct btrfs_path *path,
                                     const struct btrfs_file_extent_item *fi,
                                     struct extent_map *em) {
    struct btrfs_fs_info *fs_info = inode->root->fs_info;
    struct extent_buffer *leaf = path->nodes[0];
    const int slot = path->slots[0];
    struct btrfs_key key;
    u64 extent_start;
    u8 type = btrfs_file_extent_type(leaf, fi);
    int compress_type = btrfs_file_extent_compression(leaf, fi);

    btrfs_item_key_to_cpu(leaf, &key, slot);
    extent_start = key.offset;
    em->ram_bytes = btrfs_file_extent_ram_bytes(leaf, fi);
    em->generation = btrfs_file_extent_generation(leaf, fi);
    if (type == BTRFS_FILE_EXTENT_REG || type == BTRFS_FILE_EXTENT_PREALLOC) {
        const u64 disk_bytenr = btrfs_file_extent_disk_bytenr(leaf, fi);

        em->start = extent_start;
        em->len = btrfs_file_extent_end(path) - extent_start;
        if (disk_bytenr == 0) {
            em->disk_bytenr = EXTENT_MAP_HOLE;
            em->disk_num_bytes = 0;
            em->offset = 0;
            return;
        }
        em->disk_bytenr = disk_bytenr;
        em->disk_num_bytes = btrfs_file_extent_disk_num_bytes(leaf, fi);
        em->offset = btrfs_file_extent_offset(leaf, fi);
        if (compress_type != BTRFS_COMPRESS_NONE) {
            btrfs_extent_map_set_compression(em, compress_type);
        } else {
            /*
             * Older kernels can create regular non-hole data
             * extents with ram_bytes smaller than disk_num_bytes.
             * Not a big deal, just always use disk_num_bytes
             * for ram_bytes.
             */
            em->ram_bytes = em->disk_num_bytes;
            if (type == BTRFS_FILE_EXTENT_PREALLOC)
                em->flags |= EXTENT_FLAG_PREALLOC;
        }
    } else if (type == BTRFS_FILE_EXTENT_INLINE) {
        /* Tree-checker has ensured this. */
        ASSERT(extent_start == 0);

        em->disk_bytenr = EXTENT_MAP_INLINE;
        em->start = 0;
        em->len = fs_info->sectorsize;
        em->offset = 0;
        btrfs_extent_map_set_compression(em, compress_type);
    } else {
        btrfs_err(fs_info,
                  "unknown file extent item type %d, inode %llu, offset %llu, "
                  "root %llu",
                  type, btrfs_ino(inode), extent_start, btrfs_root_id(root));
    }
}

static struct btrfs_csum_item *btrfs_lookup_csum(void *trans, struct btrfs_root *root,
                                                 struct btrfs_path *path, u64 bytenr, int cow) {
    struct btrfs_fs_info *fs_info = root->fs_info;
    int ret;
    struct btrfs_key file_key;
    struct btrfs_key found_key;
    struct btrfs_csum_item *item;
    struct extent_buffer *leaf;
    u64 csum_offset = 0;
    const u32 csum_size = fs_info->csum_size;
    int csums_in_item;

    file_key.objectid = BTRFS_EXTENT_CSUM_OBJECTID;
    file_key.type = BTRFS_EXTENT_CSUM_KEY;
    file_key.offset = bytenr;
    ret = btrfs_search_slot(trans, root, &file_key, path, 0, cow);
    if (ret < 0)
        goto fail;
    leaf = path->nodes[0];
    if (ret > 0) {
        ret = 1;
        if (path->slots[0] == 0)
            goto fail;
        path->slots[0]--;
        btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
        if (found_key.type != BTRFS_EXTENT_CSUM_KEY)
            goto fail;

        csum_offset = (bytenr - found_key.offset) >> fs_info->sectorsize_bits;
        csums_in_item = btrfs_item_size(leaf, path->slots[0]);
        csums_in_item /= csum_size;

        if (csum_offset == csums_in_item) {
            ret = -EFBIG;
            goto fail;
        } else if (csum_offset > csums_in_item) {
            goto fail;
        }
    }
    item = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_csum_item);
    item = (struct btrfs_csum_item *)((uintptr_t)item + csum_offset * csum_size);
    return item;
fail:
    if (ret > 0)
        ret = -ENOENT;
    return ERR_PTR(ret);
}

/* Sector transport for the Linux lookup/verify checksum read path. The held
 * csum leaf supplies subsequent sectors without repeatedly reading the tree. */
int btrfs_read_data(struct btrfs_inode *inode, u64 logical, void *data, size_t size,
                    size_t *processed) {
    struct btrfs_fs_info *info = inode->root->fs_info;
    struct btrfs_path path = {0};
    u8 *sector = malloc(info->sectorsize);
    u8 *dest = data;
    int ret = 0;
    *processed = 0;
    if (!sector)
        return -ENOMEM;
    if (size > UINT64_MAX - logical) {
        ret = -EIO;
        goto out;
    }
    while (size) {
        const u64 aligned = logical & ~((u64)info->sectorsize - 1);
        const size_t off = (size_t)(logical - aligned);
        const size_t count = min_t(size_t, size, info->sectorsize - off);
        const bool has_csum = !(inode->info.flags & BTRFS_INODE_NODATASUM);
        u8 expected[BTRFS_CSUM_SIZE];
        if (has_csum) {
            struct btrfs_key key;
            size_t pos = 0;
            bool cached = false;
            if (path.nodes[0]) {
                btrfs_item_key_to_cpu(path.nodes[0], &key, path.slots[0]);
                const u64 delta = aligned >= key.offset ? aligned - key.offset : UINT64_MAX;
                const u64 index = delta >> info->sectorsize_bits;
                const u32 bytes = btrfs_item_size(path.nodes[0], path.slots[0]);
                if (index < bytes / info->csum_size) {
                    pos = btrfs_item_ptr_offset(path.nodes[0], path.slots[0]) +
                          (size_t)index * info->csum_size;
                    cached = true;
                }
            }
            if (!cached) {
                btrfs_release_path(&path);
                struct btrfs_csum_item *item =
                    btrfs_lookup_csum(NULL, &info->csum_root, &path, aligned, 0);
                if (IS_ERR(item)) {
                    ret = PTR_ERR(item);
                    goto out;
                }
                pos = (size_t)(uintptr_t)item;
                const size_t start = btrfs_item_ptr_offset(path.nodes[0], path.slots[0]);
                const u32 bytes = btrfs_item_size(path.nodes[0], path.slots[0]);
                if (pos < start || pos - start > bytes || info->csum_size > bytes - (pos - start)) {
                    ret = -EUCLEAN;
                    goto out;
                }
            }
            read_extent_buffer(path.nodes[0], expected, pos, info->csum_size);
        }
        const unsigned int copies = btrfs_num_copies(info, aligned);
        for (unsigned int mirror = 1; mirror <= copies; mirror++) {
            ret = btrfs_read_logical(info, aligned, sector, info->sectorsize, mirror);
            if (!ret && has_csum) {
                u8 actual[BTRFS_CSUM_SIZE] = {0};
                ret = info->source.checksum(info->csum_type, sector, info->sectorsize, actual);
                if (!ret && memcmp(actual, expected, info->csum_size))
                    ret = -EILSEQ;
            }
            if (!ret || ret == -ECANCELED || ret == -ENOMEM)
                break;
        }
        if (ret)
            goto out;
        memcpy(dest, sector + off, count);
        *processed += count;
        dest += count;
        logical += count;
        size -= count;
    }
out:
    btrfs_release_path(&path);
    free(sector);
    return ret;
}

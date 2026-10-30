// SPDX-License-Identifier: GPL-2.0
/* File lifetime and contiguous-buffer services for the Linux read path. */
#include "formats/btrfs/extent_map.h"

int btrfs_open_file(struct btrfs_volume *volume, uint64_t root, uint64_t ino,
                    struct btrfs_file **result) {
    if (!result)
        return -EINVAL;
    *result = NULL;
    if (!volume ||
        (root != BTRFS_FS_TREE_OBJECTID && (root < BTRFS_FIRST_FREE_OBJECTID || root >> 48)))
        return -EINVAL;
    struct btrfs_file *file = calloc(1, sizeof(*file));
    if (!file)
        return -ENOMEM;
    int ret = btrfs_read_tree_root(&volume->fs_info, root, &file->root);
    if (!ret) {
        file->inode.root = &file->root;
        file->inode.info.root = root;
        file->inode.info.ino = ino ? ino : file->root.dirid;
        ret = btrfs_read_locked_inode(&file->inode);
    }
    if (ret) {
        free(file);
        return ret;
    }
    *result = file;
    return 0;
}

void btrfs_close_file(struct btrfs_file *file) { free(file); }

void btrfs_get_file_info(const struct btrfs_file *file, struct btrfs_file_info *info) {
    *info = file->inode.info;
}

int btrfs_read_directory(struct btrfs_file *file, void *context, btrfs_dir_emit emit) {
    if (!file || !emit)
        return -EINVAL;
    if (!S_ISDIR(file->inode.info.mode))
        return -ENOTDIR;
    return btrfs_real_readdir(&file->inode, context, emit);
}

/* Linux read_inline_extent/uncompress_inline through a contiguous buffer. */
static int read_inline_extent(struct btrfs_file *file, const struct btrfs_path *path, u64 start,
                              void *data, size_t size) {
    struct btrfs_fs_info *info = file->root.fs_info;
    struct extent_buffer *leaf = path->nodes[0];
    const int slot = path->slots[0];
    struct btrfs_file_extent_item *item = btrfs_item_ptr(leaf, slot, struct btrfs_file_extent_item);
    const unsigned int compression = btrfs_file_extent_compression(leaf, item);
    const size_t ram_size =
        (size_t)min_t(u64, btrfs_file_extent_ram_bytes(leaf, item), info->sectorsize);
    const size_t input_size = btrfs_item_size(leaf, slot) - BTRFS_FILE_EXTENT_INLINE_DATA_START;
    const u8 *input =
        leaf->data + btrfs_item_ptr_offset(leaf, slot) + BTRFS_FILE_EXTENT_INLINE_DATA_START;
    u8 *decoded = calloc(1, info->sectorsize);
    if (!decoded)
        return -ENOMEM;
    int ret = 0;
    if (!compression) {
        if (input_size < ram_size)
            ret = -EUCLEAN;
        else
            memcpy(decoded, input, ram_size);
    } else if (!info->source.decode) {
        ret = -EOPNOTSUPP;
    } else {
        ret = info->source.decode(info->source.context, compression, info->sectorsize, 1, input,
                                  input_size, decoded, ram_size);
    }
    if (!ret)
        memcpy(data, decoded + start, size);
    free(decoded);
    return ret;
}

/* The disk extent is verified before decompression. Extent offset refers to
 * the full decompressed extent, including slices shared through reflinks. */
static int read_compressed_extent(struct btrfs_file *file, const struct extent_map *em, u64 start,
                                  void *data, size_t size) {
    struct btrfs_fs_info *info = file->root.fs_info;
    if (!info->source.decode)
        return -EOPNOTSUPP;
    /* Linux compression.h caps both disk and decompressed extents at 128 KiB. */
    if (!em->disk_num_bytes || em->disk_num_bytes > (128U << 10) || !em->ram_bytes ||
        em->ram_bytes > (128U << 10) || em->offset > em->ram_bytes ||
        start > em->ram_bytes - em->offset || size > em->ram_bytes - em->offset - start)
        return -EUCLEAN;
    u8 *input = malloc((size_t)em->disk_num_bytes);
    u8 *decoded = calloc(1, (size_t)em->ram_bytes);
    if (!input || !decoded) {
        free(input);
        free(decoded);
        return -ENOMEM;
    }
    size_t processed;
    int ret = btrfs_read_data(&file->inode, em->disk_bytenr, input, (size_t)em->disk_num_bytes,
                              &processed);
    if (!ret)
        ret = info->source.decode(info->source.context, btrfs_extent_map_compression(em),
                                  info->sectorsize, 0, input, (size_t)em->disk_num_bytes, decoded,
                                  (size_t)em->ram_bytes);
    if (!ret)
        memcpy(data, decoded + em->offset + start, size);
    free(input);
    free(decoded);
    return ret;
}

int btrfs_pread_file(struct btrfs_file *file, uint64_t offset, void *data, size_t size,
                     size_t *processed) {
    if (!processed)
        return -EINVAL;
    *processed = 0;
    if (!file || (!data && size))
        return -EINVAL;
    if (!S_ISREG(file->inode.info.mode) && !S_ISLNK(file->inode.info.mode))
        return -EOPNOTSUPP;
    if (offset >= file->inode.info.size)
        return 0;
    size = (size_t)min_t(u64, size, file->inode.info.size - offset);
    u8 *dest = data;
    while (size) {
        struct btrfs_path path = {0};
        struct extent_map em;
        int ret = btrfs_poll(file->root.fs_info);
        if (!ret)
            ret = btrfs_get_extent(&file->inode, offset, size, &path, &em);
        if (ret) {
            btrfs_release_path(&path);
            return ret;
        }
        const u64 within = offset - em.start;
        const size_t count = (size_t)min_t(u64, size, em.len - within);
        size_t done = 0;
        if (em.disk_bytenr == EXTENT_MAP_HOLE || (em.flags & EXTENT_FLAG_PREALLOC)) {
            memset(dest, 0, count);
        } else if (em.disk_bytenr == EXTENT_MAP_INLINE) {
            ret = read_inline_extent(file, &path, offset, dest, count);
        } else if (btrfs_extent_map_compression(&em) != BTRFS_COMPRESS_NONE) {
            ret = read_compressed_extent(file, &em, within, dest, count);
        } else {
            u64 physical;
            if (em.offset > em.disk_num_bytes || within > em.disk_num_bytes - em.offset ||
                count > em.disk_num_bytes - em.offset - within ||
                check_add_overflow(em.disk_bytenr, em.offset, &physical) ||
                check_add_overflow(physical, within, &physical)) {
                ret = -EUCLEAN;
            } else {
                ret = btrfs_read_data(&file->inode, physical, dest, count, &done);
            }
        }
        btrfs_release_path(&path);
        *processed += ret ? done : count;
        if (ret)
            return ret;
        if (!count)
            return -EUCLEAN;
        dest += count;
        offset += count;
        size -= count;
    }
    return 0;
}

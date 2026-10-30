/* SPDX-License-Identifier: GPL-2.0 */
/* Read-side extent-map representation from Linux fs/btrfs/extent_map.h. */
#ifndef SEVENZIP_BTRFS_EXTENT_MAP_H
#define SEVENZIP_BTRFS_EXTENT_MAP_H
#include "formats/btrfs/fs.h"
#define EXTENT_MAP_HOLE ((u64) - 3)
#define EXTENT_MAP_INLINE ((u64) - 2)
enum {
    EXTENT_FLAG_PINNED = 1U << 0,
    EXTENT_FLAG_COMPRESS_ZLIB = 1U << 1,
    EXTENT_FLAG_COMPRESS_LZO = 1U << 2,
    EXTENT_FLAG_COMPRESS_ZSTD = 1U << 3,
    EXTENT_FLAG_PREALLOC = 1U << 4
};
struct extent_map {
    u64 start, len, disk_bytenr, disk_num_bytes, offset, ram_bytes, generation;
    u32 flags;
};
static inline void btrfs_extent_map_set_compression(struct extent_map *em,
                                                    enum btrfs_compression_type type) {
    if (type == BTRFS_COMPRESS_ZLIB)
        em->flags |= EXTENT_FLAG_COMPRESS_ZLIB;
    else if (type == BTRFS_COMPRESS_LZO)
        em->flags |= EXTENT_FLAG_COMPRESS_LZO;
    else if (type == BTRFS_COMPRESS_ZSTD)
        em->flags |= EXTENT_FLAG_COMPRESS_ZSTD;
}

static inline enum btrfs_compression_type
btrfs_extent_map_compression(const struct extent_map *em) {
    if (em->flags & EXTENT_FLAG_COMPRESS_ZLIB)
        return BTRFS_COMPRESS_ZLIB;

    if (em->flags & EXTENT_FLAG_COMPRESS_LZO)
        return BTRFS_COMPRESS_LZO;

    if (em->flags & EXTENT_FLAG_COMPRESS_ZSTD)
        return BTRFS_COMPRESS_ZSTD;

    return BTRFS_COMPRESS_NONE;
}

static inline u64 btrfs_extent_map_end(const struct extent_map *em) {
    if (em->start + em->len < em->start)
        return (u64)-1;
    return em->start + em->len;
}

u64 btrfs_file_extent_end(const struct btrfs_path *);
void btrfs_extent_item_to_extent_map(struct btrfs_inode *, const struct btrfs_path *,
                                     const struct btrfs_file_extent_item *, struct extent_map *);
int btrfs_get_extent(struct btrfs_inode *, u64, u64, struct btrfs_path *, struct extent_map *);
int btrfs_read_data(struct btrfs_inode *, u64, void *, size_t, size_t *);
#endif

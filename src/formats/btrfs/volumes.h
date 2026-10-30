/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2007 Oracle. All rights reserved.
 * Copyright (C) Qu Wenruo 2017. All rights reserved. */
/* Read-side geometry adapted from Linux fs/btrfs/volumes.h. */
#ifndef SEVENZIP_BTRFS_VOLUMES_H
#define SEVENZIP_BTRFS_VOLUMES_H
#include "formats/btrfs/fs.h"
const u8 *btrfs_sb_fsid_ptr(const struct btrfs_super_block *);
static inline size_t btrfs_chunk_item_size(int n) {
    return offsetof(struct btrfs_chunk, stripe) + sizeof(struct btrfs_stripe) * (size_t)n;
}
#define BTRFS_BLOCK_GROUP_VALID                                                                    \
    (BTRFS_BLOCK_GROUP_TYPE_MASK | BTRFS_BLOCK_GROUP_PROFILE_MASK | BTRFS_BLOCK_GROUP_REMAPPED)
enum btrfs_raid_types {
    BTRFS_RAID_SINGLE = 0,
    BTRFS_RAID_RAID0 = 1,
    BTRFS_RAID_RAID1 = 2,
    BTRFS_RAID_DUP = 3,
    BTRFS_RAID_RAID10 = 4,
    BTRFS_RAID_RAID5 = 5,
    BTRFS_RAID_RAID6 = 6,
    BTRFS_RAID_RAID1C3 = 7,
    BTRFS_RAID_RAID1C4 = 8,
    BTRFS_NR_RAID_TYPES
};

struct btrfs_raid_attr {
    u8 sub_stripes, dev_stripes, devs_min, ncopies, nparity;
};
extern const struct btrfs_raid_attr btrfs_raid_array[BTRFS_NR_RAID_TYPES];
enum btrfs_raid_types btrfs_bg_flags_to_raid_index(u64);
int btrfs_check_chunk_valid(const struct btrfs_fs_info *, const struct extent_buffer *,
                            const struct btrfs_chunk *, u64, u32);
static inline u64 btrfs_stripe_nr_to_offset(u64 nr) { return nr << BTRFS_STRIPE_LEN_SHIFT; }
static inline int nr_data_stripes(const struct btrfs_chunk_map *map) {
    const struct btrfs_raid_attr *a = &btrfs_raid_array[btrfs_bg_flags_to_raid_index(map->type)];
    return (map->num_stripes - a->nparity) / a->ncopies;
}
#endif

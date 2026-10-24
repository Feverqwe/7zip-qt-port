// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2007 Oracle. All rights reserved. */
#include "volumes.h"
#define BTRFS_BLOCK_GROUP_STRIPE_MASK                                                              \
    (BTRFS_BLOCK_GROUP_RAID0 | BTRFS_BLOCK_GROUP_RAID10 | BTRFS_BLOCK_GROUP_RAID56_MASK)

static struct btrfs_chunk_map *btrfs_find_chunk_map_nolock(struct btrfs_fs_info *, u64, u64);
static void set_real_chunk_type(struct btrfs_chunk_map *);
static int btrfs_chunk_map_cmp(const struct rb_node *, const struct rb_node *);

const struct btrfs_raid_attr btrfs_raid_array[BTRFS_NR_RAID_TYPES] = {
    [BTRFS_RAID_RAID10] =
        {
            .sub_stripes = 2,
            .dev_stripes = 1,

            .devs_min = 2,

            .ncopies = 2,
            .nparity = 0,

        },
    [BTRFS_RAID_RAID1] =
        {
            .sub_stripes = 1,
            .dev_stripes = 1,

            .devs_min = 2,

            .ncopies = 2,
            .nparity = 0,

        },
    [BTRFS_RAID_RAID1C3] =
        {
            .sub_stripes = 1,
            .dev_stripes = 1,

            .devs_min = 3,

            .ncopies = 3,
            .nparity = 0,

        },
    [BTRFS_RAID_RAID1C4] =
        {
            .sub_stripes = 1,
            .dev_stripes = 1,

            .devs_min = 4,

            .ncopies = 4,
            .nparity = 0,

        },
    [BTRFS_RAID_DUP] =
        {
            .sub_stripes = 1,
            .dev_stripes = 2,

            .devs_min = 1,

            .ncopies = 2,
            .nparity = 0,

        },
    [BTRFS_RAID_RAID0] =
        {
            .sub_stripes = 1,
            .dev_stripes = 1,

            .devs_min = 1,

            .ncopies = 1,
            .nparity = 0,

        },
    [BTRFS_RAID_SINGLE] =
        {
            .sub_stripes = 1,
            .dev_stripes = 1,

            .devs_min = 1,

            .ncopies = 1,
            .nparity = 0,

        },
    [BTRFS_RAID_RAID5] =
        {
            .sub_stripes = 1,
            .dev_stripes = 1,

            .devs_min = 2,

            .ncopies = 1,
            .nparity = 1,

        },
    [BTRFS_RAID_RAID6] =
        {
            .sub_stripes = 1,
            .dev_stripes = 1,

            .devs_min = 3,

            .ncopies = 1,
            .nparity = 2,

        },
};
enum btrfs_raid_types btrfs_bg_flags_to_raid_index(u64 flags) {
    const u64 profile = (flags & BTRFS_BLOCK_GROUP_PROFILE_MASK);

    if (!profile)
        return BTRFS_RAID_SINGLE;

    unsigned int index = 0;
    u64 bits = profile >> 2;
    while (bits >>= 1)
        index++;
    return (enum btrfs_raid_types)index;
}
const u8 *btrfs_sb_fsid_ptr(const struct btrfs_super_block *sb) {
    bool has_metadata_uuid =
        (btrfs_super_incompat_flags(sb) & BTRFS_FEATURE_INCOMPAT_METADATA_UUID);

    return has_metadata_uuid ? sb->metadata_uuid : sb->fsid;
}

/* Linux read_one_chunk: owned device IDs replace borrowed kernel devices;
 * maps remain owned by this volume, without concurrent allocation/removal. */
static int read_one_chunk(struct btrfs_key *key, struct extent_buffer *leaf,
                          struct btrfs_chunk *chunk) {
    struct btrfs_fs_info *fs_info = leaf->fs_info;
    const u64 logical = key->offset;
    const u64 length = btrfs_chunk_length(leaf, chunk);
    const u64 type = btrfs_chunk_type(leaf, chunk);
    const int num_stripes = btrfs_chunk_num_stripes(leaf, chunk);
    struct btrfs_chunk_map *map;
    int ret = btrfs_check_chunk_valid(fs_info, leaf, chunk, logical, fs_info->sectorsize);
    if (ret)
        return ret;
    if (type & BTRFS_BLOCK_GROUP_REMAPPED)
        return -EOPNOTSUPP;
    map = calloc(1, sizeof(*map) + sizeof(struct btrfs_io_stripe) * num_stripes);
    if (!map)
        return -ENOMEM;
    map->start = logical;
    map->chunk_len = length;
    map->num_stripes = num_stripes;
    map->on_disk_type = type;
    map->sub_stripes = btrfs_raid_array[btrfs_bg_flags_to_raid_index(type)].sub_stripes;
    set_real_chunk_type(map);
    for (int i = 0; i < num_stripes; i++) {
        const size_t offset = (size_t)(uintptr_t)chunk + offsetof(struct btrfs_chunk, stripe) +
                              (size_t)i * sizeof(struct btrfs_stripe);
        struct btrfs_stripe stripe;
        read_extent_buffer(leaf, &stripe, offset, sizeof(stripe));
        map->stripes[i].devid = get_unaligned_le64(&stripe.devid);
        map->stripes[i].physical = get_unaligned_le64(&stripe.offset);
        memcpy(map->stripes[i].uuid, stripe.dev_uuid, BTRFS_UUID_SIZE);
        if (!map->stripes[i].devid || !IS_ALIGNED(map->stripes[i].physical, fs_info->sectorsize)) {
            free(map);
            return -EUCLEAN;
        }
        /* Resolve companions while the archive opening callback is alive.
         * Missing peers are tested when reading their stripes; usable mirrors
         * can still supply data. Never suppress cancellation or allocation failure. */
        if (fs_info->source.open_device) {
            ret = fs_info->source.open_device(fs_info->source.context, map->stripes[i].devid,
                                              map->stripes[i].uuid);
            if (ret == -ECANCELED || ret == -ENOMEM) {
                free(map);
                return ret;
            }
        }
    }
    struct btrfs_chunk_map *old = btrfs_find_chunk_map_nolock(fs_info, logical, length);
    if (old) {
        /* A system chunk appears in both the superblock and the chunk tree. */
        ret = old->start == map->start && old->chunk_len == map->chunk_len &&
                      old->on_disk_type == map->on_disk_type && old->num_stripes == num_stripes
                  ? 0
                  : -EUCLEAN;
        for (int i = 0; !ret && i < num_stripes; i++)
            if (old->stripes[i].devid != map->stripes[i].devid ||
                old->stripes[i].physical != map->stripes[i].physical ||
                memcmp(old->stripes[i].uuid, map->stripes[i].uuid, BTRFS_UUID_SIZE))
                ret = -EUCLEAN;
        free(map);
        return ret;
    }
    RB_CLEAR_NODE(&map->rb_node);
    if (rb_find_add_cached(&map->rb_node, &fs_info->mapping_tree, btrfs_chunk_map_cmp)) {
        free(map);
        return -EEXIST;
    }
    return 0;
}

static struct btrfs_chunk_map *btrfs_find_chunk_map_nolock(struct btrfs_fs_info *fs_info,
                                                           u64 logical, u64 length) {
    struct rb_node *node = fs_info->mapping_tree.rb_root.rb_node;
    struct rb_node *prev = NULL;
    struct rb_node *orig_prev;
    struct btrfs_chunk_map *map;
    struct btrfs_chunk_map *prev_map = NULL;

    while (node) {
        map = rb_entry(node, struct btrfs_chunk_map, rb_node);
        prev = node;
        prev_map = map;

        if (logical < map->start) {
            node = node->rb_left;
        } else if (logical >= map->start + map->chunk_len) {
            node = node->rb_right;
        } else {
            return map;
        }
    }

    if (!prev)
        return NULL;

    orig_prev = prev;
    while (prev && logical >= prev_map->start + prev_map->chunk_len) {
        prev = rb_next(prev);
        prev_map = prev ? rb_entry(prev, struct btrfs_chunk_map, rb_node) : NULL;
    }

    if (!prev) {
        prev = orig_prev;
        prev_map = prev ? rb_entry(prev, struct btrfs_chunk_map, rb_node) : NULL;
        while (prev && logical < prev_map->start) {
            prev = rb_prev(prev);
            prev_map = prev ? rb_entry(prev, struct btrfs_chunk_map, rb_node) : NULL;
        }
    }

    if (prev) {
        u64 end = logical + length;

        /*
         * Caller can pass a U64_MAX length when it wants to get any
         * chunk starting at an offset of 'logical' or higher, so deal
         * with underflow by resetting the end offset to U64_MAX.
         */
        if (end < logical)
            end = U64_MAX;

        if (end > prev_map->start && logical < prev_map->start + prev_map->chunk_len) {
            return prev_map;
        }
    }

    return NULL;
}

static int btrfs_chunk_map_cmp(const struct rb_node *new, const struct rb_node *exist) {
    const struct btrfs_chunk_map *new_map = rb_entry(new, struct btrfs_chunk_map, rb_node);
    const struct btrfs_chunk_map *exist_map = rb_entry(exist, struct btrfs_chunk_map, rb_node);

    if (new_map->start == exist_map->start)
        return 0;
    if (new_map->start < exist_map->start)
        return -1;
    return 1;
}

static void set_real_chunk_type(struct btrfs_chunk_map *map) {
    map->type = map->on_disk_type;
    if (((map->on_disk_type & BTRFS_BLOCK_GROUP_RAID56_MASK) == 0 || nr_data_stripes(map) > 1))
        return;
    if (map->on_disk_type & BTRFS_BLOCK_GROUP_RAID5)
        map->type |= BTRFS_BLOCK_GROUP_RAID1;
    else
        map->type |= BTRFS_BLOCK_GROUP_RAID1C3;
    map->type &= ~BTRFS_BLOCK_GROUP_RAID56_MASK;
}

struct btrfs_io_geometry {
    u32 stripe_index;
    u32 stripe_nr;
    int mirror_num;
    int num_stripes;
    u64 stripe_offset;
    u64 raid56_full_stripe_start;
    int max_errors;
    int op;
    bool use_rst;
};
#define BTRFS_MAP_READ 0

static void map_blocks_raid0(const struct btrfs_chunk_map *map, struct btrfs_io_geometry *io_geom) {
    io_geom->stripe_index = io_geom->stripe_nr % map->num_stripes;
    io_geom->stripe_nr /= map->num_stripes;
    if (io_geom->op == BTRFS_MAP_READ)
        io_geom->mirror_num = 1;
}

static void map_blocks_dup(const struct btrfs_chunk_map *map, struct btrfs_io_geometry *io_geom) {
    (void)map;

    if (io_geom->mirror_num) {
        io_geom->stripe_index = io_geom->mirror_num - 1;
        return;
    }

    io_geom->mirror_num = 1;
}

static void map_blocks_single(const struct btrfs_chunk_map *map,
                              struct btrfs_io_geometry *io_geom) {
    io_geom->stripe_index = io_geom->stripe_nr % map->num_stripes;
    io_geom->stripe_nr /= map->num_stripes;
    io_geom->mirror_num = io_geom->stripe_index + 1;
}

static void map_blocks_raid56_read(struct btrfs_chunk_map *map, struct btrfs_io_geometry *io_geom) {
    int data_stripes = nr_data_stripes(map);

    (void)0;
    /* Just grab the data stripe directly. */
    io_geom->stripe_index = io_geom->stripe_nr % data_stripes;
    io_geom->stripe_nr /= data_stripes;

    /* We distribute the parity blocks across stripes. */
    io_geom->stripe_index = (io_geom->stripe_nr + io_geom->stripe_index) % map->num_stripes;

    if (io_geom->op == BTRFS_MAP_READ && io_geom->mirror_num < 1)
        io_geom->mirror_num = 1;
}

static void map_blocks_raid1(struct btrfs_fs_info *fs_info, struct btrfs_chunk_map *map,
                             struct btrfs_io_geometry *io_geom, bool dev_replace_is_ongoing) {
    (void)fs_info;
    (void)dev_replace_is_ongoing;
    (void)map;

    if (io_geom->mirror_num) {
        io_geom->stripe_index = io_geom->mirror_num - 1;
        return;
    }
}

static void map_blocks_raid10(struct btrfs_fs_info *fs_info, struct btrfs_chunk_map *map,
                              struct btrfs_io_geometry *io_geom, bool dev_replace_is_ongoing) {
    (void)fs_info;
    (void)dev_replace_is_ongoing;
    u32 factor = map->num_stripes / map->sub_stripes;

    io_geom->stripe_index = (io_geom->stripe_nr % factor) * map->sub_stripes;
    io_geom->stripe_nr /= factor;

    if (io_geom->mirror_num) {
        io_geom->stripe_index += io_geom->mirror_num - 1;
        return;
    }
}

/* Read-only portion of btrfs_map_block. All callers request an explicit
 * mirror; kernel bio allocation and device-replace/write paths are absent. */
static int btrfs_map_block(struct btrfs_fs_info *fs_info, u64 logical, u64 *length,
                           struct btrfs_io_stripe *dst, unsigned int mirror) {
    struct btrfs_chunk_map *map = btrfs_find_chunk_map_nolock(fs_info, logical, *length);
    struct btrfs_io_geometry geom = {0};
    u64 map_offset, max_len;
    if (!map || logical < map->start || logical - map->start >= map->chunk_len)
        return -EIO;
    if (!mirror ||
        mirror > (unsigned int)btrfs_raid_array[btrfs_bg_flags_to_raid_index(map->type)].ncopies)
        return -EINVAL;
    geom.mirror_num = (int)mirror;
    geom.num_stripes = 1;
    geom.op = BTRFS_MAP_READ;
    map_offset = logical - map->start;
    geom.stripe_offset = map_offset & BTRFS_STRIPE_LEN_MASK;
    geom.stripe_nr = (u32)(map_offset >> BTRFS_STRIPE_LEN_SHIFT);
    max_len = map->type & BTRFS_BLOCK_GROUP_STRIPE_MASK ? BTRFS_STRIPE_LEN - geom.stripe_offset
                                                        : UINT64_MAX;
    *length = min_t(u64, *length, min_t(u64, map->chunk_len - map_offset, max_len));
    switch (map->type & BTRFS_BLOCK_GROUP_PROFILE_MASK) {
    case BTRFS_BLOCK_GROUP_RAID0:
        map_blocks_raid0(map, &geom);
        break;
    case BTRFS_BLOCK_GROUP_RAID1:
    case BTRFS_BLOCK_GROUP_RAID1C3:
    case BTRFS_BLOCK_GROUP_RAID1C4:
        map_blocks_raid1(fs_info, map, &geom, false);
        break;
    case BTRFS_BLOCK_GROUP_DUP:
        map_blocks_dup(map, &geom);
        break;
    case BTRFS_BLOCK_GROUP_RAID10:
        map_blocks_raid10(fs_info, map, &geom, false);
        break;
    case BTRFS_BLOCK_GROUP_RAID5:
    case BTRFS_BLOCK_GROUP_RAID6:
        map_blocks_raid56_read(map, &geom);
        break;
    default:
        map_blocks_single(map, &geom);
        break;
    }
    if (geom.stripe_index >= (u32)map->num_stripes || !*length)
        return -EUCLEAN;
    *dst = map->stripes[geom.stripe_index];
    u64 offset;
    if (check_add_overflow(geom.stripe_offset, btrfs_stripe_nr_to_offset(geom.stripe_nr),
                           &offset) ||
        check_add_overflow(dst->physical, offset, &dst->physical))
        return -EUCLEAN;
    return 0;
}

unsigned int btrfs_num_copies(struct btrfs_fs_info *fs_info, u64 logical) {
    struct btrfs_chunk_map *map = btrfs_find_chunk_map_nolock(fs_info, logical, 1);
    return map ? btrfs_raid_array[btrfs_bg_flags_to_raid_index(map->type)].ncopies : 1;
}

int btrfs_read_logical(struct btrfs_fs_info *fs_info, u64 logical, void *data, size_t size,
                       unsigned int mirror) {
    u8 *dest = data;
    if (size > UINT64_MAX - logical)
        return -EIO;
    while (size) {
        struct btrfs_io_stripe stripe;
        u64 length = size;
        int ret = btrfs_poll(fs_info);
        if (!ret)
            ret = btrfs_map_block(fs_info, logical, &length, &stripe, mirror);
        if (ret)
            return ret;
        ret = btrfs_read_physical(fs_info, stripe.devid, stripe.uuid, stripe.physical, dest,
                                  (size_t)length);
        if (ret)
            return ret;
        dest += length;
        logical += length;
        size -= (size_t)length;
    }
    return 0;
}

int btrfs_read_sys_array(struct btrfs_fs_info *fs_info) {
    struct btrfs_super_block *super_copy = &fs_info->super_copy;
    struct extent_buffer sb = {fs_info, BTRFS_SUPER_INFO_OFFSET, BTRFS_SUPER_INFO_SIZE,
                               (u8 *)super_copy};
    const u32 array_size = btrfs_super_sys_array_size(super_copy);
    u32 cur_offset = 0;
    /* btrfs_validate_super checked the complete system array before this call. */
    while (cur_offset < array_size) {
        struct btrfs_key key;
        struct btrfs_disk_key *disk_key =
            (struct btrfs_disk_key *)(super_copy->sys_chunk_array + cur_offset);
        btrfs_disk_key_to_cpu(&key, disk_key);
        cur_offset += sizeof(*disk_key);
        size_t offset = offsetof(struct btrfs_super_block, sys_chunk_array) + cur_offset;
        struct btrfs_chunk *chunk = (struct btrfs_chunk *)(uintptr_t)offset;
        const size_t length = btrfs_chunk_item_size(btrfs_chunk_num_stripes(&sb, chunk));
        int ret = read_one_chunk(&key, &sb, chunk);
        if (ret)
            return ret;
        cur_offset += (u32)length;
    }
    return 0;
}

int btrfs_read_chunk_tree(struct btrfs_fs_info *fs_info) {
    struct btrfs_path path = {0};
    struct btrfs_key key = {BTRFS_DEV_ITEMS_OBJECTID, 0, 0};
    int ret = btrfs_search_slot_for_read(&fs_info->chunk_root, &key, &path, 1, 0);
    while (!ret) {
        struct extent_buffer *leaf = path.nodes[0];
        const int slot = path.slots[0];
        btrfs_item_key_to_cpu(leaf, &key, slot);
        if (key.type == BTRFS_DEV_ITEM_KEY) {
            if (btrfs_item_size(leaf, slot) != sizeof(struct btrfs_dev_item)) {
                ret = -EUCLEAN;
                break;
            }
            const struct btrfs_dev_item *item =
                (const struct btrfs_dev_item *)(leaf->data + btrfs_item_ptr_offset(leaf, slot));
            if (get_unaligned_le64(&item->devid) != key.offset ||
                memcmp(item->fsid, fs_info->metadata_uuid, BTRFS_FSID_SIZE)) {
                ret = -EUCLEAN;
                break;
            }
        } else if (key.type == BTRFS_CHUNK_ITEM_KEY) {
            if (btrfs_item_size(leaf, slot) < sizeof(struct btrfs_chunk)) {
                ret = -EUCLEAN;
                break;
            }
            struct btrfs_chunk *chunk =
                (struct btrfs_chunk *)(uintptr_t)btrfs_item_ptr_offset(leaf, slot);
            if (btrfs_item_size(leaf, slot) !=
                btrfs_chunk_item_size(btrfs_chunk_num_stripes(leaf, chunk))) {
                ret = -EUCLEAN;
                break;
            }
            ret = read_one_chunk(&key, leaf, chunk);
            if (ret)
                break;
        }
        ret = btrfs_poll(fs_info);
        if (ret)
            break;
        if (++path.slots[0] >= (int)btrfs_header_nritems(leaf))
            ret = btrfs_next_leaf(&fs_info->chunk_root, &path);
    }
    btrfs_release_path(&path);
    return ret < 0 ? ret : 0;
}

void btrfs_free_mapping_tree(struct btrfs_fs_info *fs_info) {
    struct rb_node *node;
    while ((node = rb_first(&fs_info->mapping_tree.rb_root))) {
        rb_erase_cached(node, &fs_info->mapping_tree);
        free(rb_entry(node, struct btrfs_chunk_map, rb_node));
    }
}

// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) Qu Wenruo 2017. All rights reserved. */
/* Linux read-time validation, with kernel diagnostics returned as errors. */
#include "formats/btrfs/volumes.h"
static int check_leaf_item_read(struct extent_buffer *, struct btrfs_key *, int,
                                struct btrfs_key *);

/* Structural part of Linux __btrfs_check_leaf. Consumers validate their
 * item bodies before interpreting variable-length inode/directory data. */
int btrfs_check_leaf(struct extent_buffer *leaf) {
    struct btrfs_key prev_key = {0, 0, 0}, key;
    const u32 nritems = btrfs_header_nritems(leaf);
    const u32 data_size = leaf->len - sizeof(struct btrfs_header);
    if (btrfs_header_level(leaf) != 0 ||
        !(get_unaligned_le64(leaf->data + offsetof(struct btrfs_header, flags)) &
          BTRFS_HEADER_FLAG_WRITTEN) ||
        nritems > data_size / sizeof(struct btrfs_item))
        return -EUCLEAN;
    if (!nritems && !(get_unaligned_le64(leaf->data + offsetof(struct btrfs_header, flags)) &
                      BTRFS_HEADER_FLAG_RELOC)) {
        const u64 owner = btrfs_header_owner(leaf);
        if (!owner || owner == BTRFS_ROOT_TREE_OBJECTID || owner == BTRFS_CHUNK_TREE_OBJECTID ||
            owner == BTRFS_DEV_TREE_OBJECTID || owner == BTRFS_FS_TREE_OBJECTID ||
            owner == BTRFS_DATA_RELOC_TREE_OBJECTID ||
            (owner == BTRFS_EXTENT_TREE_OBJECTID &&
             !btrfs_fs_incompat(leaf->fs_info, EXTENT_TREE_V2)))
            return -EUCLEAN;
    }
    for (u32 slot = 0; slot < nritems; slot++) {
        const u64 item_data_end = (u64)btrfs_item_offset(leaf, slot) + btrfs_item_size(leaf, slot);
        const u32 item_end_expected = slot ? btrfs_item_offset(leaf, slot - 1) : data_size;
        btrfs_item_key_to_cpu(leaf, &key, slot);
        if (btrfs_comp_cpu_keys(&prev_key, &key) >= 0 || item_data_end != item_end_expected ||
            item_data_end > data_size ||
            btrfs_item_ptr_offset(leaf, slot) <
                sizeof(struct btrfs_header) + (size_t)nritems * sizeof(struct btrfs_item))
            return -EUCLEAN;
        int ret = check_leaf_item_read(leaf, &key, slot, &prev_key);
        if (ret)
            return ret;
        prev_key = key;
    }
    return 0;
}

/* Linux __btrfs_check_node, also checking the last pointer before access. */
int btrfs_check_node(struct extent_buffer *node) {
    const u32 nr = btrfs_header_nritems(node);
    const int level = btrfs_header_level(node);
    if (level <= 0 || level >= BTRFS_MAX_LEVEL || !nr ||
        nr > (node->len - sizeof(struct btrfs_header)) / sizeof(struct btrfs_key_ptr) ||
        !(get_unaligned_le64(node->data + offsetof(struct btrfs_header, flags)) &
          BTRFS_HEADER_FLAG_WRITTEN))
        return -EUCLEAN;
    for (u32 slot = 0; slot < nr; slot++) {
        struct btrfs_key key, next_key;
        const u64 bytenr = btrfs_node_blockptr(node, slot);
        if (!bytenr || !IS_ALIGNED(bytenr, node->fs_info->sectorsize))
            return -EUCLEAN;
        if (slot + 1 < nr) {
            btrfs_node_key_to_cpu(node, &key, slot);
            btrfs_node_key_to_cpu(node, &next_key, slot + 1);
            if (btrfs_comp_cpu_keys(&key, &next_key) >= 0)
                return -EUCLEAN;
        }
    }
    return 0;
}

static bool btrfs_is_fstree(u64 rootid) {
    if (rootid == BTRFS_FS_TREE_OBJECTID)
        return true;
    if ((s64)rootid < (s64)BTRFS_FIRST_FREE_OBJECTID)
        return false;
    if (rootid >> 48)
        return false;
    return true;
}

int btrfs_check_eb_owner(const struct extent_buffer *eb, u64 root_owner) {
    const bool is_subvol = btrfs_is_fstree(root_owner);
    const u64 eb_owner = btrfs_header_owner(eb);
    if (!root_owner || root_owner == BTRFS_TREE_LOG_OBJECTID ||
        root_owner == BTRFS_TREE_RELOC_OBJECTID)
        return 0;
    if (!is_subvol)
        return root_owner == eb_owner ? 0 : -EUCLEAN;
    /* Snapshots may share blocks whose owner is a different subvolume. */
    return is_subvol == btrfs_is_fstree(eb_owner) ? 0 : -EUCLEAN;
}

static bool valid_stripe_count(u64 profile, u16 num_stripes, u16 sub_stripes) {
    switch (profile) {
    case BTRFS_BLOCK_GROUP_RAID0:
        return true;
    case BTRFS_BLOCK_GROUP_RAID10:
        return sub_stripes == btrfs_raid_array[BTRFS_RAID_RAID10].sub_stripes;
    case BTRFS_BLOCK_GROUP_RAID1:
        return num_stripes == btrfs_raid_array[BTRFS_RAID_RAID1].devs_min;
    case BTRFS_BLOCK_GROUP_RAID1C3:
        return num_stripes == btrfs_raid_array[BTRFS_RAID_RAID1C3].devs_min;
    case BTRFS_BLOCK_GROUP_RAID1C4:
        return num_stripes == btrfs_raid_array[BTRFS_RAID_RAID1C4].devs_min;
    case BTRFS_BLOCK_GROUP_RAID5:
        return num_stripes >= btrfs_raid_array[BTRFS_RAID_RAID5].devs_min;
    case BTRFS_BLOCK_GROUP_RAID6:
        return num_stripes >= btrfs_raid_array[BTRFS_RAID_RAID6].devs_min;
    case BTRFS_BLOCK_GROUP_DUP:
        return num_stripes == btrfs_raid_array[BTRFS_RAID_DUP].dev_stripes;
    case 0: /* SINGLE */
        return num_stripes == btrfs_raid_array[BTRFS_RAID_SINGLE].dev_stripes;
    default:
        return false;
    }
}

int btrfs_check_chunk_valid(const struct btrfs_fs_info *fs_info, const struct extent_buffer *leaf,
                            const struct btrfs_chunk *chunk, u64 logical, u32 sectorsize) {
    u64 length;
    u64 chunk_end;
    u64 stripe_len;
    u16 num_stripes;
    u16 sub_stripes;
    u64 type;
    u64 features;
    u32 chunk_sector_size;
    bool mixed = false;
    bool remapped;
    int raid_index;
    int nparity;
    int ncopies;

    if (leaf) {
        length = btrfs_chunk_length(leaf, chunk);
        stripe_len = btrfs_chunk_stripe_len(leaf, chunk);
        num_stripes = btrfs_chunk_num_stripes(leaf, chunk);
        sub_stripes = btrfs_chunk_sub_stripes(leaf, chunk);
        type = btrfs_chunk_type(leaf, chunk);
        chunk_sector_size = btrfs_chunk_sector_size(leaf, chunk);
    } else {
        length = btrfs_stack_chunk_length(chunk);
        stripe_len = btrfs_stack_chunk_stripe_len(chunk);
        num_stripes = btrfs_stack_chunk_num_stripes(chunk);
        sub_stripes = btrfs_stack_chunk_sub_stripes(chunk);
        type = btrfs_stack_chunk_type(chunk);
        chunk_sector_size = btrfs_stack_chunk_sector_size(chunk);
    }
    /* Reject invalid profiles before indexing the Linux geometry table. */
    if ((type & ~BTRFS_BLOCK_GROUP_VALID) ||
        ((type & BTRFS_BLOCK_GROUP_PROFILE_MASK) &&
         !has_single_bit_set(type & BTRFS_BLOCK_GROUP_PROFILE_MASK)))
        return -EUCLEAN;
    raid_index = btrfs_bg_flags_to_raid_index(type);
    ncopies = btrfs_raid_array[raid_index].ncopies;
    nparity = btrfs_raid_array[raid_index].nparity;
    remapped = (type & BTRFS_BLOCK_GROUP_REMAPPED);

    if (unlikely(!remapped && !num_stripes)) {
        return -EUCLEAN;
    }
    if (unlikely(num_stripes != 0 && num_stripes < ncopies)) {
        return -EUCLEAN;
    }
    if (unlikely(nparity && num_stripes == nparity)) {
        return -EUCLEAN;
    }
    if (unlikely(!IS_ALIGNED(logical, sectorsize))) {
        return -EUCLEAN;
    }
    if (unlikely(chunk_sector_size != sectorsize)) {
        return -EUCLEAN;
    }
    if (unlikely(!length || !IS_ALIGNED(length, sectorsize))) {
        return -EUCLEAN;
    }
    if (unlikely(check_add_overflow(logical, length, &chunk_end))) {
        return -EUCLEAN;
    }
    if (unlikely(!is_power_of_2(stripe_len) || stripe_len != BTRFS_STRIPE_LEN)) {
        return -EUCLEAN;
    }
    /*
     * We artificially limit the chunk size, so that the number of stripes
     * inside a chunk can be fit into a U32.  The current limit (256G) is
     * way too large for real world usage anyway, and it's also much larger
     * than our existing limit (10G).
     *
     * Thus it should be a good way to catch obvious bitflips.
     */
    if (unlikely(length >= btrfs_stripe_nr_to_offset(U32_MAX))) {
        return -EUCLEAN;
    }
    if (unlikely(type & ~BTRFS_BLOCK_GROUP_VALID)) {
        return -EUCLEAN;
    }

    if (unlikely(!has_single_bit_set(type & BTRFS_BLOCK_GROUP_PROFILE_MASK) &&
                 (type & BTRFS_BLOCK_GROUP_PROFILE_MASK) != 0)) {
        return -EUCLEAN;
    }
    if (unlikely((type & BTRFS_BLOCK_GROUP_TYPE_MASK) == 0)) {
        return -EUCLEAN;
    }

    if (unlikely((type & BTRFS_BLOCK_GROUP_SYSTEM) &&
                 (type & (BTRFS_BLOCK_GROUP_METADATA | BTRFS_BLOCK_GROUP_DATA)))) {
        return -EUCLEAN;
    }

    features = btrfs_super_incompat_flags(&fs_info->super_copy);
    if (features & BTRFS_FEATURE_INCOMPAT_MIXED_GROUPS)
        mixed = true;

    if (!mixed) {
        if (unlikely((type & BTRFS_BLOCK_GROUP_METADATA) && (type & BTRFS_BLOCK_GROUP_DATA))) {
            return -EUCLEAN;
        }
    }

    if (unlikely((type & BTRFS_BLOCK_GROUP_METADATA_REMAP) &&
                 !(features & BTRFS_FEATURE_INCOMPAT_REMAP_TREE))) {
        return -EUCLEAN;
    }

    if (unlikely(remapped && !(features & BTRFS_FEATURE_INCOMPAT_REMAP_TREE))) {
        return -EUCLEAN;
    }

    if (!remapped &&
        !valid_stripe_count(type & BTRFS_BLOCK_GROUP_PROFILE_MASK, num_stripes, sub_stripes)) {
        return -EUCLEAN;
    }

    return 0;
}

/* Linux item validators. Diagnostics do not introduce a write/repair service. */
#define generic_err(...) ((void)0)
#define dir_item_err(...) ((void)0)
#define inode_item_err(...) ((void)0)
#define file_extent_err(...) ((void)0)
#define ASSERT(...) ((void)0)
#define IS_ENABLED(x) 0
#define WARN_ON(x) (!!(x))
#define sb_rdonly(x) true
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define XATTR_NAME_MAX 255
#define BTRFS_MAX_XATTR_SIZE(info)                                                                 \
    ((info)->nodesize - sizeof(struct btrfs_header) - sizeof(struct btrfs_item) -                  \
     sizeof(struct btrfs_dir_item))
#define CHECK_FE_ALIGNED(leaf, slot, fi, name, a)                                                  \
    (!IS_ALIGNED(btrfs_file_extent_##name(leaf, fi), a))
#define ALIGN(n, a) (((n) + (a) - 1) & ~((u64)(a) - 1))
static bool btrfs_is_fstree(u64);
static bool check_prev_ino(struct extent_buffer *leaf, struct btrfs_key *key, int slot,
                           struct btrfs_key *prev_key) {
    /* No prev key, skip check */
    if (slot == 0)
        return true;

    /* Only these key->types needs to be checked */
    ASSERT(key->type == BTRFS_XATTR_ITEM_KEY || key->type == BTRFS_INODE_REF_KEY ||
               key->type == BTRFS_INODE_EXTREF_KEY || key->type == BTRFS_DIR_INDEX_KEY ||
               key->type == BTRFS_DIR_ITEM_KEY || key->type == BTRFS_EXTENT_DATA_KEY,
           "key->type=%u", key->type);

    /*
     * Only subvolume trees along with their reloc trees need this check.
     * Things like log tree doesn't follow this ino requirement.
     */
    if (!btrfs_is_fstree(btrfs_header_owner(leaf)))
        return true;

    if (key->objectid == prev_key->objectid)
        return true;

    /* Error found */
    dir_item_err(leaf, slot, "invalid previous key objectid, have %llu expect %llu",
                 prev_key->objectid, key->objectid);
    return false;
}

static u64 file_extent_end(struct extent_buffer *leaf, struct btrfs_key *key,
                           struct btrfs_file_extent_item *extent) {
    u64 end;
    u64 len;

    if (btrfs_file_extent_type(leaf, extent) == BTRFS_FILE_EXTENT_INLINE) {
        len = btrfs_file_extent_ram_bytes(leaf, extent);
        end = ALIGN(key->offset + len, leaf->fs_info->sectorsize);
    } else {
        len = btrfs_file_extent_num_bytes(leaf, extent);
        end = key->offset + len;
    }
    return end;
}

static int check_extent_data_item(struct extent_buffer *leaf, struct btrfs_key *key, int slot,
                                  struct btrfs_key *prev_key) {
    struct btrfs_fs_info *fs_info = leaf->fs_info;
    struct btrfs_file_extent_item *fi;
    u32 sectorsize = fs_info->sectorsize;
    u32 item_size = btrfs_item_size(leaf, slot);
    u64 extent_end;

    if (unlikely(!IS_ALIGNED(key->offset, sectorsize))) {
        file_extent_err(leaf, slot,
                        "unaligned file_offset for file extent, have %llu should be aligned to %u",
                        key->offset, sectorsize);
        return -EUCLEAN;
    }

    /*
     * Previous key must have the same key->objectid (ino).
     * It can be XATTR_ITEM, INODE_ITEM or just another EXTENT_DATA.
     * But if objectids mismatch, it means we have a missing
     * INODE_ITEM.
     */
    if (unlikely(!check_prev_ino(leaf, key, slot, prev_key)))
        return -EUCLEAN;

    fi = btrfs_item_ptr(leaf, slot, struct btrfs_file_extent_item);

    /*
     * Make sure the item contains at least inline header, so the file
     * extent type is not some garbage.
     */
    if (unlikely(item_size < BTRFS_FILE_EXTENT_INLINE_DATA_START)) {
        file_extent_err(leaf, slot, "invalid item size, have %u expect [%zu, %u)", item_size,
                        BTRFS_FILE_EXTENT_INLINE_DATA_START, SZ_4K);
        return -EUCLEAN;
    }
    if (unlikely(btrfs_file_extent_type(leaf, fi) >= BTRFS_NR_FILE_EXTENT_TYPES)) {
        file_extent_err(leaf, slot, "invalid type for file extent, have %u expect range [0, %u]",
                        btrfs_file_extent_type(leaf, fi), BTRFS_NR_FILE_EXTENT_TYPES - 1);
        return -EUCLEAN;
    }

    /*
     * Support for new compression/encryption must introduce incompat flag,
     * and must be caught in open_ctree().
     */
    if (unlikely(btrfs_file_extent_compression(leaf, fi) >= BTRFS_NR_COMPRESS_TYPES)) {
        file_extent_err(leaf, slot,
                        "invalid compression for file extent, have %u expect range [0, %u]",
                        btrfs_file_extent_compression(leaf, fi), BTRFS_NR_COMPRESS_TYPES - 1);
        return -EUCLEAN;
    }
    if (unlikely(btrfs_file_extent_encryption(leaf, fi))) {
        file_extent_err(leaf, slot, "invalid encryption for file extent, have %u expect 0",
                        btrfs_file_extent_encryption(leaf, fi));
        return -EUCLEAN;
    }
    if (btrfs_file_extent_type(leaf, fi) == BTRFS_FILE_EXTENT_INLINE) {
        /* Inline extent must have 0 as key offset */
        if (unlikely(key->offset)) {
            file_extent_err(leaf, slot,
                            "invalid file_offset for inline file extent, have %llu expect 0",
                            key->offset);
            return -EUCLEAN;
        }

        /* Compressed inline extent has no on-disk size, skip it */
        if (btrfs_file_extent_compression(leaf, fi) != BTRFS_COMPRESS_NONE)
            return 0;

        /* Uncompressed inline extent size must match item size */
        if (unlikely(item_size !=
                     BTRFS_FILE_EXTENT_INLINE_DATA_START + btrfs_file_extent_ram_bytes(leaf, fi))) {
            file_extent_err(
                leaf, slot, "invalid ram_bytes for uncompressed inline extent, have %u expect %llu",
                item_size,
                BTRFS_FILE_EXTENT_INLINE_DATA_START + btrfs_file_extent_ram_bytes(leaf, fi));
            return -EUCLEAN;
        }
        return 0;
    }

    /*
     * For the data reloc tree, file extent items are written by
     * relocation's own paths. The data reloc inode is created with
     * BTRFS_INODE_NOCOMPRESS, so insert_ordered_extent_file_extent()
     * always leaves the compression field at 0. Encryption and
     * other_encoding are reserved-and-zero in btrfs. A non-zero value
     * for any of these means the leaf decoded from disk does not match
     * what the kernel wrote, i.e. on-disk corruption.
     *
     * The file_extent_item's offset field is NOT a universal invariant
     * here: partial-PREALLOC writebacks legitimately produce REG items
     * with non-zero offset at non-boundary keys. The offset check is
     * performed at the call site in get_new_location(), which only
     * inspects cluster-boundary keys where offset is always 0.
     */
    if (unlikely(btrfs_header_owner(leaf) == BTRFS_DATA_RELOC_TREE_OBJECTID &&
                 (btrfs_file_extent_compression(leaf, fi) ||
                  btrfs_file_extent_encryption(leaf, fi) ||
                  btrfs_file_extent_other_encoding(leaf, fi)))) {
        file_extent_err(leaf, slot,
                        "invalid encoding fields for data reloc tree, compression=%u encryption=%u "
                        "other_encoding=%u",
                        btrfs_file_extent_compression(leaf, fi),
                        btrfs_file_extent_encryption(leaf, fi),
                        btrfs_file_extent_other_encoding(leaf, fi));
        return -EUCLEAN;
    }

    /* Regular or preallocated extent has fixed item size */
    if (unlikely(item_size != sizeof(*fi))) {
        file_extent_err(leaf, slot,
                        "invalid item size for reg/prealloc file extent, have %u expect %zu",
                        item_size, sizeof(*fi));
        return -EUCLEAN;
    }
    if (unlikely(CHECK_FE_ALIGNED(leaf, slot, fi, ram_bytes, sectorsize) ||
                 CHECK_FE_ALIGNED(leaf, slot, fi, disk_bytenr, sectorsize) ||
                 CHECK_FE_ALIGNED(leaf, slot, fi, disk_num_bytes, sectorsize) ||
                 CHECK_FE_ALIGNED(leaf, slot, fi, offset, sectorsize) ||
                 CHECK_FE_ALIGNED(leaf, slot, fi, num_bytes, sectorsize)))
        return -EUCLEAN;

    /* Catch extent end overflow */
    if (unlikely(
            check_add_overflow(btrfs_file_extent_num_bytes(leaf, fi), key->offset, &extent_end))) {
        file_extent_err(leaf, slot,
                        "extent end overflow, have file offset %llu extent num bytes %llu",
                        key->offset, btrfs_file_extent_num_bytes(leaf, fi));
        return -EUCLEAN;
    }

    /*
     * Check that no two consecutive file extent items, in the same leaf,
     * present ranges that overlap each other.
     */
    if (slot > 0 && prev_key->objectid == key->objectid &&
        prev_key->type == BTRFS_EXTENT_DATA_KEY) {
        struct btrfs_file_extent_item *prev_fi;
        u64 prev_end;

        prev_fi = btrfs_item_ptr(leaf, slot - 1, struct btrfs_file_extent_item);
        prev_end = file_extent_end(leaf, prev_key, prev_fi);
        if (unlikely(prev_end > key->offset)) {
            file_extent_err(leaf, slot - 1,
                            "file extent end range (%llu) goes beyond start offset (%llu) of the "
                            "next file extent",
                            prev_end, key->offset);
            return -EUCLEAN;
        }
    }

    /*
     * For non-compressed data extents, ram_bytes should match its
     * disk_num_bytes.
     * However we do not really utilize ram_bytes in this case, so this check
     * is only optional for DEBUG builds for developers to catch the
     * unexpected behaviors.
     */
    if (IS_ENABLED(CONFIG_BTRFS_DEBUG) &&
        btrfs_file_extent_compression(leaf, fi) == BTRFS_COMPRESS_NONE &&
        btrfs_file_extent_disk_bytenr(leaf, fi)) {
        if (WARN_ON(btrfs_file_extent_ram_bytes(leaf, fi) !=
                    btrfs_file_extent_disk_num_bytes(leaf, fi)))
            file_extent_err(
                leaf, slot,
                "mismatch ram_bytes (%llu) and disk_num_bytes (%llu) for non-compressed extent",
                btrfs_file_extent_ram_bytes(leaf, fi), btrfs_file_extent_disk_num_bytes(leaf, fi));
    }

    return 0;
}

static int check_csum_item(struct extent_buffer *leaf, struct btrfs_key *key, int slot,
                           struct btrfs_key *prev_key) {
    struct btrfs_fs_info *fs_info = leaf->fs_info;
    u32 sectorsize = fs_info->sectorsize;
    const u32 csumsize = fs_info->csum_size;

    if (unlikely(key->objectid != BTRFS_EXTENT_CSUM_OBJECTID)) {
        generic_err(leaf, slot, "invalid key objectid for csum item, have %llu expect %llu",
                    key->objectid, BTRFS_EXTENT_CSUM_OBJECTID);
        return -EUCLEAN;
    }
    if (unlikely(!IS_ALIGNED(key->offset, sectorsize))) {
        generic_err(leaf, slot,
                    "unaligned key offset for csum item, have %llu should be aligned to %u",
                    key->offset, sectorsize);
        return -EUCLEAN;
    }
    if (unlikely(!IS_ALIGNED(btrfs_item_size(leaf, slot), csumsize))) {
        generic_err(leaf, slot,
                    "unaligned item size for csum item, have %u should be aligned to %u",
                    btrfs_item_size(leaf, slot), csumsize);
        return -EUCLEAN;
    }
    if (slot > 0 && prev_key->type == BTRFS_EXTENT_CSUM_KEY) {
        u64 prev_csum_end;
        u32 prev_item_size;

        prev_item_size = btrfs_item_size(leaf, slot - 1);
        prev_csum_end = (prev_item_size / csumsize) * sectorsize;
        prev_csum_end += prev_key->offset;
        if (unlikely(prev_csum_end > key->offset)) {
            generic_err(
                leaf, slot - 1,
                "csum end range (%llu) goes beyond the start range (%llu) of the next csum item",
                prev_csum_end, key->offset);
            return -EUCLEAN;
        }
    }
    return 0;
}

static int check_inode_key(struct extent_buffer *leaf, struct btrfs_key *key, int slot) {
    struct btrfs_key item_key;
    bool is_inode_item;

    btrfs_item_key_to_cpu(leaf, &item_key, slot);
    is_inode_item = (item_key.type == BTRFS_INODE_ITEM_KEY);

    /* For XATTR_ITEM, location key should be all 0 */
    if (item_key.type == BTRFS_XATTR_ITEM_KEY) {
        if (unlikely(key->objectid != 0 || key->type != 0 || key->offset != 0))
            return -EUCLEAN;
        return 0;
    }

    if (unlikely((key->objectid < BTRFS_FIRST_FREE_OBJECTID ||
                  key->objectid > BTRFS_LAST_FREE_OBJECTID) &&
                 key->objectid != BTRFS_ROOT_TREE_DIR_OBJECTID &&
                 key->objectid != BTRFS_FREE_INO_OBJECTID)) {
        if (is_inode_item) {
            generic_err(leaf, slot,
                        "invalid key objectid: has %llu expect %llu or [%llu, %llu] or %llu",
                        key->objectid, BTRFS_ROOT_TREE_DIR_OBJECTID, BTRFS_FIRST_FREE_OBJECTID,
                        BTRFS_LAST_FREE_OBJECTID, BTRFS_FREE_INO_OBJECTID);
        } else {
            dir_item_err(
                leaf, slot,
                "invalid location key objectid: has %llu expect %llu or [%llu, %llu] or %llu",
                key->objectid, BTRFS_ROOT_TREE_DIR_OBJECTID, BTRFS_FIRST_FREE_OBJECTID,
                BTRFS_LAST_FREE_OBJECTID, BTRFS_FREE_INO_OBJECTID);
        }
        return -EUCLEAN;
    }
    if (unlikely(key->offset != 0)) {
        if (is_inode_item)
            inode_item_err(leaf, slot, "invalid key offset: has %llu expect 0", key->offset);
        else
            dir_item_err(leaf, slot, "invalid location key offset:has %llu expect 0", key->offset);
        return -EUCLEAN;
    }
    return 0;
}

static int check_root_key(struct extent_buffer *leaf, struct btrfs_key *key, int slot) {
    struct btrfs_key item_key;
    bool is_root_item;

    btrfs_item_key_to_cpu(leaf, &item_key, slot);
    is_root_item = (item_key.type == BTRFS_ROOT_ITEM_KEY);

    /*
     * Bad rootid for reloc trees.
     *
     * Reloc trees are only for subvolume trees, other trees only need
     * to be COWed to be relocated.
     */
    if (unlikely(is_root_item && key->objectid == BTRFS_TREE_RELOC_OBJECTID &&
                 !btrfs_is_fstree(key->offset))) {
        generic_err(leaf, slot, "invalid reloc tree for root %lld, root id is not a subvolume tree",
                    key->offset);
        return -EUCLEAN;
    }

    /* No such tree id */
    if (unlikely(key->objectid == 0)) {
        if (is_root_item)
            generic_err(leaf, slot, "invalid root id 0");
        else
            dir_item_err(leaf, slot, "invalid location key root id 0");
        return -EUCLEAN;
    }

    /* DIR_ITEM/INDEX/INODE_REF is not allowed to point to non-fs trees */
    if (unlikely(!btrfs_is_fstree(key->objectid) && !is_root_item)) {
        dir_item_err(leaf, slot, "invalid location key objectid, have %llu expect [%llu, %llu]",
                     key->objectid, BTRFS_FIRST_FREE_OBJECTID, BTRFS_LAST_FREE_OBJECTID);
        return -EUCLEAN;
    }

    /*
     * ROOT_ITEM with non-zero offset means this is a snapshot, created at
     * @offset transid.
     * Furthermore, for location key in DIR_ITEM, its offset is always -1.
     *
     * So here we only check offset for reloc tree whose key->offset must
     * be a valid tree.
     */
    if (unlikely(key->objectid == BTRFS_TREE_RELOC_OBJECTID && key->offset == 0)) {
        generic_err(leaf, slot, "invalid root id 0 for reloc tree");
        return -EUCLEAN;
    }
    return 0;
}

static int check_dir_item(struct extent_buffer *leaf, struct btrfs_key *key,
                          struct btrfs_key *prev_key, int slot) {
    struct btrfs_fs_info *fs_info = leaf->fs_info;
    struct btrfs_dir_item *di;
    u32 item_size = btrfs_item_size(leaf, slot);
    u32 cur = 0;

    if (unlikely(!check_prev_ino(leaf, key, slot, prev_key)))
        return -EUCLEAN;

    di = btrfs_item_ptr(leaf, slot, struct btrfs_dir_item);
    while (cur < item_size) {
        struct btrfs_key location_key;
        u32 name_len;
        u32 data_len;
        u32 max_name_len;
        u32 total_size;
        u32 name_hash;
        u8 dir_type;
        int ret;

        /* header itself should not cross item boundary */
        if (unlikely(cur + sizeof(*di) > item_size)) {
            dir_item_err(leaf, slot, "dir item header crosses item boundary, have %zu boundary %u",
                         cur + sizeof(*di), item_size);
            return -EUCLEAN;
        }

        /* Location key check */
        btrfs_dir_item_key_to_cpu(leaf, di, &location_key);
        if (location_key.type == BTRFS_ROOT_ITEM_KEY) {
            ret = check_root_key(leaf, &location_key, slot);
            if (unlikely(ret < 0))
                return ret;
        } else if (location_key.type == BTRFS_INODE_ITEM_KEY || location_key.type == 0) {
            ret = check_inode_key(leaf, &location_key, slot);
            if (unlikely(ret < 0))
                return ret;
        } else {
            dir_item_err(leaf, slot, "invalid location key type, have %u, expect %u or %u",
                         location_key.type, BTRFS_ROOT_ITEM_KEY, BTRFS_INODE_ITEM_KEY);
            return -EUCLEAN;
        }

        /* dir type check */
        dir_type = btrfs_dir_ftype(leaf, di);
        if (unlikely(dir_type <= BTRFS_FT_UNKNOWN || dir_type >= BTRFS_FT_MAX)) {
            dir_item_err(leaf, slot, "invalid dir item type, have %u expect (0, %u)", dir_type,
                         BTRFS_FT_MAX);
            return -EUCLEAN;
        }

        if (unlikely(key->type == BTRFS_XATTR_ITEM_KEY && dir_type != BTRFS_FT_XATTR)) {
            dir_item_err(leaf, slot, "invalid dir item type for XATTR key, have %u expect %u",
                         dir_type, BTRFS_FT_XATTR);
            return -EUCLEAN;
        }
        if (unlikely(dir_type == BTRFS_FT_XATTR && key->type != BTRFS_XATTR_ITEM_KEY)) {
            dir_item_err(leaf, slot, "xattr dir type found for non-XATTR key");
            return -EUCLEAN;
        }
        if (dir_type == BTRFS_FT_XATTR)
            max_name_len = XATTR_NAME_MAX;
        else
            max_name_len = BTRFS_NAME_LEN;

        /* Name/data length check */
        name_len = btrfs_dir_name_len(leaf, di);
        data_len = btrfs_dir_data_len(leaf, di);
        if (unlikely(name_len > max_name_len)) {
            dir_item_err(leaf, slot, "dir item name len too long, have %u max %u", name_len,
                         max_name_len);
            return -EUCLEAN;
        }
        if (unlikely(name_len + data_len > BTRFS_MAX_XATTR_SIZE(fs_info))) {
            dir_item_err(leaf, slot, "dir item name and data len too long, have %u max %u",
                         name_len + data_len, BTRFS_MAX_XATTR_SIZE(fs_info));
            return -EUCLEAN;
        }

        if (unlikely(data_len && dir_type != BTRFS_FT_XATTR)) {
            dir_item_err(leaf, slot, "dir item with invalid data len, have %u expect 0", data_len);
            return -EUCLEAN;
        }

        total_size = sizeof(*di) + name_len + data_len;

        /* header and name/data should not cross item boundary */
        if (unlikely(cur + total_size > item_size)) {
            dir_item_err(leaf, slot, "dir item data crosses item boundary, have %u boundary %u",
                         cur + total_size, item_size);
            return -EUCLEAN;
        }

        /*
         * Special check for XATTR/DIR_ITEM, as key->offset is name
         * hash, should match its name
         */
        if (key->type == BTRFS_DIR_ITEM_KEY || key->type == BTRFS_XATTR_ITEM_KEY) {
            char namebuf[MAX(BTRFS_NAME_LEN, XATTR_NAME_MAX)];

            read_extent_buffer(leaf, namebuf, (size_t)(uintptr_t)(di + 1), name_len);
            name_hash = btrfs_name_hash(namebuf, name_len);
            if (unlikely(key->offset != name_hash)) {
                dir_item_err(leaf, slot,
                             "name hash mismatch with key, have 0x%016x expect 0x%016llx",
                             name_hash, key->offset);
                return -EUCLEAN;
            }
        }
        cur += total_size;
        di = (struct btrfs_dir_item *)((uintptr_t)di + total_size);
    }
    return 0;
}

static int check_inode_item(struct extent_buffer *leaf, struct btrfs_key *key, int slot) {
    struct btrfs_fs_info *fs_info = leaf->fs_info;
    struct btrfs_inode_item *iitem;
    u64 super_gen = btrfs_super_generation(&fs_info->super_copy);
    u32 valid_mask = (S_IFMT | S_ISUID | S_ISGID | S_ISVTX | 0777);
    const u32 item_size = btrfs_item_size(leaf, slot);
    u32 mode;
    int ret;
    u32 flags;
    u32 ro_flags;

    ret = check_inode_key(leaf, key, slot);
    if (unlikely(ret < 0))
        return ret;

    if (unlikely(item_size != sizeof(*iitem))) {
        generic_err(leaf, slot, "invalid item size: has %u expect %zu", item_size, sizeof(*iitem));
        return -EUCLEAN;
    }

    iitem = btrfs_item_ptr(leaf, slot, struct btrfs_inode_item);

    /* Here we use super block generation + 1 to handle log tree */
    if (unlikely(btrfs_inode_generation(leaf, iitem) > super_gen + 1)) {
        inode_item_err(leaf, slot, "invalid inode generation: has %llu expect (0, %llu]",
                       btrfs_inode_generation(leaf, iitem), super_gen + 1);
        return -EUCLEAN;
    }
    /* Note for ROOT_TREE_DIR_ITEM, mkfs could set its transid 0 */
    if (unlikely(btrfs_inode_transid(leaf, iitem) > super_gen + 1)) {
        inode_item_err(leaf, slot, "invalid inode transid: has %llu expect [0, %llu]",
                       btrfs_inode_transid(leaf, iitem), super_gen + 1);
        return -EUCLEAN;
    }

    /*
     * For size and nbytes it's better not to be too strict, as for dir
     * item its size/nbytes can easily get wrong, but doesn't affect
     * anything in the fs. So here we skip the check.
     */
    mode = btrfs_inode_mode(leaf, iitem);
    if (unlikely(mode & ~valid_mask)) {
        inode_item_err(leaf, slot, "unknown mode bit detected: 0x%x", mode & ~valid_mask);
        return -EUCLEAN;
    }

    /*
     * S_IFMT is not bit mapped so we can't completely rely on
     * is_power_of_2/has_single_bit_set, but it can save us from checking
     * FIFO/CHR/DIR/REG.  Only needs to check BLK, LNK and SOCKS
     */
    if (!has_single_bit_set(mode & S_IFMT)) {
        if (unlikely(!S_ISLNK(mode) && !S_ISBLK(mode) && !S_ISSOCK(mode))) {
            inode_item_err(leaf, slot, "invalid mode: has 0%o expect valid S_IF* bit(s)",
                           mode & S_IFMT);
            return -EUCLEAN;
        }
    }
    if (unlikely(S_ISDIR(mode) && btrfs_inode_nlink(leaf, iitem) > 1)) {
        inode_item_err(leaf, slot, "invalid nlink: has %u expect no more than 1 for dir",
                       btrfs_inode_nlink(leaf, iitem));
        return -EUCLEAN;
    }
    btrfs_inode_split_flags(btrfs_inode_flags(leaf, iitem), &flags, &ro_flags);
    if (unlikely(flags & ~BTRFS_INODE_FLAG_MASK)) {
        inode_item_err(leaf, slot, "unknown incompat flags detected: 0x%x", flags);
        return -EUCLEAN;
    }
    if (unlikely(!sb_rdonly(fs_info->sb) && (ro_flags & ~BTRFS_INODE_RO_FLAG_MASK))) {
        inode_item_err(leaf, slot, "unknown ro-compat flags detected on writeable mount: 0x%x",
                       ro_flags);
        return -EUCLEAN;
    }
    return 0;
}

static int check_root_item(struct extent_buffer *leaf, struct btrfs_key *key, int slot) {
    struct btrfs_fs_info *fs_info = leaf->fs_info;
    struct btrfs_root_item ri = {0};
    const u64 valid_root_flags = BTRFS_ROOT_SUBVOL_RDONLY | BTRFS_ROOT_SUBVOL_DEAD;
    int ret;

    ret = check_root_key(leaf, key, slot);
    if (unlikely(ret < 0))
        return ret;

    if (unlikely(btrfs_item_size(leaf, slot) != sizeof(ri) &&
                 btrfs_item_size(leaf, slot) != btrfs_legacy_root_item_size())) {
        generic_err(leaf, slot, "invalid root item size, have %u expect %zu or %u",
                    btrfs_item_size(leaf, slot), sizeof(ri), btrfs_legacy_root_item_size());
        return -EUCLEAN;
    }

    /*
     * For legacy root item, the members starting at generation_v2 will be
     * all filled with 0.
     * And since we allow generation_v2 as 0, it will still pass the check.
     */
    read_extent_buffer(leaf, &ri, btrfs_item_ptr_offset(leaf, slot), btrfs_item_size(leaf, slot));

    /* Generation related */
    if (unlikely(btrfs_root_generation(&ri) > btrfs_super_generation(&fs_info->super_copy) + 1)) {
        generic_err(leaf, slot, "invalid root generation, have %llu expect (0, %llu]",
                    btrfs_root_generation(&ri), btrfs_super_generation(&fs_info->super_copy) + 1);
        return -EUCLEAN;
    }
    if (unlikely(btrfs_root_generation_v2(&ri) >
                 btrfs_super_generation(&fs_info->super_copy) + 1)) {
        generic_err(leaf, slot, "invalid root v2 generation, have %llu expect (0, %llu]",
                    btrfs_root_generation_v2(&ri),
                    btrfs_super_generation(&fs_info->super_copy) + 1);
        return -EUCLEAN;
    }
    if (unlikely(btrfs_root_last_snapshot(&ri) >
                 btrfs_super_generation(&fs_info->super_copy) + 1)) {
        generic_err(leaf, slot, "invalid root last_snapshot, have %llu expect (0, %llu]",
                    btrfs_root_last_snapshot(&ri),
                    btrfs_super_generation(&fs_info->super_copy) + 1);
        return -EUCLEAN;
    }

    /* Alignment and level check */
    if (unlikely(!IS_ALIGNED(btrfs_root_bytenr(&ri), fs_info->sectorsize))) {
        generic_err(leaf, slot, "invalid root bytenr, have %llu expect to be aligned to %u",
                    btrfs_root_bytenr(&ri), fs_info->sectorsize);
        return -EUCLEAN;
    }
    if (unlikely(btrfs_root_level(&ri) >= BTRFS_MAX_LEVEL)) {
        generic_err(leaf, slot, "invalid root level, have %u expect [0, %u]", btrfs_root_level(&ri),
                    BTRFS_MAX_LEVEL - 1);
        return -EUCLEAN;
    }
    if (unlikely(btrfs_root_drop_level(&ri) >= BTRFS_MAX_LEVEL)) {
        generic_err(leaf, slot, "invalid root drop_level, have %u expect [0, %u]",
                    btrfs_root_drop_level(&ri), BTRFS_MAX_LEVEL - 1);
        return -EUCLEAN;
    }
    /*
     * If drop_progress.objectid is non-zero, a btrfs_drop_snapshot() was
     * interrupted and the resume point was recorded in drop_progress and
     * drop_level.  In that case drop_level must be >= 1: level 0 is the
     * leaf level and drop_snapshot never saves a checkpoint there (it
     * only records checkpoints at internal node levels in DROP_REFERENCE
     * stage).  A zero drop_level combined with a non-zero drop_progress
     * objectid indicates on-disk corruption and would cause a BUG_ON in
     * merge_reloc_root() and btrfs_drop_snapshot() at mount time.
     */
    if (unlikely(btrfs_disk_key_objectid(&ri.drop_progress) != 0 &&
                 btrfs_root_drop_level(&ri) == 0)) {
        generic_err(leaf, slot,
                    "invalid root drop_level 0 with non-zero drop_progress objectid %llu",
                    btrfs_disk_key_objectid(&ri.drop_progress));
        return -EUCLEAN;
    }

    /* Flags check */
    if (unlikely(btrfs_root_flags(&ri) & ~valid_root_flags)) {
        generic_err(leaf, slot, "invalid root flags, have 0x%llx expect mask 0x%llx",
                    btrfs_root_flags(&ri), valid_root_flags);
        return -EUCLEAN;
    }
    return 0;
}

static int check_leaf_item_read(struct extent_buffer *leaf, struct btrfs_key *key, int slot,
                                struct btrfs_key *prev) {
    switch (key->type) {
    case BTRFS_EXTENT_DATA_KEY:
        return check_extent_data_item(leaf, key, slot, prev);
    case BTRFS_EXTENT_CSUM_KEY:
        return check_csum_item(leaf, key, slot, prev);
    case BTRFS_INODE_ITEM_KEY:
        return check_inode_item(leaf, key, slot);
    case BTRFS_ROOT_ITEM_KEY:
        return check_root_item(leaf, key, slot);
    case BTRFS_DIR_ITEM_KEY:
    case BTRFS_DIR_INDEX_KEY:
    case BTRFS_XATTR_ITEM_KEY:
        return check_dir_item(leaf, key, prev, slot);
    default:
        return 0;
    }
}

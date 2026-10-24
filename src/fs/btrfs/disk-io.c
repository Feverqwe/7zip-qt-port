// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2007 Oracle. All rights reserved. */
/* Linux superblock and tree-buffer validation with stream-backed buffers. */
#include "volumes.h"
#define BTRFS_SUPER_FLAG_SUPP                                                                      \
    (BTRFS_HEADER_FLAG_WRITTEN | BTRFS_HEADER_FLAG_RELOC | BTRFS_SUPER_FLAG_ERROR |                \
     BTRFS_SUPER_FLAG_SEEDING | BTRFS_SUPER_FLAG_METADUMP | BTRFS_SUPER_FLAG_METADUMP_V2)
static bool btrfs_supported_blocksize(u32 n) {
    /* Contiguous userspace buffers have no host-page-size restriction. */
    return is_power_of_2(n) && n >= BTRFS_MIN_BLOCKSIZE && n <= BTRFS_MAX_METADATA_BLOCKSIZE;
}
static int validate_sys_chunk_array(const struct btrfs_fs_info *fs_info,
                                    const struct btrfs_super_block *sb) {
    unsigned int cur = 0; /* Offset inside the sys chunk array */
    /*
     * At sb read time, fs_info is not fully initialized. Thus we have
     * to use super block sectorsize, which should have been validated.
     */
    const u32 sectorsize = btrfs_super_sectorsize(sb);
    u32 sys_array_size = btrfs_super_sys_array_size(sb);

    if (unlikely(sys_array_size > BTRFS_SYSTEM_CHUNK_ARRAY_SIZE)) {
        (void)0;
        return -EUCLEAN;
    }

    /* It must hold at least one key and one chunk. */
    if (unlikely(sys_array_size < sizeof(struct btrfs_disk_key) + sizeof(struct btrfs_chunk))) {
        (void)0;
        return -EUCLEAN;
    }

    while (cur < sys_array_size) {
        struct btrfs_disk_key *disk_key;
        struct btrfs_chunk *chunk;
        struct btrfs_key key;
        u64 type;
        u16 num_stripes;
        u32 len;
        int ret;

        disk_key = (struct btrfs_disk_key *)(sb->sys_chunk_array + cur);
        len = sizeof(*disk_key);

        if (unlikely(cur + len > sys_array_size))
            goto short_read;
        cur += len;

        btrfs_disk_key_to_cpu(&key, disk_key);
        if (unlikely(key.type != BTRFS_CHUNK_ITEM_KEY)) {
            (void)0;
            return -EUCLEAN;
        }

        if (unlikely(cur + sizeof(*chunk) > sys_array_size))
            goto short_read;

        chunk = (struct btrfs_chunk *)(sb->sys_chunk_array + cur);
        num_stripes = btrfs_stack_chunk_num_stripes(chunk);
        if (unlikely(cur + btrfs_chunk_item_size(num_stripes) > sys_array_size))
            goto short_read;
        type = btrfs_stack_chunk_type(chunk);
        if (unlikely(!(type & BTRFS_BLOCK_GROUP_SYSTEM))) {
            (void)0;
            return -EUCLEAN;
        }
        ret = btrfs_check_chunk_valid(fs_info, NULL, chunk, key.offset, sectorsize);
        if (ret < 0)
            return ret;
        cur += btrfs_chunk_item_size(num_stripes);
    }
    return 0;
short_read:
    (void)0;
    return -EUCLEAN;
}

int btrfs_validate_super(const struct btrfs_fs_info *fs_info, const struct btrfs_super_block *sb,
                         int mirror_num) {
    const u32 nodesize = btrfs_super_nodesize(sb);
    const u32 sectorsize = btrfs_super_sectorsize(sb);
    int ret = 0;
    const bool ignore_flags = false;

    if (unlikely(btrfs_super_magic(sb) != BTRFS_MAGIC)) {
        (void)0;
        ret = -EINVAL;
    }
    if (unlikely(btrfs_super_flags(sb) & ~BTRFS_SUPER_FLAG_SUPP)) {
        if (!ignore_flags) {
            (void)0;
            ret = -EINVAL;
        } else {
            (void)0;
        }
    }
    if (unlikely(btrfs_super_root_level(sb) >= BTRFS_MAX_LEVEL)) {
        (void)0;
        ret = -EINVAL;
    }
    if (unlikely(btrfs_super_chunk_root_level(sb) >= BTRFS_MAX_LEVEL)) {
        (void)0;
        ret = -EINVAL;
    }
    if (unlikely(btrfs_super_log_root_level(sb) >= BTRFS_MAX_LEVEL)) {
        (void)0;
        ret = -EINVAL;
    }

    /*
     * Check sectorsize and nodesize first, other check will need it.
     * Check all possible sectorsize(4K, 8K, 16K, 32K, 64K) here.
     */
    if (unlikely(!is_power_of_2(sectorsize) || sectorsize < BTRFS_MIN_BLOCKSIZE ||
                 sectorsize > BTRFS_MAX_METADATA_BLOCKSIZE)) {
        (void)0;
        ret = -EINVAL;
    }

    if (unlikely(!btrfs_supported_blocksize(sectorsize))) {
        (void)0;
        ret = -EINVAL;
    }

    if (unlikely(!is_power_of_2(nodesize) || nodesize < sectorsize ||
                 nodesize > BTRFS_MAX_METADATA_BLOCKSIZE)) {
        (void)0;
        ret = -EINVAL;
    }
    if (unlikely(nodesize != get_unaligned_le32(&sb->__unused_leafsize))) {
        (void)0;
        ret = -EINVAL;
    }

    /* Root alignment check */
    if (unlikely(!IS_ALIGNED(btrfs_super_root(sb), sectorsize))) {
        (void)0;
        ret = -EINVAL;
    }
    if (unlikely(!IS_ALIGNED(btrfs_super_chunk_root(sb), sectorsize))) {
        (void)0;
        ret = -EINVAL;
    }
    if (unlikely(!IS_ALIGNED(btrfs_super_log_root(sb), sectorsize))) {
        (void)0;
        ret = -EINVAL;
    }

    if (unlikely(!false && memcmp(fs_info->super_copy.fsid, sb->fsid, BTRFS_FSID_SIZE) != 0)) {
        (void)0;
        ret = -EINVAL;
    }

    if (unlikely(memcmp(fs_info->metadata_uuid, btrfs_sb_fsid_ptr(sb), BTRFS_FSID_SIZE) != 0)) {
        (void)0;
        ret = -EINVAL;
    }

    if (unlikely(memcmp(fs_info->metadata_uuid, sb->dev_item.fsid, BTRFS_FSID_SIZE) != 0)) {
        (void)0;
        ret = -EINVAL;
    }

    /*
     * Artificial requirement for block-group-tree to force newer features
     * (free-space-tree, no-holes) so the test matrix is smaller.
     */
    if (unlikely(btrfs_fs_compat_ro(fs_info, BLOCK_GROUP_TREE) &&
                 (!btrfs_fs_compat_ro(fs_info, FREE_SPACE_TREE_VALID) ||
                  !btrfs_fs_incompat(fs_info, NO_HOLES)))) {
        (void)0;
        ret = -EINVAL;
    }

    if (btrfs_fs_incompat(fs_info, REMAP_TREE)) {
        /*
         * Reduce test matrix for remap tree by requiring block-group-tree
         * and no-holes. Free-space-tree is a hard requirement.
         */
        if (unlikely(!btrfs_fs_compat_ro(fs_info, FREE_SPACE_TREE_VALID) ||
                     !btrfs_fs_incompat(fs_info, NO_HOLES) ||
                     !btrfs_fs_compat_ro(fs_info, BLOCK_GROUP_TREE))) {
            (void)0;
            ret = -EINVAL;
        }

        if (unlikely(btrfs_fs_incompat(fs_info, MIXED_GROUPS))) {
            (void)0;
            ret = -EINVAL;
        }

        if (unlikely(btrfs_fs_incompat(fs_info, ZONED))) {
            (void)0;
            ret = -EINVAL;
        }

        if (unlikely(sectorsize > BTRFS_MAX_METADATA_BLOCKSIZE)) {
            (void)0;
            ret = -EINVAL;
        }
    }

    /*
     * Hint to catch really bogus numbers, bitflips or so, more exact checks are
     * done later
     */
    if (unlikely(btrfs_super_bytes_used(sb) < 6 * btrfs_super_nodesize(sb))) {
        (void)0;
        ret = -EINVAL;
    }
    if (unlikely(!is_power_of_2(btrfs_super_stripesize(sb)))) {
        (void)0;
        ret = -EINVAL;
    }
    if (unlikely(btrfs_super_num_devices(sb) > (1UL << 31)))
        (void)0;
    if (unlikely(btrfs_super_num_devices(sb) == 0)) {
        (void)0;
        ret = -EINVAL;
    }

    if (unlikely(mirror_num >= 0 && btrfs_super_bytenr(sb) != btrfs_sb_offset(mirror_num))) {
        (void)0;
        ret = -EINVAL;
    }

    if (unlikely(ret))
        return ret;

    ret = validate_sys_chunk_array(fs_info, sb);

    /*
     * The generation is a global counter, we'll trust it more than the others
     * but it's still possible that it's the one that's wrong.
     */
    if (unlikely(btrfs_super_generation(sb) < btrfs_super_chunk_root_generation(sb)))
        (void)0;
    if (unlikely(btrfs_super_generation(sb) < btrfs_super_cache_generation(sb) &&
                 btrfs_super_cache_generation(sb) != (u64)-1))
        (void)0;

    return ret;
}

int btrfs_check_super_csum(struct btrfs_fs_info *fs_info, const struct btrfs_super_block *disk_sb) {
    u8 result[BTRFS_CSUM_SIZE] = {0};
    if (!fs_info->source.checksum)
        return -EOPNOTSUPP;
    int ret = fs_info->source.checksum(fs_info->csum_type, (const u8 *)disk_sb + BTRFS_CSUM_SIZE,
                                       BTRFS_SUPER_INFO_SIZE - BTRFS_CSUM_SIZE, result);
    if (ret)
        return ret;
    return memcmp(disk_sb->csum, result, fs_info->csum_size) ? -EUCLEAN : 0;
}

/* Linux btrfs_validate_extent_buffer, with contiguous-buffer checksum service. */
int btrfs_validate_extent_buffer(struct extent_buffer *eb,
                                 const struct btrfs_tree_parent_check *check) {
    struct btrfs_fs_info *fs_info = eb->fs_info;
    u8 result[BTRFS_CSUM_SIZE] = {0};
    const u8 level = btrfs_header_level(eb);
    int ret;
    if (btrfs_header_bytenr(eb) != eb->start ||
        memcmp(eb->data + offsetof(struct btrfs_header, fsid), fs_info->metadata_uuid,
               BTRFS_FSID_SIZE) ||
        level >= BTRFS_MAX_LEVEL || level != check->level ||
        (check->transid && btrfs_header_generation(eb) != check->transid))
        return -EUCLEAN;
    if (!fs_info->source.checksum)
        return -EOPNOTSUPP;
    ret = fs_info->source.checksum(fs_info->csum_type, eb->data + BTRFS_CSUM_SIZE,
                                   eb->len - BTRFS_CSUM_SIZE, result);
    if (ret)
        return ret;
    if (memcmp(result, eb->data, fs_info->csum_size))
        return -EUCLEAN;
    ret = level ? btrfs_check_node(eb) : btrfs_check_leaf(eb);
    if (ret)
        return ret;
    if (check->has_first_key) {
        struct btrfs_key found_key;
        if (!btrfs_header_nritems(eb))
            return -EUCLEAN;
        if (level)
            btrfs_node_key_to_cpu(eb, &found_key, 0);
        else
            btrfs_item_key_to_cpu(eb, &found_key, 0);
        if (btrfs_comp_cpu_keys(&check->first_key, &found_key))
            return -EUCLEAN;
    }
    return btrfs_check_eb_owner(eb, check->owner_root);
}

/* Linux btrfs_read_tree_root/read_tree_root_path without VFS root allocation
 * or root caches. The caller owns the compact root; traversal paths own buffers.
 * Retain the exact root-node owner check, distinct from shared descendant nodes.
 * btrfs_get_root_ref's live-root reference check follows the completed read. */
int btrfs_read_tree_root(struct btrfs_fs_info *info, u64 id, struct btrfs_root *root) {
    struct btrfs_path path = {0};
    struct btrfs_key key = {id, BTRFS_ROOT_ITEM_KEY, UINT64_MAX};
    struct btrfs_root_item item;
    struct btrfs_tree_parent_check check = {0};
    int ret = btrfs_find_root(&info->tree_root, &key, &path, &item, NULL);
    if (ret)
        return ret > 0 ? -ENOENT : ret;
    *root = (struct btrfs_root){info, btrfs_root_bytenr(&item), btrfs_root_generation(&item),
                                id,   btrfs_root_level(&item),  btrfs_root_dirid(&item)};
    check.level = root->level;
    check.transid = root->generation;
    check.owner_root = id;
    struct extent_buffer *node = read_tree_block(info, root->bytenr, &check);
    if (IS_ERR(node))
        return PTR_ERR(node);
    if (id != BTRFS_TREE_LOG_OBJECTID && id != BTRFS_TREE_RELOC_OBJECTID &&
        id != btrfs_header_owner(node))
        ret = -EUCLEAN;
    free_extent_buffer(node);
    if (!ret && !btrfs_root_refs(&item))
        ret = -ENOENT;
    return ret;
}

u64 btrfs_sb_offset(int mirror) {
    u64 start = 16384;
    if (mirror)
        return start << (12 * mirror);
    return BTRFS_SUPER_INFO_OFFSET;
}

/* Stream adapter for Linux read_tree_block: no block repair is performed. */
struct extent_buffer *read_tree_block(struct btrfs_fs_info *fs_info, u64 bytenr,
                                      const struct btrfs_tree_parent_check *check) {
    if (!check || !bytenr || !IS_ALIGNED(bytenr, fs_info->sectorsize))
        return ERR_PTR(-EUCLEAN);
    struct extent_buffer *eb = calloc(1, sizeof(*eb));
    if (!eb)
        return ERR_PTR(-ENOMEM);
    eb->data = malloc(fs_info->nodesize);
    if (!eb->data) {
        free(eb);
        return ERR_PTR(-ENOMEM);
    }
    eb->fs_info = fs_info;
    eb->start = bytenr;
    eb->len = fs_info->nodesize;
    int ret = -EIO;
    const unsigned int copies = btrfs_num_copies(fs_info, bytenr);
    for (unsigned int mirror = 1; mirror <= copies; mirror++) {
        ret = btrfs_read_logical(fs_info, bytenr, eb->data, eb->len, mirror);
        if (!ret)
            ret = btrfs_validate_extent_buffer(eb, check);
        if (!ret)
            return eb;
        if (ret == -ECANCELED || ret == -ENOMEM)
            break;
    }
    free_extent_buffer(eb);
    return ERR_PTR(ret);
}

void free_extent_buffer(struct extent_buffer *eb) {
    if (eb) {
        free(eb->data);
        free(eb);
    }
}

void read_extent_buffer(const struct extent_buffer *eb, void *dest, size_t start, size_t len) {
    /* Tree block and item validation precedes all offset-based access. */
    memcpy(dest, eb->data + start, len);
}

// SPDX-License-Identifier: GPL-2.0
// Stream and lifetime services for the Linux Btrfs read-side modules.
#include "formats/btrfs/btrfsreader.h"
#include "formats/btrfs/extent_map.h"
#include "formats/btrfs/volumes.h"

const char *btrfs_read_limitation(uint64_t incompat) {
    const u64 supported =
        BTRFS_FEATURE_INCOMPAT_MIXED_BACKREF | BTRFS_FEATURE_INCOMPAT_DEFAULT_SUBVOL |
        BTRFS_FEATURE_INCOMPAT_MIXED_GROUPS | BTRFS_FEATURE_INCOMPAT_COMPRESS_LZO |
        BTRFS_FEATURE_INCOMPAT_COMPRESS_ZSTD | BTRFS_FEATURE_INCOMPAT_BIG_METADATA |
        BTRFS_FEATURE_INCOMPAT_EXTENDED_IREF | BTRFS_FEATURE_INCOMPAT_RAID56 |
        BTRFS_FEATURE_INCOMPAT_SKINNY_METADATA | BTRFS_FEATURE_INCOMPAT_NO_HOLES |
        BTRFS_FEATURE_INCOMPAT_METADATA_UUID | BTRFS_FEATURE_INCOMPAT_RAID1C34 |
        BTRFS_FEATURE_INCOMPAT_SIMPLE_QUOTA;
    if (incompat & BTRFS_FEATURE_INCOMPAT_REMAP_TREE)
        return "This Btrfs volume uses a remap tree, which this reader cannot resolve.";
    if (incompat & BTRFS_FEATURE_INCOMPAT_RAID_STRIPE_TREE)
        return "This Btrfs volume uses a RAID stripe tree, which this reader cannot resolve.";
    if (incompat & BTRFS_FEATURE_INCOMPAT_ZONED)
        return "This Btrfs volume requires zoned superblock-log access, which this reader does not "
               "support.";
    if (incompat & BTRFS_FEATURE_INCOMPAT_EXTENT_TREE_V2)
        return "This Btrfs volume uses extent-tree-v2, which this reader does not support.";
    return incompat & ~supported ? "This Btrfs volume uses an unknown incompatible feature." : NULL;
}

int btrfs_inspect_device(const void *data, size_t size, struct btrfs_device_info *result) {
    if (!data || !result || size != BTRFS_SUPER_INFO_SIZE)
        return -EINVAL;
    struct btrfs_fs_info info = {0};
    memcpy(&info.super_copy, data, size);
    const struct btrfs_super_block *sb = &info.super_copy;
    if (btrfs_super_magic(sb) != BTRFS_MAGIC)
        return -EINVAL;
    info.csum_type = btrfs_super_csum_type(sb);
    info.csum_size = btrfs_csum_type_size(info.csum_type);
    if (!info.csum_size)
        return -EOPNOTSUPP;
    info.source.checksum = btrfs_checksum_native;
    memcpy(info.metadata_uuid, btrfs_sb_fsid_ptr(sb), BTRFS_FSID_SIZE);
    int ret = btrfs_check_super_csum(&info, sb);
    if (!ret)
        ret = btrfs_validate_super(&info, sb, 0);
    if (ret)
        return ret;
    result->devid = get_unaligned_le64(&sb->dev_item.devid);
    result->bytes = get_unaligned_le64(&sb->dev_item.total_bytes);
    result->incompat = btrfs_super_incompat_flags(sb);
    memcpy(result->uuid, sb->dev_item.uuid, BTRFS_UUID_SIZE);
    memcpy(result->fsid, sb->fsid, BTRFS_FSID_SIZE);
    memcpy(result->metadata_uuid, info.metadata_uuid, BTRFS_FSID_SIZE);
    return result->devid ? 0 : -EUCLEAN;
}

int btrfs_poll(struct btrfs_fs_info *fs_info) {
    return fs_info->source.poll ? fs_info->source.poll(fs_info->source.context) : 0;
}

int btrfs_read_physical(struct btrfs_fs_info *fs_info, u64 devid, const u8 *uuid, u64 offset,
                        void *data, size_t size) {
    if (size > UINT64_MAX - offset)
        return -EIO;
    if (!devid || devid == fs_info->primary_devid) {
        if (devid && memcmp(uuid, fs_info->primary_uuid, BTRFS_UUID_SIZE))
            return -EUCLEAN;
        if (offset > fs_info->source.size || size > fs_info->source.size - offset)
            return -EIO;
    } else {
        if (!fs_info->source.open_device)
            return -ENODEV;
        int ret = fs_info->source.open_device(fs_info->source.context, devid, uuid);
        if (ret)
            return ret;
    }
    int ret = btrfs_poll(fs_info);
    return ret ? ret
               : fs_info->source.read(fs_info->source.context, devid, uuid, offset, data, size);
}

void btrfs_close_volume(struct btrfs_volume *volume) {
    if (!volume)
        return;
    btrfs_free_mapping_tree(&volume->fs_info);
    free(volume);
}

/* The Linux mount sequence: super validation, system chunks, chunk tree.
 * Source writes, log replay and mount-time cleanup have no stream services. */
int btrfs_open_volume(const struct btrfs_source *source, struct btrfs_volume **result) {
    if (!result)
        return -EINVAL;
    *result = NULL;
    if (!source || !source->read || !source->checksum)
        return -EINVAL;
    struct btrfs_volume *volume = calloc(1, sizeof(*volume));
    if (!volume)
        return -ENOMEM;
    struct btrfs_fs_info *info = &volume->fs_info;
    info->source = *source;
    int ret = btrfs_read_physical(info, 0, NULL, BTRFS_SUPER_INFO_OFFSET, &info->super_copy,
                                  BTRFS_SUPER_INFO_SIZE);
    if (ret)
        goto fail;
    struct btrfs_super_block *sb = &info->super_copy;
    if (btrfs_super_magic(sb) != BTRFS_MAGIC) {
        ret = -EINVAL;
        goto fail;
    }
    info->csum_type = btrfs_super_csum_type(sb);
    info->csum_size = btrfs_csum_type_size(info->csum_type);
    if (!info->csum_size) {
        ret = -EOPNOTSUPP;
        goto fail;
    }
    ret = btrfs_check_super_csum(info, sb);
    if (ret)
        goto fail;
    memcpy(info->metadata_uuid, btrfs_sb_fsid_ptr(sb), BTRFS_FSID_SIZE);
    ret = btrfs_validate_super(info, sb, 0);
    if (ret)
        goto fail;
    /* These features require logical remap/stripe trees or a zoned super log;
     * they must be connected before a stream consumer may accept them. */
    if (btrfs_read_limitation(btrfs_super_incompat_flags(sb))) {
        ret = -EOPNOTSUPP;
        goto fail;
    }
    info->sectorsize = btrfs_super_sectorsize(sb);
    for (u32 n = info->sectorsize; n > 1; n >>= 1)
        info->sectorsize_bits++;
    info->nodesize = btrfs_super_nodesize(sb);
    info->primary_devid = get_unaligned_le64(&sb->dev_item.devid);
    memcpy(info->primary_uuid, sb->dev_item.uuid, BTRFS_UUID_SIZE);
    if (!info->primary_devid) {
        ret = -EUCLEAN;
        goto fail;
    }
    ret = btrfs_read_sys_array(info);
    if (ret)
        goto fail;
    info->chunk_root = (struct btrfs_root){info,
                                           btrfs_super_chunk_root(sb),
                                           btrfs_super_chunk_root_generation(sb),
                                           BTRFS_CHUNK_TREE_OBJECTID,
                                           btrfs_super_chunk_root_level(sb),
                                           0};
    ret = btrfs_read_chunk_tree(info);
    if (ret)
        goto fail;
    info->tree_root = (struct btrfs_root){info,
                                          btrfs_super_root(sb),
                                          btrfs_super_generation(sb),
                                          BTRFS_ROOT_TREE_OBJECTID,
                                          btrfs_super_root_level(sb),
                                          0};
    struct btrfs_tree_parent_check check = {0};
    check.owner_root = info->tree_root.objectid;
    check.transid = info->tree_root.generation;
    check.level = info->tree_root.level;
    struct extent_buffer *root = read_tree_block(info, info->tree_root.bytenr, &check);
    if (IS_ERR(root)) {
        ret = PTR_ERR(root);
        goto fail;
    }
    free_extent_buffer(root);
    ret = btrfs_read_tree_root(info, BTRFS_CSUM_TREE_OBJECTID, &info->csum_root);
    if (ret)
        goto fail;
    *result = volume;
    return 0;
fail:
    btrfs_close_volume(volume);
    return ret;
}

void btrfs_get_volume_info(const struct btrfs_volume *volume, struct btrfs_volume_info *result) {
    const struct btrfs_fs_info *info = &volume->fs_info;
    const struct btrfs_super_block *sb = &info->super_copy;
    memset(result, 0, sizeof(*result));
    result->size = btrfs_super_total_bytes(sb);
    result->used = btrfs_super_bytes_used(sb);
    result->generation = btrfs_super_generation(sb);
    result->devices = btrfs_super_num_devices(sb);
    result->incompat = btrfs_super_incompat_flags(sb);
    result->sectorsize = info->sectorsize;
    result->nodesize = info->nodesize;
    result->checksum = info->csum_type;
    result->log_pending = btrfs_super_log_root(sb) != 0;
    memcpy(result->uuid, sb->fsid, sizeof(result->uuid));
    memcpy(result->label, sb->label, BTRFS_LABEL_SIZE);
}

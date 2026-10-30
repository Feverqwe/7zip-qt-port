/* SPDX-License-Identifier: GPL-2.0 */
/* Read accessors adapted from Linux fs/btrfs/accessors.h. */
#ifndef SEVENZIP_BTRFS_ACCESSORS_H
#define SEVENZIP_BTRFS_ACCESSORS_H

/* An extent_buffer is contiguous in userspace, rather than backed by folios.
 * Callers obtain buffers only after tree-checker validates slot boundaries. */
#define BTRFS_SETGET_HEADER_FUNCS(name, type, member, bits)                                        \
    static inline u##bits btrfs_##name(const struct extent_buffer *eb) {                           \
        return get_unaligned_le##bits(eb->data + offsetof(type, member));                          \
    }
BTRFS_SETGET_HEADER_FUNCS(header_bytenr, struct btrfs_header, bytenr, 64)
BTRFS_SETGET_HEADER_FUNCS(header_generation, struct btrfs_header, generation, 64)
BTRFS_SETGET_HEADER_FUNCS(header_owner, struct btrfs_header, owner, 64)
BTRFS_SETGET_HEADER_FUNCS(header_nritems, struct btrfs_header, nritems, 32)
BTRFS_SETGET_HEADER_FUNCS(header_level, struct btrfs_header, level, 8)
#undef BTRFS_SETGET_HEADER_FUNCS

static inline void btrfs_disk_key_to_cpu(struct btrfs_key *cpu, const struct btrfs_disk_key *disk) {
    cpu->offset = get_unaligned_le64(&disk->offset);
    cpu->type = disk->type;
    cpu->objectid = get_unaligned_le64(&disk->objectid);
}
static inline int btrfs_comp_keys(const struct btrfs_disk_key *disk, const struct btrfs_key *key) {
    struct btrfs_key cpu;
    btrfs_disk_key_to_cpu(&cpu, disk);
    return btrfs_comp_cpu_keys(&cpu, key);
}
static inline size_t btrfs_node_key_ptr_offset(const struct extent_buffer *eb, int nr) {
    (void)eb;
    return offsetof(struct btrfs_node, ptrs) + sizeof(struct btrfs_key_ptr) * nr;
}
static inline void btrfs_node_key_to_cpu(const struct extent_buffer *eb, struct btrfs_key *key,
                                         int nr) {
    struct btrfs_disk_key disk_key;
    read_extent_buffer(eb, &disk_key, btrfs_node_key_ptr_offset(eb, nr), sizeof(disk_key));
    btrfs_disk_key_to_cpu(key, &disk_key);
}
static inline u64 btrfs_node_blockptr(const struct extent_buffer *eb, int nr) {
    return get_unaligned_le64(eb->data + btrfs_node_key_ptr_offset(eb, nr) +
                              offsetof(struct btrfs_key_ptr, blockptr));
}
static inline u64 btrfs_node_ptr_generation(const struct extent_buffer *eb, int nr) {
    return get_unaligned_le64(eb->data + btrfs_node_key_ptr_offset(eb, nr) +
                              offsetof(struct btrfs_key_ptr, generation));
}
static inline size_t btrfs_item_nr_offset(const struct extent_buffer *eb, int nr) {
    (void)eb;
    return offsetof(struct btrfs_leaf, items) + sizeof(struct btrfs_item) * nr;
}
static inline void btrfs_item_key(const struct extent_buffer *eb, struct btrfs_disk_key *disk_key,
                                  int nr) {
    read_extent_buffer(eb, disk_key, btrfs_item_nr_offset(eb, nr), sizeof(*disk_key));
}
static inline void btrfs_item_key_to_cpu(const struct extent_buffer *eb, struct btrfs_key *key,
                                         int nr) {
    struct btrfs_disk_key disk_key;
    btrfs_item_key(eb, &disk_key, nr);
    btrfs_disk_key_to_cpu(key, &disk_key);
}
static inline u32 btrfs_item_offset(const struct extent_buffer *eb, int nr) {
    return get_unaligned_le32(eb->data + btrfs_item_nr_offset(eb, nr) +
                              offsetof(struct btrfs_item, offset));
}
static inline u32 btrfs_item_size(const struct extent_buffer *eb, int nr) {
    return get_unaligned_le32(eb->data + btrfs_item_nr_offset(eb, nr) +
                              offsetof(struct btrfs_item, size));
}
static inline size_t btrfs_item_ptr_offset(const struct extent_buffer *eb, int nr) {
    return sizeof(struct btrfs_header) + btrfs_item_offset(eb, nr);
}

/* Getter half of the Linux SETGET macros; ptr arguments are buffer offsets. */
#define BTRFS_SETGET_FUNCS(name, type, member, bits)                                               \
    static inline u##bits btrfs_##name(const struct extent_buffer *eb, const type *ptr) {          \
        return get_unaligned_le##bits(eb->data + (size_t)(uintptr_t)ptr + offsetof(type, member)); \
    }
#define BTRFS_SETGET_STACK_FUNCS(name, type, member, bits)                                         \
    static inline u##bits btrfs_##name(const type *ptr) {                                          \
        return get_unaligned_le##bits((const u8 *)ptr + offsetof(type, member));                   \
    }
BTRFS_SETGET_FUNCS(device_type, struct btrfs_dev_item, type, 64);
BTRFS_SETGET_FUNCS(device_bytes_used, struct btrfs_dev_item, bytes_used, 64);
BTRFS_SETGET_FUNCS(device_io_align, struct btrfs_dev_item, io_align, 32);
BTRFS_SETGET_FUNCS(device_io_width, struct btrfs_dev_item, io_width, 32);
BTRFS_SETGET_FUNCS(device_start_offset, struct btrfs_dev_item, start_offset, 64);
BTRFS_SETGET_FUNCS(device_sector_size, struct btrfs_dev_item, sector_size, 32);
BTRFS_SETGET_FUNCS(device_id, struct btrfs_dev_item, devid, 64);
BTRFS_SETGET_FUNCS(device_group, struct btrfs_dev_item, dev_group, 32);
BTRFS_SETGET_FUNCS(device_seek_speed, struct btrfs_dev_item, seek_speed, 8);
BTRFS_SETGET_FUNCS(device_bandwidth, struct btrfs_dev_item, bandwidth, 8);
BTRFS_SETGET_FUNCS(device_generation, struct btrfs_dev_item, generation, 64);
BTRFS_SETGET_STACK_FUNCS(stack_device_type, struct btrfs_dev_item, type, 64);
BTRFS_SETGET_STACK_FUNCS(stack_device_total_bytes, struct btrfs_dev_item, total_bytes, 64);
BTRFS_SETGET_STACK_FUNCS(stack_device_bytes_used, struct btrfs_dev_item, bytes_used, 64);
BTRFS_SETGET_STACK_FUNCS(stack_device_io_align, struct btrfs_dev_item, io_align, 32);
BTRFS_SETGET_STACK_FUNCS(stack_device_io_width, struct btrfs_dev_item, io_width, 32);
BTRFS_SETGET_STACK_FUNCS(stack_device_sector_size, struct btrfs_dev_item, sector_size, 32);
BTRFS_SETGET_STACK_FUNCS(stack_device_id, struct btrfs_dev_item, devid, 64);
BTRFS_SETGET_STACK_FUNCS(stack_device_group, struct btrfs_dev_item, dev_group, 32);
BTRFS_SETGET_STACK_FUNCS(stack_device_seek_speed, struct btrfs_dev_item, seek_speed, 8);
BTRFS_SETGET_STACK_FUNCS(stack_device_bandwidth, struct btrfs_dev_item, bandwidth, 8);
BTRFS_SETGET_STACK_FUNCS(stack_device_generation, struct btrfs_dev_item, generation, 64);
BTRFS_SETGET_FUNCS(chunk_length, struct btrfs_chunk, length, 64);
BTRFS_SETGET_FUNCS(chunk_owner, struct btrfs_chunk, owner, 64);
BTRFS_SETGET_FUNCS(chunk_stripe_len, struct btrfs_chunk, stripe_len, 64);
BTRFS_SETGET_FUNCS(chunk_io_align, struct btrfs_chunk, io_align, 32);
BTRFS_SETGET_FUNCS(chunk_io_width, struct btrfs_chunk, io_width, 32);
BTRFS_SETGET_FUNCS(chunk_sector_size, struct btrfs_chunk, sector_size, 32);
BTRFS_SETGET_FUNCS(chunk_type, struct btrfs_chunk, type, 64);
BTRFS_SETGET_FUNCS(chunk_num_stripes, struct btrfs_chunk, num_stripes, 16);
BTRFS_SETGET_FUNCS(chunk_sub_stripes, struct btrfs_chunk, sub_stripes, 16);
BTRFS_SETGET_STACK_FUNCS(stack_chunk_length, struct btrfs_chunk, length, 64);
BTRFS_SETGET_STACK_FUNCS(stack_chunk_owner, struct btrfs_chunk, owner, 64);
BTRFS_SETGET_STACK_FUNCS(stack_chunk_stripe_len, struct btrfs_chunk, stripe_len, 64);
BTRFS_SETGET_STACK_FUNCS(stack_chunk_io_align, struct btrfs_chunk, io_align, 32);
BTRFS_SETGET_STACK_FUNCS(stack_chunk_io_width, struct btrfs_chunk, io_width, 32);
BTRFS_SETGET_STACK_FUNCS(stack_chunk_sector_size, struct btrfs_chunk, sector_size, 32);
BTRFS_SETGET_STACK_FUNCS(stack_chunk_type, struct btrfs_chunk, type, 64);
BTRFS_SETGET_STACK_FUNCS(stack_chunk_num_stripes, struct btrfs_chunk, num_stripes, 16);
BTRFS_SETGET_STACK_FUNCS(stack_chunk_sub_stripes, struct btrfs_chunk, sub_stripes, 16);
BTRFS_SETGET_FUNCS(inode_generation, struct btrfs_inode_item, generation, 64);
BTRFS_SETGET_FUNCS(inode_sequence, struct btrfs_inode_item, sequence, 64);
BTRFS_SETGET_FUNCS(inode_transid, struct btrfs_inode_item, transid, 64);
BTRFS_SETGET_FUNCS(inode_size, struct btrfs_inode_item, size, 64);
BTRFS_SETGET_FUNCS(inode_nbytes, struct btrfs_inode_item, nbytes, 64);
BTRFS_SETGET_FUNCS(inode_block_group, struct btrfs_inode_item, block_group, 64);
BTRFS_SETGET_FUNCS(inode_nlink, struct btrfs_inode_item, nlink, 32);
BTRFS_SETGET_FUNCS(inode_uid, struct btrfs_inode_item, uid, 32);
BTRFS_SETGET_FUNCS(inode_gid, struct btrfs_inode_item, gid, 32);
BTRFS_SETGET_FUNCS(inode_mode, struct btrfs_inode_item, mode, 32);
BTRFS_SETGET_FUNCS(inode_rdev, struct btrfs_inode_item, rdev, 64);
BTRFS_SETGET_FUNCS(inode_flags, struct btrfs_inode_item, flags, 64);
BTRFS_SETGET_STACK_FUNCS(stack_inode_generation, struct btrfs_inode_item, generation, 64);
BTRFS_SETGET_STACK_FUNCS(stack_inode_sequence, struct btrfs_inode_item, sequence, 64);
BTRFS_SETGET_STACK_FUNCS(stack_inode_transid, struct btrfs_inode_item, transid, 64);
BTRFS_SETGET_STACK_FUNCS(stack_inode_size, struct btrfs_inode_item, size, 64);
BTRFS_SETGET_STACK_FUNCS(stack_inode_nbytes, struct btrfs_inode_item, nbytes, 64);
BTRFS_SETGET_STACK_FUNCS(stack_inode_block_group, struct btrfs_inode_item, block_group, 64);
BTRFS_SETGET_STACK_FUNCS(stack_inode_nlink, struct btrfs_inode_item, nlink, 32);
BTRFS_SETGET_STACK_FUNCS(stack_inode_uid, struct btrfs_inode_item, uid, 32);
BTRFS_SETGET_STACK_FUNCS(stack_inode_gid, struct btrfs_inode_item, gid, 32);
BTRFS_SETGET_STACK_FUNCS(stack_inode_mode, struct btrfs_inode_item, mode, 32);
BTRFS_SETGET_STACK_FUNCS(stack_inode_rdev, struct btrfs_inode_item, rdev, 64);
BTRFS_SETGET_STACK_FUNCS(stack_inode_flags, struct btrfs_inode_item, flags, 64);
BTRFS_SETGET_FUNCS(timespec_sec, struct btrfs_timespec, sec, 64);
BTRFS_SETGET_FUNCS(timespec_nsec, struct btrfs_timespec, nsec, 32);
BTRFS_SETGET_STACK_FUNCS(stack_timespec_sec, struct btrfs_timespec, sec, 64);
BTRFS_SETGET_STACK_FUNCS(stack_timespec_nsec, struct btrfs_timespec, nsec, 32);
BTRFS_SETGET_FUNCS(dir_data_len, struct btrfs_dir_item, data_len, 16);
BTRFS_SETGET_FUNCS(dir_flags, struct btrfs_dir_item, type, 8);
BTRFS_SETGET_FUNCS(dir_name_len, struct btrfs_dir_item, name_len, 16);
BTRFS_SETGET_FUNCS(dir_transid, struct btrfs_dir_item, transid, 64);
BTRFS_SETGET_STACK_FUNCS(stack_dir_flags, struct btrfs_dir_item, type, 8);
BTRFS_SETGET_STACK_FUNCS(stack_dir_data_len, struct btrfs_dir_item, data_len, 16);
BTRFS_SETGET_STACK_FUNCS(stack_dir_name_len, struct btrfs_dir_item, name_len, 16);
BTRFS_SETGET_STACK_FUNCS(stack_dir_transid, struct btrfs_dir_item, transid, 64);
BTRFS_SETGET_FUNCS(disk_root_generation, struct btrfs_root_item, generation, 64);
BTRFS_SETGET_FUNCS(disk_root_refs, struct btrfs_root_item, refs, 32);
BTRFS_SETGET_FUNCS(disk_root_bytenr, struct btrfs_root_item, bytenr, 64);
BTRFS_SETGET_FUNCS(disk_root_level, struct btrfs_root_item, level, 8);
BTRFS_SETGET_STACK_FUNCS(root_generation, struct btrfs_root_item, generation, 64);
BTRFS_SETGET_STACK_FUNCS(root_bytenr, struct btrfs_root_item, bytenr, 64);
BTRFS_SETGET_STACK_FUNCS(root_drop_level, struct btrfs_root_item, drop_level, 8);
BTRFS_SETGET_STACK_FUNCS(root_level, struct btrfs_root_item, level, 8);
BTRFS_SETGET_STACK_FUNCS(root_dirid, struct btrfs_root_item, root_dirid, 64);
BTRFS_SETGET_STACK_FUNCS(root_refs, struct btrfs_root_item, refs, 32);
BTRFS_SETGET_STACK_FUNCS(root_flags, struct btrfs_root_item, flags, 64);
BTRFS_SETGET_STACK_FUNCS(root_used, struct btrfs_root_item, bytes_used, 64);
BTRFS_SETGET_STACK_FUNCS(root_limit, struct btrfs_root_item, byte_limit, 64);
BTRFS_SETGET_STACK_FUNCS(root_last_snapshot, struct btrfs_root_item, last_snapshot, 64);
BTRFS_SETGET_STACK_FUNCS(root_generation_v2, struct btrfs_root_item, generation_v2, 64);
BTRFS_SETGET_STACK_FUNCS(root_ctransid, struct btrfs_root_item, ctransid, 64);
BTRFS_SETGET_STACK_FUNCS(root_otransid, struct btrfs_root_item, otransid, 64);
BTRFS_SETGET_STACK_FUNCS(root_stransid, struct btrfs_root_item, stransid, 64);
BTRFS_SETGET_STACK_FUNCS(root_rtransid, struct btrfs_root_item, rtransid, 64);
BTRFS_SETGET_STACK_FUNCS(super_bytenr, struct btrfs_super_block, bytenr, 64);
BTRFS_SETGET_STACK_FUNCS(super_flags, struct btrfs_super_block, flags, 64);
BTRFS_SETGET_STACK_FUNCS(super_generation, struct btrfs_super_block, generation, 64);
BTRFS_SETGET_STACK_FUNCS(super_root, struct btrfs_super_block, root, 64);
BTRFS_SETGET_STACK_FUNCS(super_sys_array_size, struct btrfs_super_block, sys_chunk_array_size, 32);
BTRFS_SETGET_STACK_FUNCS(super_chunk_root_generation, struct btrfs_super_block,
                         chunk_root_generation, 64);
BTRFS_SETGET_STACK_FUNCS(super_root_level, struct btrfs_super_block, root_level, 8);
BTRFS_SETGET_STACK_FUNCS(super_chunk_root, struct btrfs_super_block, chunk_root, 64);
BTRFS_SETGET_STACK_FUNCS(super_chunk_root_level, struct btrfs_super_block, chunk_root_level, 8);
BTRFS_SETGET_STACK_FUNCS(super_log_root, struct btrfs_super_block, log_root, 64);
BTRFS_SETGET_STACK_FUNCS(super_log_root_level, struct btrfs_super_block, log_root_level, 8);
BTRFS_SETGET_STACK_FUNCS(super_total_bytes, struct btrfs_super_block, total_bytes, 64);
BTRFS_SETGET_STACK_FUNCS(super_bytes_used, struct btrfs_super_block, bytes_used, 64);
BTRFS_SETGET_STACK_FUNCS(super_sectorsize, struct btrfs_super_block, sectorsize, 32);
BTRFS_SETGET_STACK_FUNCS(super_nodesize, struct btrfs_super_block, nodesize, 32);
BTRFS_SETGET_STACK_FUNCS(super_stripesize, struct btrfs_super_block, stripesize, 32);
BTRFS_SETGET_STACK_FUNCS(super_root_dir, struct btrfs_super_block, root_dir_objectid, 64);
BTRFS_SETGET_STACK_FUNCS(super_num_devices, struct btrfs_super_block, num_devices, 64);
BTRFS_SETGET_STACK_FUNCS(super_compat_flags, struct btrfs_super_block, compat_flags, 64);
BTRFS_SETGET_STACK_FUNCS(super_compat_ro_flags, struct btrfs_super_block, compat_ro_flags, 64);
BTRFS_SETGET_STACK_FUNCS(super_incompat_flags, struct btrfs_super_block, incompat_flags, 64);
BTRFS_SETGET_STACK_FUNCS(super_csum_type, struct btrfs_super_block, csum_type, 16);
BTRFS_SETGET_STACK_FUNCS(super_cache_generation, struct btrfs_super_block, cache_generation, 64);
BTRFS_SETGET_STACK_FUNCS(super_magic, struct btrfs_super_block, magic, 64);
BTRFS_SETGET_STACK_FUNCS(super_uuid_tree_generation, struct btrfs_super_block, uuid_tree_generation,
                         64);
BTRFS_SETGET_STACK_FUNCS(super_nr_global_roots, struct btrfs_super_block, nr_global_roots, 64);
BTRFS_SETGET_STACK_FUNCS(super_remap_root, struct btrfs_super_block, remap_root, 64);
BTRFS_SETGET_STACK_FUNCS(super_remap_root_generation, struct btrfs_super_block,
                         remap_root_generation, 64);
BTRFS_SETGET_STACK_FUNCS(super_remap_root_level, struct btrfs_super_block, remap_root_level, 8);
BTRFS_SETGET_STACK_FUNCS(stack_file_extent_type, struct btrfs_file_extent_item, type, 8);
BTRFS_SETGET_STACK_FUNCS(stack_file_extent_disk_bytenr, struct btrfs_file_extent_item, disk_bytenr,
                         64);
BTRFS_SETGET_STACK_FUNCS(stack_file_extent_offset, struct btrfs_file_extent_item, offset, 64);
BTRFS_SETGET_STACK_FUNCS(stack_file_extent_generation, struct btrfs_file_extent_item, generation,
                         64);
BTRFS_SETGET_STACK_FUNCS(stack_file_extent_num_bytes, struct btrfs_file_extent_item, num_bytes, 64);
BTRFS_SETGET_STACK_FUNCS(stack_file_extent_ram_bytes, struct btrfs_file_extent_item, ram_bytes, 64);
BTRFS_SETGET_STACK_FUNCS(stack_file_extent_disk_num_bytes, struct btrfs_file_extent_item,
                         disk_num_bytes, 64);
BTRFS_SETGET_STACK_FUNCS(stack_file_extent_compression, struct btrfs_file_extent_item, compression,
                         8);
BTRFS_SETGET_FUNCS(file_extent_type, struct btrfs_file_extent_item, type, 8);
BTRFS_SETGET_FUNCS(file_extent_disk_bytenr, struct btrfs_file_extent_item, disk_bytenr, 64);
BTRFS_SETGET_FUNCS(file_extent_generation, struct btrfs_file_extent_item, generation, 64);
BTRFS_SETGET_FUNCS(file_extent_disk_num_bytes, struct btrfs_file_extent_item, disk_num_bytes, 64);
BTRFS_SETGET_FUNCS(file_extent_offset, struct btrfs_file_extent_item, offset, 64);
BTRFS_SETGET_FUNCS(file_extent_num_bytes, struct btrfs_file_extent_item, num_bytes, 64);
BTRFS_SETGET_FUNCS(file_extent_ram_bytes, struct btrfs_file_extent_item, ram_bytes, 64);
BTRFS_SETGET_FUNCS(file_extent_compression, struct btrfs_file_extent_item, compression, 8);
BTRFS_SETGET_FUNCS(file_extent_encryption, struct btrfs_file_extent_item, encryption, 8);
BTRFS_SETGET_FUNCS(file_extent_other_encoding, struct btrfs_file_extent_item, other_encoding, 16);
#undef BTRFS_SETGET_FUNCS
#undef BTRFS_SETGET_STACK_FUNCS
static inline u8 btrfs_dir_ftype(const struct extent_buffer *eb,
                                 const struct btrfs_dir_item *item) {
    return btrfs_dir_flags_to_ftype(btrfs_dir_flags(eb, item));
}
static inline void btrfs_dir_item_key_to_cpu(const struct extent_buffer *eb,
                                             const struct btrfs_dir_item *item,
                                             struct btrfs_key *key) {
    struct btrfs_disk_key disk;
    read_extent_buffer(eb, &disk,
                       (size_t)(uintptr_t)item + offsetof(struct btrfs_dir_item, location),
                       sizeof(disk));
    btrfs_disk_key_to_cpu(key, &disk);
}
static inline u64 btrfs_disk_key_objectid(const struct btrfs_disk_key *key) {
    return get_unaligned_le64(&key->objectid);
}
#endif

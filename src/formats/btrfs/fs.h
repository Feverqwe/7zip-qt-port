/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2007 Oracle. All rights reserved. */
/* Read-side state adapted from Linux fs/btrfs/fs.h and volumes.h. */
#ifndef SEVENZIP_BTRFS_FS_H
#define SEVENZIP_BTRFS_FS_H
#include "compat/linux/include/linux/rbtree.h"
#include "formats/btrfs/btrfsreader.h"
#include "formats/btrfs/ctree.h"

#define BTRFS_SUPER_INFO_OFFSET UINT64_C(65536)
#define BTRFS_SUPER_INFO_SIZE 4096
#define BTRFS_MIN_BLOCKSIZE 4096
#define BTRFS_MAX_METADATA_BLOCKSIZE 65536
#define BTRFS_STRIPE_LEN_SHIFT 16
#define BTRFS_STRIPE_LEN (UINT64_C(1) << BTRFS_STRIPE_LEN_SHIFT)
#define BTRFS_STRIPE_LEN_MASK (BTRFS_STRIPE_LEN - 1)

struct btrfs_io_stripe {
    u64 devid, physical;
    u8 uuid[BTRFS_UUID_SIZE];
};
struct btrfs_chunk_map {
    struct rb_node rb_node;
    u64 start, chunk_len, type, on_disk_type;
    int num_stripes, sub_stripes;
    struct btrfs_io_stripe stripes[];
};
struct btrfs_fs_info {
    struct btrfs_source source;
    struct btrfs_super_block super_copy;
    struct rb_root_cached mapping_tree;
    struct btrfs_root tree_root, chunk_root, csum_root;
    u32 nodesize, sectorsize, sectorsize_bits;
    u16 csum_type, csum_size;
    u64 primary_devid;
    u8 primary_uuid[BTRFS_UUID_SIZE];
    u8 metadata_uuid[BTRFS_FSID_SIZE];
};
struct btrfs_volume {
    struct btrfs_fs_info fs_info;
};
struct btrfs_inode {
    struct btrfs_root *root;
    struct btrfs_file_info info;
};
struct btrfs_file {
    struct btrfs_root root;
    struct btrfs_inode inode;
};
enum btrfs_compression_type {
    BTRFS_COMPRESS_NONE = 0,
    BTRFS_COMPRESS_ZLIB = 1,
    BTRFS_COMPRESS_LZO = 2,
    BTRFS_COMPRESS_ZSTD = 3,
    BTRFS_NR_COMPRESS_TYPES = 4
};
#define BTRFS_FILE_EXTENT_INLINE_DATA_START offsetof(struct btrfs_file_extent_item, disk_bytenr)
int btrfs_find_root(struct btrfs_root *, const struct btrfs_key *, struct btrfs_path *,
                    struct btrfs_root_item *, struct btrfs_key *);
int btrfs_lookup_inode(void *, struct btrfs_root *, struct btrfs_path *, struct btrfs_key *, int);
int btrfs_lookup_file_extent(void *, struct btrfs_root *, struct btrfs_path *, u64, u64, int);
int btrfs_read_tree_root(struct btrfs_fs_info *, u64, struct btrfs_root *);
int btrfs_read_locked_inode(struct btrfs_inode *);
int btrfs_real_readdir(struct btrfs_inode *, void *, btrfs_dir_emit);
static inline void btrfs_inode_split_flags(u64 value, u32 *flags, u32 *ro_flags) {
    *flags = (u32)value;
    *ro_flags = (u32)(value >> 32);
}
u32 crc32c(u32, const void *, size_t);
static inline u64 btrfs_name_hash(const char *name, int len) {
    return (u64)crc32c((u32)~1, name, len);
}

int btrfs_validate_super(const struct btrfs_fs_info *, const struct btrfs_super_block *, int);
int btrfs_check_super_csum(struct btrfs_fs_info *, const struct btrfs_super_block *);
int btrfs_validate_extent_buffer(struct extent_buffer *, const struct btrfs_tree_parent_check *);
int btrfs_check_leaf(struct extent_buffer *);
int btrfs_check_node(struct extent_buffer *);
int btrfs_check_eb_owner(const struct extent_buffer *, u64);
int btrfs_read_sys_array(struct btrfs_fs_info *);
int btrfs_read_chunk_tree(struct btrfs_fs_info *);
int btrfs_read_logical(struct btrfs_fs_info *, u64, void *, size_t, unsigned int mirror);
int btrfs_read_physical(struct btrfs_fs_info *, u64 devid, const u8 *, u64, void *, size_t);
unsigned int btrfs_num_copies(struct btrfs_fs_info *, u64);
void btrfs_free_mapping_tree(struct btrfs_fs_info *);
u64 btrfs_sb_offset(int mirror);
int btrfs_poll(struct btrfs_fs_info *);
u16 btrfs_csum_type_size(u16);
#define btrfs_fs_incompat(info, opt)                                                               \
    (!!(btrfs_super_incompat_flags(&(info)->super_copy) & BTRFS_FEATURE_INCOMPAT_##opt))
#define btrfs_fs_compat_ro(info, opt)                                                              \
    (!!(btrfs_super_compat_ro_flags(&(info)->super_copy) & BTRFS_FEATURE_COMPAT_RO_##opt))
#endif

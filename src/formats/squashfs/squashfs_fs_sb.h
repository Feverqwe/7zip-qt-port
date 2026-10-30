/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Squashfs
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008
 * Phillip Lougher <phillip@squashfs.org.uk>
 * Read-only archive view of squashfs_sb_info; VFS/cache fields are omitted.
 */
#ifndef SQUASHFS_FS_SB
#define SQUASHFS_FS_SB
#include "formats/squashfs/squashfscompat.h"
struct squashfs_sb_info {
    const struct squashfs_decompressor *decompressor;
    void *stream;
    unsigned int block_size;
    unsigned int block_log;
    int next_meta_index;
    struct mutex meta_index_mutex;
    struct meta_index *meta_index;
    long long bytes_used;
    bool panic_on_errors;
    const struct squashfs_decompressor_thread_ops *thread_ops;
    void *read_context;
    struct squashfs_cache *block_cache;
    __le64 *id_table;
    __le64 *fragment_index;
    __le64 *xattr_id_table;
    __le64 *inode_lookup_table;
    u64 inode_table;
    u64 directory_table;
    u64 xattr_table;
    unsigned int inodes;
    unsigned int fragments;
    unsigned int xattr_ids;
    unsigned int ids;
};
struct squashfs_cache {
    void *private_data;
};
struct squashfs_cache_entry {
    u64 block;
    int length;
    u64 next_index;
    int error;
    void **data;
};
#endif

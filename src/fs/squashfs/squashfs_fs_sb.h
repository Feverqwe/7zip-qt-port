/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Squashfs
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008
 * Phillip Lougher <phillip@squashfs.org.uk>
 * Read-only archive view of squashfs_sb_info; VFS/cache fields are omitted.
 */
#ifndef SQUASHFS_FS_SB
#define SQUASHFS_FS_SB
#include "squashfscompat.h"
struct squashfs_sb_info {
    const struct squashfs_decompressor *decompressor;
    void *stream;
    unsigned int block_size;
    long long bytes_used;
    bool panic_on_errors;
    const struct squashfs_decompressor_thread_ops *thread_ops;
    void *read_context;
};
#endif

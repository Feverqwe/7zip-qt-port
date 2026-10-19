/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Squashfs
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008
 * Phillip Lougher <phillip@squashfs.org.uk>
 * Archive view of the xattr fields of squashfs_inode_info.
 */
#ifndef SEVENZIP_SQUASHFS_FS_I_H
#define SEVENZIP_SQUASHFS_FS_I_H
struct squashfs_inode_info {
    u64 xattr;
    unsigned int xattr_size;
    int xattr_count;
};
struct inode { struct super_block *i_sb; struct squashfs_inode_info info; };
static inline struct squashfs_inode_info *squashfs_i(struct inode *inode) { return &inode->info; }
#endif

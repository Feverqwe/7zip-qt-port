/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Squashfs - a compressed read only filesystem for Linux
 *
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008
 * Phillip Lougher <phillip@squashfs.org.uk>
 *
 * squashfs.h
 * Portable archive-facing declarations from the kernel driver.
 */
#ifndef SEVENZIP_SQUASHFS_H
#define SEVENZIP_SQUASHFS_H
#include "formats/squashfs/squashfscompat.h"
int squashfs_read_data(struct super_block *, u64, int, u64 *, struct squashfs_page_actor *);
const struct squashfs_decompressor *squashfs_lookup_decompressor(int);
void *squashfs_decompressor_setup(struct super_block *, unsigned short);
struct squashfs_cache;
struct squashfs_cache_entry;
struct inode;
int squashfs_read_blocklist(struct inode *, int, u64 *);
int squashfs_read_blocklist_ptrs(struct inode *, int, u64 *, int *, u64 *);
int squashfs_read_blocklist_next(struct inode *, u64 *, int *);
struct squashfs_cache_entry *squashfs_cache_get(struct super_block *, struct squashfs_cache *, u64,
                                                int);
void squashfs_cache_put(struct squashfs_cache_entry *);
int squashfs_read_metadata(struct super_block *, void *, u64 *, int *, int);
void *squashfs_read_table(struct super_block *, u64, int);
int squashfs_get_id(struct super_block *, unsigned int, unsigned int *);
__le64 *squashfs_read_id_index_table(struct super_block *, u64, u64, unsigned short);
int squashfs_frag_lookup(struct super_block *, unsigned int, u64 *);
__le64 *squashfs_read_fragment_index_table(struct super_block *, u64, u64, unsigned int);
__le64 *squashfs_read_inode_lookup_table(struct super_block *, u64, u64, unsigned int);
long long squashfs_inode_lookup(struct super_block *, int);
#endif

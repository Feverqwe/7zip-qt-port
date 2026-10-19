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
#include "squashfscompat.h"
int squashfs_read_data(struct super_block *, u64, int, u64 *, struct squashfs_page_actor *);
const struct squashfs_decompressor *squashfs_lookup_decompressor(int);
void *squashfs_decompressor_setup(struct super_block *, unsigned short);
#endif

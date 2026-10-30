// SPDX-License-Identifier: GPL-2.0
/*
 * linux/fs/hfsplus/bfind.c
 * Copyright (C) 2001 Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 * hfsplus_brec_read_cat: the archive supplies an already bounded record instead
 * of the VFS B-tree read service. On-disk type/size validation is retained.
 */
#include "formats/hfs/plus/hfscompat.h"

int hfsplus_brec_read_cat(const void *record, size_t length)
{
    const u8 *entry = record;
    size_t expected_size;
    if (length < 2)
        return -EIO;
    switch ((u16)((u16)entry[0] << 8 | entry[1])) {
    case 1:
        expected_size = 88;
        break;
    case 2:
        expected_size = 248;
        break;
    case 3:
    case 4:
        if (length < 10)
            return -EIO;
        expected_size = 10 + 2 * ((size_t)entry[8] << 8 | entry[9]);
        if (expected_size > 10 + 255 * 2)
            return -EIO;
        break;
    default:
        return -EIO;
    }
    return length == expected_size ? 0 : -EIO;
}

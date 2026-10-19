// SPDX-License-Identifier: GPL-2.0
/*
 * linux/fs/hfsplus/xattr.c
 * Vyacheslav Dubeyko <slava@dubeyko.com>
 * hfsplus_getxattr_finder_info: catalog reads are supplied by the existing
 * archive reader; user_info/finder_info bytes and size semantics are retained.
 */
#include "hfscompat.h"

int hfsplus_getxattr_finder_info(const void *record, size_t length, void *value, size_t size)
{
    const u8 *entry = record;
    u16 entry_type;
    if (length < 2)
        return -EIO;
    entry_type = (u16)((u16)entry[0] << 8 | entry[1]);
    if (entry_type != 1 && entry_type != 2)
        return -EOPNOTSUPP;
    if (hfsplus_brec_read_cat(record, length))
        return -EIO;
    if (size >= 32) {
        memcpy(value, entry + 48, 32);
        return 32;
    }
    return size ? -ERANGE : 32;
}

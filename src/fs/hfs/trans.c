/*
 * linux/fs/hfs/trans.c
 * Copyright (C) 1995-1997 Paul H. Hargrove
 * This file may be distributed under the terms of the GNU General Public License.
 * hfs_mac2asc default byte-preserving read branch. Mount-selected NLS is omitted;
 * the archive projects each returned byte to one UTF-16 code unit.
 */
#include "hfs_fs.h"

int hfs_mac2asc(u8 *out, const struct hfs_name *in)
{
    const u8 *src = in->name;
    int srclen = in->len;
    u8 *dst = out;
    u8 ch;
    if (srclen > HFS_NAMELEN)
        srclen = HFS_NAMELEN;
    while (--srclen >= 0)
        *dst++ = (ch = *src++) == '/' ? ':' : ch;
    return (int)(dst - out);
}

// SPDX-License-Identifier: GPL-2.0
/*
 * linux/fs/hfsplus/inode.c
 * Copyright (C) 2001 Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 * hfsplus_get_perms: disk permission interpretation. Mount credential overrides
 * and mutable VFS inode flags are represented by explicit defaults/raw flags.
 */
#include "hfscompat.h"
#include "../../include/linux/kdev_t.h"

static u32 get32(const u8 *p)
{
    return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3];
}

int hfsplus_get_perms(const void *permissions, int dir, uint32_t uid, uint32_t gid,
                      uint16_t umask, struct sevenzip_hfsplus_permissions *result)
{
    const u8 *perms = permissions;
    u16 mode = (u16)((u16)perms[10] << 8 | perms[11]);
    if (dir) {
        if (mode && (mode & 0170000) != 0040000)
            goto bad_type;
    } else if (mode) {
        switch (mode & 0170000) {
        case 0100000:
        case 0120000:
        case 0020000:
        case 0060000:
        case 0010000:
        case 0140000:
            break;
        default:
            goto bad_type;
        }
    }
    result->owner = get32(perms);
    if (!result->owner && !mode)
        result->owner = uid;
    result->group = get32(perms + 4);
    if (!result->group && !mode)
        result->group = gid;
    if (dir) {
        mode = mode ? (mode & 07777) : (0777 & ~umask);
        mode |= 0040000;
    } else if (!mode)
        mode = 0100000 | (0666 & ~umask);
    result->mode = mode;
    result->userflags = perms[9];
    result->rootflags = perms[8];
    return 0;
bad_type:
    return -EIO;
}

/* hfsplus_cat_read_inode: the caller has already bounded the catalog record,
 * mapped its forks and retained timestamps. Keep permission/link/special inode
 * interpretation here; VFS operations and inode publication are not needed. */
int hfsplus_cat_read_inode(const void *permissions, int dir,
                          struct sevenzip_hfsplus_inode *result)
{
    int res;
    u32 dev = get32((const u8 *)permissions + 12);
    res = hfsplus_get_perms(permissions, dir, 0, 0, 0022, &result->permissions);
    if (res)
        return res;
    result->links = 1;
    result->device_major = result->device_minor = 0;
    if (!dir) {
        u16 mode = result->permissions.mode;
        if ((mode & 0170000) == 0100000) {
            if (dev)
                result->links = dev;
        } else if ((mode & 0170000) != 0120000) {
            /* init_special_inode receives the raw internal Linux dev_t, not
             * the new_encode_dev disk encoding used by other filesystems. */
            result->device_major = MAJOR(dev);
            result->device_minor = MINOR(dev);
        }
    }
    return 0;
}

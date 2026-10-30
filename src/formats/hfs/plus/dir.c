// SPDX-License-Identifier: GPL-2.0
/*
 * linux/fs/hfsplus/dir.c
 * Copyright (C) 2001 Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 *
 * hfsplus_lookup hardlink resolution. Catalog/VFS services are supplied by the
 * archive catalog adapter; inode/dentry publication and write paths are omitted.
 */
#include "formats/hfs/plus/hfscompat.h"
#include <stdio.h>

int hfsplus_lookup(struct sevenzip_hfsplus_record entry, unsigned source,
                   uint32_t hidden_id, uint32_t hidden_creation, uint32_t root_creation,
                   sevenzip_hfsplus_lookup_fn lookup, void *context, unsigned *resolved)
{
    unsigned cnid = source;
    int err, seen = 0;
    *resolved = source;
again:
    if (!entry.is_directory && entry.file_type == 0x686c6e6b &&
        entry.creator == 0x6866732b && hidden_id &&
        (entry.creation == hidden_creation || entry.creation == root_creation)) {
        char name[32];
        if (seen) {
            /* A link pointing to another link is treated as the original file. */
            cnid = source;
        } else {
            seen = 1;
            snprintf(name, sizeof(name), "iNode%d", (int32_t)entry.special);
            err = lookup(context, hidden_id, name, &entry, &cnid);
            if (err)
                return err;
            goto again;
        }
    }
    *resolved = cnid;
    return 0;
}

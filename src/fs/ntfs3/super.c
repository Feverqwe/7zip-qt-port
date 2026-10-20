// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 *
 * ntfs_fill_super $AttrDef read-side table selection. VFS/system-file I/O is
 * supplied by the existing archive reader; allocation and mount setup omitted.
 */
#include "ntfseacompat.h"

int sevenzip_ntfs_ea_limit(const void *data, size_t bytes, u32 *limit)
{
    const struct ATTR_DEF_ENTRY *t = data;
    size_t done;
    *limit = 0;
    if (!data || bytes < sizeof(*t) || bytes > 100 * sizeof(*t) || ATTR_STD != t->type)
        return -EINVAL;
    t += 1;
    done = sizeof(*t);
    while (done + sizeof(*t) <= bytes) {
        u32 t32 = le32_to_cpu(t->type);
        u64 sz = le64_to_cpu(t->max_sz);
        if ((t32 & 0xF) || le32_to_cpu(t[-1].type) >= t32)
            break;
        if (t->type == ATTR_EA)
            *limit = (u32)sz;
        done += sizeof(*t);
        t += 1;
    }
    return 0;
}

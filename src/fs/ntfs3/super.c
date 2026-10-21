// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 *
 * ntfs_fill_super $AttrDef/$Volume read-side projection. VFS/system-file I/O is
 * supplied by the existing archive reader; allocation and mount setup omitted.
 */
#include "ntfseacompat.h"
#include "../../ntfsvolume.h"

static inline bool is_attr_ext(const struct ATTRIB *attr)
{
	return attr->flags & (ATTR_FLAG_SPARSED | ATTR_FLAG_COMPRESSED);
}

/* ntfs_fill_super $Volume read projection; SDK supplies the selected attribute.
 * An unavailable state is reported to the archive owner, without repairing it. */
int sevenzip_ntfs_read_volume_info(const void *data, size_t bytes, int nonresident,
                                  uint16_t flags, struct sevenzip_ntfs_volume_info *state)
{
    struct ATTRIB attr = {data, bytes, nonresident, cpu_to_le16(flags)};
    const struct VOLUME_INFO *info;
    if (!data || is_attr_ext(&attr) ||
        !(info = resident_data_ex(&attr, SIZEOF_ATTRIBUTE_VOLUME_INFO)))
        return -EINVAL;
    state->major = info->major_ver;
    state->minor = info->minor_ver;
    state->dirty = false;
    if (info->flags & VOLUME_FLAG_DIRTY)
        state->dirty = true;
    return 0;
}

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

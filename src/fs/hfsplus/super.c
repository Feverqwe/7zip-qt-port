// SPDX-License-Identifier: GPL-2.0-only
/*
 * linux/fs/hfsplus/super.c
 * Copyright (C) 2001 Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 * hfsplus_fill_super read-side version guard; VFS failure transport omitted.
 */
#include "hfscompat.h"

int sevenzip_hfsplus_volume_version(uint16_t version)
{
    if (version < HFSPLUS_MIN_VERSION || version > HFSPLUS_CURRENT_VERSION)
        return 0;
    return 1;
}

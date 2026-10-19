// SPDX-License-Identifier: GPL-2.0-only
/*
 * lowlevel.c
 *
 * PURPOSE
 *  Low Level Device Routines for the UDF filesystem
 *
 * COPYRIGHT
 *  (C) 1999-2001 Ben Fennema
 *
 * HISTORY
 *
 *  03/26/99 blf  Created.
 */

#include "udfdecl.h"

/* IInStream provides no CD-ROM ioctl interface: use the regular-file branches. */
unsigned int udf_get_last_session(struct super_block *sb)
{
    (void)sb;
    return 0;
}

udf_pblk_t udf_get_last_block(struct super_block *sb)
{
    uint64_t blocks = sb_bdev_nr_blocks(sb);
    if (blocks > UINT32_MAX)
        return 0;
    return blocks ? blocks - 1 : 0;
}

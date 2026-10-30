/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  linux/include/linux/hfsplus_fs.h
 *
 * Copyright (C) 1999
 * Brad Boyer (flar@pants.nu)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 *
 * Read-only time helper; VFS declarations and write conversion are omitted.
 */
#ifndef SEVENZIP_HFSPLUS_FS_H
#define SEVENZIP_HFSPLUS_FS_H
#include "formats/hfs/plus/hfscompat.h"

/*
 * time helpers: convert between 1904-base and 1970-base timestamps
 *
 * HFS+ implementations are highly inconsistent, this one matches the
 * traditional behavior of 64-bit Linux, giving the most useful
 * time range between 1970 and 2106, by treating any on-disk timestamp
 * under HFSPLUS_UTC_OFFSET (Jan 1 1970) as a time between 2040 and 2106.
 */
#define HFSPLUS_UTC_OFFSET 2082844800U

static inline int64_t __hfsp_mt2ut(u32 mt) {
    int64_t ut = (u32)(be32_to_cpu(mt) - HFSPLUS_UTC_OFFSET);

    return ut;
}
#endif

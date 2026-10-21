// SPDX-License-Identifier: GPL-2.0
/*
 * linux/fs/hfsplus/wrapper.c
 * Copyright (C) 2001 Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 * Read-only MDB decoder; byte transport and volume lifetime stay with the owner.
 */
#include "hfscompat.h"
#define cpu_to_be16 be16_to_cpu
static u32 get_unaligned_be32(const void *data)
{
    const u8 *p = data;
    return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3];
}

int hfsplus_read_mdb(const unsigned char *bufptr, struct hfsplus_wd *wd)
{
	u32 extent;
	u16 attrib;
	__be16 sig;

	sig = *(__be16 *)(bufptr + HFSP_WRAPOFF_EMBEDSIG);
	if (sig != cpu_to_be16(HFSPLUS_VOLHEAD_SIG) &&
	    sig != cpu_to_be16(HFSPLUS_VOLHEAD_SIGX))
		return 0;

	attrib = be16_to_cpu(*(__be16 *)(bufptr + HFSP_WRAPOFF_ATTRIB));
	/* Archive read-only adaptation: expose the mount protection requirement
	 * as a limitation, without discarding a bounded embedded volume. */
	wd->unprotected = !(attrib & HFSP_WRAP_ATTRIB_SLOCK) ||
			 !(attrib & HFSP_WRAP_ATTRIB_SPARED);

	wd->ablk_size =
		be32_to_cpu(*(__be32 *)(bufptr + HFSP_WRAPOFF_ABLKSIZE));
	if (wd->ablk_size < HFSPLUS_SECTOR_SIZE)
		return 0;
	if (wd->ablk_size % HFSPLUS_SECTOR_SIZE)
		return 0;
	wd->ablk_start =
		be16_to_cpu(*(__be16 *)(bufptr + HFSP_WRAPOFF_ABLKSTART));

	extent = get_unaligned_be32(bufptr + HFSP_WRAPOFF_EMBEDEXT);
	wd->embed_start = (extent >> 16) & 0xFFFF;
	wd->embed_count = extent & 0xFFFF;

	return 1;
}

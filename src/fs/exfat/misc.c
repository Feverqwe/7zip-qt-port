// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Written 1992,1993 by Werner Almesberger
 *  22/11/2000 - Fixed fat_date_unix2dos for dates earlier than 01/01/1980
 *		 and date_dos2unix for date==0 by Igor Zhbanov(bsg@uniyar.ac.ru)
 * Copyright (C) 2012-2013 Samsung Electronics Co., Ltd.
 */

/* Read-only userspace adaptation; see docs/upstream-port-audit/exfat-linux.md. */

#include "exfat_fs.h"

u16 exfat_calc_chksum16(void *data, int len, u16 chksum, int type)
{
	int i;
	u8 *c = (u8 *)data;

	for (i = 0; i < len; i++, c++) {
		if (unlikely(type == CS_DIR_ENTRY && (i == 2 || i == 3)))
			continue;
		chksum = ((chksum << 15) | (chksum >> 1)) + *c;
	}
	return chksum;
}

u32 exfat_calc_chksum32(void *data, int len, u32 chksum, int type)
{
	int i;
	u8 *c = (u8 *)data;

	for (i = 0; i < len; i++, c++) {
		if (unlikely(type == CS_BOOT_SECTOR &&
			     (i == 106 || i == 107 || i == 112)))
			continue;
		chksum = ((chksum << 31) | (chksum >> 1)) + *c;
	}
	return chksum;
}

void exfat_chain_set(struct exfat_chain *ec, unsigned int dir,
		unsigned int size, unsigned char flags)
{
	ec->dir = dir;
	ec->size = size;
	ec->flags = flags;
}

void exfat_chain_dup(struct exfat_chain *dup, struct exfat_chain *ec)
{
	exfat_chain_set(dup, ec->dir, ec->size, ec->flags);
}

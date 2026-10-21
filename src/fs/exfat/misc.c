// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Written 1992,1993 by Werner Almesberger
 *  22/11/2000 - Fixed fat_date_unix2dos for dates earlier than 01/01/1980
 *		 and date_dos2unix for date==0 by Igor Zhbanov(bsg@uniyar.ac.ru)
 * Copyright (C) 2012-2013 Samsung Electronics Co., Ltd.
 */

/* Read-only userspace adaptation; see docs/upstream-port-audit/exfat-linux.md. */

#include "exfat_fs.h"

#define SECS_PER_MIN    (60)
#define TIMEZONE_SEC(x) ((x) * 15 * SECS_PER_MIN)
#define NSEC_PER_MSEC 1000000L

static void exfat_adjust_tz(struct timespec64 *ts, u8 tz_off)
{
	if (tz_off <= 0x3F)
		ts->tv_sec -= TIMEZONE_SEC(tz_off);
	else /* 0x40 <= (tz_off & 0x7F) <=0x7F */
		ts->tv_sec += TIMEZONE_SEC(0x80 - tz_off);
}

static inline int exfat_tz_offset(struct exfat_sb_info *sbi)
{
	/* Archive reads have no mount options or system timezone override.
	 * Use the original driver's zero-initialized default time_offset. */
	(void)sbi;
	return 0;
}

/* Convert a EXFAT time/date pair to a UNIX date (seconds since 1 1 70). */
void exfat_get_entry_time(struct exfat_sb_info *sbi, struct timespec64 *ts,
		u8 tz, __le16 time, __le16 date, u8 time_cs)
{
	u16 t = le16_to_cpu(time);
	u16 d = le16_to_cpu(date);

	ts->tv_sec = mktime64(1980 + (d >> 9), d >> 5 & 0x000F, d & 0x001F,
			      t >> 11, (t >> 5) & 0x003F, (t & 0x001F) << 1);

	/* time_cs field represent 0 ~ 199cs(1990 ms) */
	if (time_cs) {
		ts->tv_sec += time_cs / 100;
		ts->tv_nsec = (time_cs % 100) * 10 * NSEC_PER_MSEC;
	} else
		ts->tv_nsec = 0;

	if (tz & EXFAT_TZ_VALID)
		/* Adjust timezone to UTC0. */
		exfat_adjust_tz(ts, tz & ~EXFAT_TZ_VALID);
	else
		ts->tv_sec -= exfat_tz_offset(sbi) * SECS_PER_MIN;
}

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

// SPDX-License-Identifier: LGPL-2.0+
/* Copyright (C) 1993, 1994, 1995, 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Paul Eggert (eggert@twinsun.com). */

/*
 * dgb 10/02/98: ripped this from glibc source to help convert timestamps
 *               to unix time
 *     10/04/98: added new table-based lookup after seeing how ugly
 *               the gnu code is
 * blf 09/27/99: ripped out all the old code and inserted new table from
 *		 John Brockmeyer (without leap second corrections)
 *		 rewrote udf_stamp_to_time and fixed timezone accounting in
 *		 udf_time_to_stamp.
 */

/*
 * We don't take into account leap seconds. This may be correct or incorrect.
 * For more NIST information (especially dealing with leap seconds), see:
 * http://www.boulder.nist.gov/timefreq/pubs/bulletin/leapsecond.htm
 */

#include "formats/udf/udfdecl.h"

void
udf_disk_stamp_to_time(struct timespec64 *dest, struct timestamp src)
{
	u16 typeAndTimezone = le16_to_cpu(src.typeAndTimezone);
	u16 year = le16_to_cpu(src.year);
	uint8_t type = typeAndTimezone >> 12;
	int16_t offset;

	if (type == 1) {
		offset = typeAndTimezone << 4;
		/* sign extent offset */
		offset = (offset >> 4);
		if (offset == -2047) /* unspecified offset */
			offset = 0;
	} else
		offset = 0;

	dest->tv_sec = mktime64(year, src.month, src.day, src.hour, src.minute,
			src.second);
	dest->tv_sec -= offset * 60;

	/*
	 * Sanitize nanosecond field since reportedly some filesystems are
	 * recorded with bogus sub-second values.
	 */
	if (src.centiseconds < 100 && src.hundredsOfMicroseconds < 100 &&
	    src.microseconds < 100) {
		dest->tv_nsec = 1000 * (src.centiseconds * 10000 +
			src.hundredsOfMicroseconds * 100 + src.microseconds);
	} else {
		dest->tv_nsec = 0;
	}
}

void sevenzip_udf_stamp(const void *raw, struct sevenzip_udf_time *output)
{
    struct timestamp stamp;
    struct timespec64 time;
    memcpy(&stamp, raw, sizeof(stamp));
    udf_disk_stamp_to_time(&time, stamp);
    output->seconds = time.tv_sec;
    output->nanoseconds = (uint32_t)time.tv_nsec;
}

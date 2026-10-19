// SPDX-License-Identifier: GPL-2.0-only
/*
 * directory.c
 *
 * PURPOSE
 *	Directory related functions
 *
 */

#include "udfdecl.h"

struct short_ad *udf_get_fileshortad(uint8_t *ptr, int maxoffset, uint32_t *offset,
			      int inc)
{
	struct short_ad *sa;

	if ((!ptr) || (!offset)) {
		pr_err("%s: invalidparms\n", __func__);
		return NULL;
	}

	if ((*offset + sizeof(struct short_ad)) > maxoffset)
		return NULL;
	else {
		sa = (struct short_ad *)ptr;
		if (sa->extLength == 0)
			return NULL;
	}

	if (inc)
		*offset += sizeof(struct short_ad);
	return sa;
}

struct long_ad *udf_get_filelongad(uint8_t *ptr, int maxoffset, uint32_t *offset, int inc)
{
	struct long_ad *la;

	if ((!ptr) || (!offset)) {
		pr_err("%s: invalidparms\n", __func__);
		return NULL;
	}

	if ((*offset + sizeof(struct long_ad)) > maxoffset)
		return NULL;
	else {
		la = (struct long_ad *)ptr;
		if (la->extLength == 0)
			return NULL;
	}

	if (inc)
		*offset += sizeof(struct long_ad);
	return la;
}

// SPDX-License-Identifier: GPL-2.0-only
/*
 * partition.c
 *
 * PURPOSE
 *      Partition handling routines for the OSTA-UDF(tm) filesystem.
 *
 * COPYRIGHT
 *  (C) 1998-2001 Ben Fennema
 *
 * HISTORY
 *
 * 12/06/98 blf  Created file.
 *
 */

#include "udfdecl.h"

uint32_t udf_get_pblock_spar15(struct super_block *sb, uint32_t block,
			       uint16_t partition, uint32_t offset)
{
	int i;
	struct sparingTable *st = NULL;
	struct udf_sb_info *sbi = UDF_SB(sb);
	struct udf_part_map *map;
	uint32_t packet;
	struct udf_sparing_data *sdata;

	map = &sbi->s_partmaps[partition];
	sdata = &map->s_type_specific.s_sparing;
	if (offset > UINT32_MAX - block)
		return UINT32_MAX;
	packet = (block + offset) & ~(sdata->s_packet_len - 1);

	for (i = 0; i < 4; i++) {
		if (sdata->s_spar_map[i] != NULL) {
			st = (struct sparingTable *)
					sdata->s_spar_map[i]->b_data;
			break;
		}
	}

	if (st) {
		for (i = 0; i < le16_to_cpu(st->reallocationTableLen); i++) {
			struct sparingEntry *entry = &st->mapEntry[i];
			u32 origLoc = le32_to_cpu(entry->origLocation);
			if (origLoc >= 0xFFFFFFF0)
				break;
			else if (origLoc == packet) {
				u32 mapped = le32_to_cpu(entry->mappedLocation);
				u32 within = (block + offset) & (sdata->s_packet_len - 1);
				if (mapped == UINT32_MAX || within > UINT32_MAX - mapped)
					return UINT32_MAX;
				return mapped + within;
			}
			else if (origLoc > packet)
				break;
		}
	}

	if (map->s_partition_root > UINT32_MAX - (block + offset))
		return UINT32_MAX;
	return map->s_partition_root + block + offset;
}

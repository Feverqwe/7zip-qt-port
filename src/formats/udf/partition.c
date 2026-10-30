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

#include "formats/udf/udfdecl.h"

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

uint32_t udf_get_pblock(struct super_block *sb, uint32_t block,
			uint16_t partition, uint32_t offset)
{
	struct udf_sb_info *sbi = UDF_SB(sb);
	struct udf_part_map *map;
	if (partition >= sbi->s_partitions) {
		udf_debug("block=%u, partition=%u, offset=%u: invalid partition\n",
			  block, partition, offset);
		return 0xFFFFFFFF;
	}
	map = &sbi->s_partmaps[partition];
	if (map->s_partition_func)
		return map->s_partition_func(sb, block, partition, offset);
	else {
		uint64_t physical = (uint64_t)map->s_partition_root + block + offset;
		return physical < UINT32_MAX ? (uint32_t)physical : UINT32_MAX;
	}
}

uint32_t udf_get_pblock_virt15(struct super_block *sb, uint32_t block,
			       uint16_t partition, uint32_t offset)
{
	struct buffer_head *bh = NULL;
	uint32_t newblock;
	uint32_t index;
	uint32_t loc;
	struct udf_sb_info *sbi = UDF_SB(sb);
	struct udf_part_map *map;
	struct udf_virtual_data *vdata;
	struct udf_inode_info *iinfo = UDF_I(sbi->s_vat_inode);
	int err;

	map = &sbi->s_partmaps[partition];
	vdata = &map->s_type_specific.s_virtual;

	if (block >= vdata->s_num_entries) {
		udf_debug("Trying to access block beyond end of VAT (%u max %u)\n",
			  block, vdata->s_num_entries);
		return 0xFFFFFFFF;
	}

	if (iinfo->i_alloc_type == ICBTAG_FLAG_AD_IN_ICB) {
		__le32 cell;
		memcpy(&cell, iinfo->i_data + vdata->s_start_offset + (uint64_t)block * 4, 4);
		loc = le32_to_cpu(cell);
		goto translate;
	}
	/* Normalize extended headers without unsigned first-block subtraction. */
	{
		uint64_t position = vdata->s_start_offset + (uint64_t)block * sizeof(uint32_t);
		newblock = (uint32_t)(position / sb->s_blocksize);
		index = (uint32_t)(position % sb->s_blocksize) / sizeof(uint32_t);
	}

	bh = udf_bread(sbi->s_vat_inode, newblock, 0, &err);
	if (!bh) {
		udf_debug("get_pblock(UDF_VIRTUAL_MAP:%p,%u,%u)\n",
			  sb, block, partition);
		return 0xFFFFFFFF;
	}

	loc = le32_to_cpu(((__le32 *)bh->b_data)[index]);

	brelse(bh);

translate:
	if (iinfo->i_location.partitionReferenceNum == partition) {
		udf_debug("recursive call to udf_get_pblock!\n");
		return 0xFFFFFFFF;
	}

	return udf_get_pblock(sb, loc,
			      iinfo->i_location.partitionReferenceNum,
			      offset);
}

uint32_t udf_get_pblock_virt20(struct super_block *sb, uint32_t block,
				      uint16_t partition, uint32_t offset)
{
	return udf_get_pblock_virt15(sb, block, partition, offset);
}

static uint32_t udf_try_read_meta(struct inode *inode, uint32_t block,
					uint16_t partition, uint32_t offset)
{
	struct super_block *sb = inode->i_sb;
	struct udf_part_map *map;
	struct kernel_lb_addr eloc;
	uint32_t elen;
	sector_t ext_offset;
	struct extent_position epos = {0};
	uint32_t phyblock;
	int8_t etype;
	int err = 0;

	err = inode_bmap(inode, block, &epos, &eloc, &elen, &ext_offset, &etype);
	if (err <= 0 || etype != (EXT_RECORDED_ALLOCATED >> 30))
		phyblock = 0xFFFFFFFF;
	else {
		map = &UDF_SB(sb)->s_partmaps[partition];
		/* map to sparable/physical partition desc */
		phyblock = udf_get_pblock(sb, eloc.logicalBlockNum,
			map->s_type_specific.s_metadata.s_phys_partition_ref,
			ext_offset + offset);
	}

	brelse(epos.bh);
	return phyblock;
}

uint32_t udf_get_pblock_meta25(struct super_block *sb, uint32_t block,
				uint16_t partition, uint32_t offset)
{
	struct udf_sb_info *sbi = UDF_SB(sb);
	struct udf_part_map *map;
	struct udf_meta_data *mdata;
	uint32_t retblk;
	struct inode *inode;

	udf_debug("READING from METADATA\n");

	map = &sbi->s_partmaps[partition];
	mdata = &map->s_type_specific.s_metadata;
	inode = mdata->s_metadata_fe ? mdata->s_metadata_fe : mdata->s_mirror_fe;

	if (!inode)
		return 0xFFFFFFFF;

	retblk = udf_try_read_meta(inode, block, partition, offset);
	if (retblk == 0xFFFFFFFF && mdata->s_metadata_fe) {
		udf_warn(sb, "error reading from METADATA, trying to read from MIRROR\n");
		if (!(mdata->s_flags & MF_MIRROR_FE_LOADED)) {
			mdata->s_mirror_fe = udf_find_metadata_inode_efe(sb,
				mdata->s_mirror_file_loc,
				mdata->s_phys_partition_ref);
			if (IS_ERR(mdata->s_mirror_fe))
				mdata->s_mirror_fe = NULL;
			mdata->s_flags |= MF_MIRROR_FE_LOADED;
		}

		inode = mdata->s_mirror_fe;
		if (!inode)
			return 0xFFFFFFFF;
		retblk = udf_try_read_meta(inode, block, partition, offset);
	}

	return retblk;
}

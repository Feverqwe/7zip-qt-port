// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2012-2013 Samsung Electronics Co., Ltd.
 */

/* Read-only userspace adaptation; see docs/upstream-port-audit/exfat-linux.md. */

#include "exfat_fs.h"

#define __le_long __le64
#define BITS_PER_LONG 64
#define lel_to_cpu(A) le64_to_cpu(A)
#define BITMAP_LAST_WORD_MASK(nbits) (UINT64_MAX >> (-(nbits) & (BITS_PER_LONG - 1)))
unsigned long __sw_hweight64(uint64_t);
#define hweight_long(word) __sw_hweight64(word)

static bool exfat_test_bitmap_range(struct super_block *sb, unsigned int clu,
		unsigned int count)
{
	struct exfat_sb_info *sbi = EXFAT_SB(sb);
	unsigned int start = clu;
	unsigned int end = clu + count;
	unsigned int ent_idx, i, b;
	unsigned int bit_offset, bits_to_check;
	__le_long *bitmap_le;
	u64 mask, word; /* LLP64 hosts still use 64-bit bitmap words. */

	if (!is_valid_cluster(sbi, start) || !is_valid_cluster(sbi, end - 1))
		return false;

	while (start < end) {
		ent_idx = CLUSTER_TO_BITMAP_ENT(start);
		i = BITMAP_OFFSET_SECTOR_INDEX(sb, ent_idx);
		b = BITMAP_OFFSET_BIT_IN_SECTOR(sb, ent_idx);

		bitmap_le = (__le_long *)sbi->vol_amap[i]->b_data;

		/* Calculate how many bits we can check in the current word */
		bit_offset = b % BITS_PER_LONG;
		bits_to_check = min(end - start,
				    (unsigned int)(BITS_PER_LONG - bit_offset));

		/* Create a bitmask for the range of bits to check */
		if (bits_to_check >= BITS_PER_LONG)
			mask = UINT64_MAX;
		else
			mask = ((UINT64_C(1) << bits_to_check) - 1) << bit_offset;
		word = lel_to_cpu(bitmap_le[b / BITS_PER_LONG]);

		/* Check if all bits in the mask are set */
		if ((word & mask) != mask)
			return false;

		start += bits_to_check;
	}

	return true;
}

static int exfat_allocate_bitmap(struct super_block *sb,
		struct exfat_dentry *ep)
{
	struct exfat_sb_info *sbi = EXFAT_SB(sb);
	long long map_size;
	unsigned int i, j, need_map_size, map_clusters, cluster;
	struct exfat_chain chain;
	sector_t sector;

	sbi->map_clu = le32_to_cpu(ep->dentry.bitmap.start_clu);
	map_size = le64_to_cpu(ep->dentry.bitmap.size);
	need_map_size = ((EXFAT_DATA_CLUSTER_COUNT(sbi) - 1) / BITS_PER_BYTE)
		+ 1;
	if (need_map_size != map_size) {
		exfat_err(sb, "bogus allocation bitmap size(need : %u, cur : %lld)",
			  need_map_size, map_size);
		/*
		 * Only allowed when bogus allocation
		 * bitmap size is large
		 */
		if (need_map_size > map_size)
			return -EIO;
	}
	if (map_size > exfat_cluster_to_bytes(sbi, EXFAT_DATA_CLUSTER_COUNT(sbi)) ||
	    !is_valid_cluster(sbi, sbi->map_clu))
		return -EIO;
	map_clusters = exfat_bytes_to_cluster_round_up(sbi, map_size);
	exfat_chain_set(&chain, sbi->map_clu, 0, ALLOC_FAT_CHAIN);
	if (exfat_count_num_clusters(sb, &chain, &chain.size) ||
	    chain.size < map_clusters)
		return -EIO;
	sbi->map_sectors = ((need_map_size - 1) >>
			(sb->s_blocksize_bits)) + 1;
	sbi->vol_amap = kvmalloc_objs(struct buffer_head *, sbi->map_sectors);
	if (!sbi->vol_amap)
		return -ENOMEM;

	cluster = sbi->map_clu;
	sector = exfat_cluster_to_sector(sbi, cluster);

	for (i = 0; i < sbi->map_sectors; i++) {
		/* Allocation Bitmaps have FAT chains, including on TexFAT. */
		if (i && !(i & (sbi->sect_per_clus - 1))) {
			if (exfat_get_next_cluster(sb, &cluster))
				goto err_out;
			sector = exfat_cluster_to_sector(sbi, cluster);
		}
		sbi->vol_amap[i] = sb_bread(sb, sector++);
		if (!sbi->vol_amap[i])
			goto err_out;
	}

	cluster = sbi->map_clu;
	for (j = 0; j < map_clusters; j++) {
		if (!exfat_test_bitmap_range(sb, cluster, 1))
			goto err_out;
		if (j + 1 < map_clusters && exfat_get_next_cluster(sb, &cluster))
			goto err_out;
	}

	return 0;

err_out:
	j = 0;
	/* release all buffers and free vol_amap */
	while (j < i)
		brelse(sbi->vol_amap[j++]);

	kvfree(sbi->vol_amap);
	sbi->vol_amap = NULL;
	return -EIO;
}

int exfat_load_bitmap(struct super_block *sb)
{
	unsigned int i, type, seen = 0;
	struct exfat_chain clu;
	struct exfat_sb_info *sbi = EXFAT_SB(sb);
	struct exfat_dentry selected = {0};
	const unsigned int expected = sbi->FAT1_start_sector == sbi->FAT2_start_sector ? 1 : 3;

	exfat_chain_set(&clu, sbi->root_dir, 0, ALLOC_FAT_CHAIN);
	while (clu.dir != EXFAT_EOF_CLUSTER) {
		for (i = 0; i < sbi->dentries_per_clu; i++) {
			struct exfat_dentry *ep;
			struct buffer_head *bh;

			ep = exfat_get_dentry(sb, &clu, i, &bh);
			if (!ep)
				return -EIO;

			type = exfat_get_entry_type(ep);
			if (type == TYPE_BITMAP) {
				unsigned int id = ep->dentry.bitmap.flags & 1;
				if ((seen & (1u << id)) || !(expected & (1u << id))) {
					brelse(bh);
					return -EINVAL;
				}
				seen |= 1u << id;
				if (id == sbi->active_fat)
					memcpy(&selected, ep, sizeof(selected));
			}
			brelse(bh);

			if (type == TYPE_UNUSED)
				goto load;
		}

		if (exfat_get_next_cluster(sb, &clu.dir))
			return -EIO;
	}

load:
	if (seen != expected)
		return -EINVAL;
	/* The inactive FAT/bitmap are stale; never use them as repair fallbacks. */
	return exfat_allocate_bitmap(sb, &selected);
}

void exfat_free_bitmap(struct exfat_sb_info *sbi)
{
	int i;

	for (i = 0; i < sbi->map_sectors; i++)
		__brelse(sbi->vol_amap[i]);

	kvfree(sbi->vol_amap);
}

bool exfat_test_bitmap(struct super_block *sb, unsigned int clu)
{
	int i, b;
	unsigned int ent_idx;
	struct exfat_sb_info *sbi = EXFAT_SB(sb);

	if (!sbi->vol_amap)
		return true;

	if (!is_valid_cluster(sbi, clu))
		return false;

	ent_idx = CLUSTER_TO_BITMAP_ENT(clu);
	i = BITMAP_OFFSET_SECTOR_INDEX(sb, ent_idx);
	b = BITMAP_OFFSET_BIT_IN_SECTOR(sb, ent_idx);

	if (!test_bit_le(b, sbi->vol_amap[i]->b_data))
		return false;

	return true;
}

int exfat_count_used_clusters(struct super_block *sb, unsigned int *ret_count)
{
	struct exfat_sb_info *sbi = EXFAT_SB(sb);
	unsigned int count = 0;
	unsigned int i, map_i = 0, map_b = 0;
	unsigned int total_clus = EXFAT_DATA_CLUSTER_COUNT(sbi);
	unsigned int last_mask = total_clus & (BITS_PER_LONG - 1);
	u64 *bitmap, clu_bits; /* Match the fixed 64-bit bitmap service on LLP64. */

	total_clus &= ~last_mask;
	for (i = 0; i < total_clus; i += BITS_PER_LONG) {
		/* Archive cancellation also applies to cached bitmap scans. */
		if (!map_b && sb->poll && sb->poll(sb->context))
			return -EIO;
		bitmap = (void *)(sbi->vol_amap[map_i]->b_data + map_b);
		count += hweight_long(*bitmap);
		map_b += sizeof(u64);
		if (map_b >= (unsigned int)sb->s_blocksize) {
			map_i++;
			map_b = 0;
		}
	}

	if (last_mask) {
		bitmap = (void *)(sbi->vol_amap[map_i]->b_data + map_b);
		clu_bits = lel_to_cpu(*(__le_long *)bitmap);
		count += hweight_long(clu_bits & BITMAP_LAST_WORD_MASK(last_mask));
	}

	*ret_count = count;
	return 0;
}

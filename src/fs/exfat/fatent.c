// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2012-2013 Samsung Electronics Co., Ltd.
 */

/* Read-only userspace adaptation; see docs/upstream-port-audit/exfat-linux.md. */

#include "exfat_fs.h"

static int __exfat_ent_get(struct super_block *sb, unsigned int loc,
		unsigned int *content, struct buffer_head **cache)
{
	unsigned int off;
	sector_t sec;
	struct buffer_head *bh = *cache;

	sec = FAT_ENT_OFFSET_SECTOR(sb, loc);
	off = FAT_ENT_OFFSET_BYTE_IN_SECTOR(sb, loc);

	if (!bh || bh->b_blocknr != sec || !buffer_uptodate(bh)) {
		brelse(bh);
		bh = sb_bread(sb, sec);
		*cache = bh;
		if (unlikely(!bh))
			return -EIO;
	}

	*content = le32_to_cpu(*(__le32 *)(&bh->b_data[off]));

	/* remap reserved clusters to simplify code */
	if (*content > EXFAT_BAD_CLUSTER)
		*content = EXFAT_EOF_CLUSTER;

	return 0;
}

int exfat_ent_get(struct super_block *sb, unsigned int loc,
		unsigned int *content, struct buffer_head **cache)
{
	struct exfat_sb_info *sbi = EXFAT_SB(sb);

	if (!is_valid_cluster(sbi, loc)) {
		exfat_fs_error_ratelimit(sb,
			"invalid access to FAT (entry 0x%08x)",
			loc);
		goto err;
	}

	if (unlikely(__exfat_ent_get(sb, loc, content, cache))) {
		exfat_fs_error_ratelimit(sb,
			"failed to access to FAT (entry 0x%08x)",
			loc);
		goto err;
	}

	if (unlikely(*content == EXFAT_FREE_CLUSTER)) {
		exfat_fs_error_ratelimit(sb,
			"invalid access to FAT free cluster (entry 0x%08x)",
			loc);
		goto err;
	}

	if (unlikely(*content == EXFAT_BAD_CLUSTER)) {
		exfat_fs_error_ratelimit(sb,
			"invalid access to FAT bad cluster (entry 0x%08x)",
			loc);
		goto err;
	}

	if (*content != EXFAT_EOF_CLUSTER && !is_valid_cluster(sbi, *content)) {
		exfat_fs_error_ratelimit(sb,
			"invalid access to FAT (entry 0x%08x) bogus content (0x%08x)",
			loc, *content);
		goto err;
	}

	return 0;

err:
	/* Avoid double release */
	brelse(*cache);
	*cache = NULL;
	return -EIO;
}

int exfat_count_num_clusters(struct super_block *sb,
		struct exfat_chain *p_chain, unsigned int *ret_count)
{
	unsigned int i, count;
	unsigned int clu;
	struct exfat_sb_info *sbi = EXFAT_SB(sb);
	struct buffer_head *bh = NULL;

	if (!p_chain->dir || p_chain->dir == EXFAT_EOF_CLUSTER) {
		*ret_count = 0;
		return 0;
	}

	if (p_chain->flags == ALLOC_NO_FAT_CHAIN) {
		*ret_count = p_chain->size;
		return 0;
	}

	clu = p_chain->dir;
	count = 0;
	for (i = EXFAT_FIRST_CLUSTER; i < sbi->num_clusters; i++) {
		count++;
		if (exfat_ent_get(sb, clu, &clu, &bh))
			return -EIO;
		if (clu == EXFAT_EOF_CLUSTER)
			break;
	}

	brelse(bh);
	*ret_count = count;

	/*
	 * since exfat_count_used_clusters() is not called, sbi->used_clusters
	 * cannot be used here.
	 */
	if (unlikely(i == sbi->num_clusters && clu != EXFAT_EOF_CLUSTER)) {
		exfat_fs_error(sb, "The cluster chain has a loop");
		return -EIO;
	}

	return 0;
}

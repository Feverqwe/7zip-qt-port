// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2012-2013 Samsung Electronics Co., Ltd.
 */

/* Read-only userspace adaptation; see docs/upstream-port-audit/exfat-linux.md. */

#include "formats/exfat/exfat_fs.h"

static int exfat_calibrate_blocksize(struct super_block *sb, int logical_sect)
{
	struct exfat_sb_info *sbi = EXFAT_SB(sb);

	if (!is_power_of_2(logical_sect)) {
		exfat_err(sb, "bogus logical sector size %u", logical_sect);
		return -EIO;
	}

	if (logical_sect < sb->s_blocksize) {
		exfat_err(sb, "logical sector size too small for device (logical sector size = %u)",
			  logical_sect);
		return -EIO;
	}

	if (logical_sect > sb->s_blocksize) {
		brelse(sbi->boot_bh);
		sbi->boot_bh = NULL;

		if (!sb_set_blocksize(sb, logical_sect)) {
			exfat_err(sb, "unable to set blocksize %u",
				  logical_sect);
			return -EIO;
		}
		sbi->boot_bh = sb_bread(sb, 0);
		if (!sbi->boot_bh) {
			exfat_err(sb, "unable to read boot sector (logical sector size = %lu)",
				  sb->s_blocksize);
			return -EIO;
		}
	}
	return 0;
}

int exfat_read_boot_sector(struct super_block *sb)
{
	struct boot_sector *p_boot;
	struct exfat_sb_info *sbi = EXFAT_SB(sb);

	/* set block size to read super block */
	if (!sb_min_blocksize(sb, 512)) {
		exfat_err(sb, "unable to set blocksize");
		return -EINVAL;
	}

	/* read boot sector */
	sbi->boot_bh = sb_bread(sb, 0);
	if (!sbi->boot_bh) {
		exfat_err(sb, "unable to read boot sector");
		return -EIO;
	}
	p_boot = (struct boot_sector *)sbi->boot_bh->b_data;

	/* check the validity of BOOT */
	if (le16_to_cpu((p_boot->signature)) != BOOT_SIGNATURE) {
		exfat_err(sb, "invalid boot record signature");
		return -EINVAL;
	}

	if (memcmp(p_boot->fs_name, STR_EXFAT, BOOTSEC_FS_NAME_LEN)) {
		exfat_err(sb, "invalid fs_name"); /* fs_name may unprintable */
		return -EINVAL;
	}

	/*
	 * must_be_zero field must be filled with zero to prevent mounting
	 * from FAT volume.
	 */
	if (memchr_inv(p_boot->must_be_zero, 0, sizeof(p_boot->must_be_zero)))
		return -EINVAL;

	if (p_boot->num_fats != 1 && p_boot->num_fats != 2) {
		exfat_err(sb, "bogus number of FAT structure");
		return -EINVAL;
	}

	/*
	 * sect_size_bits could be at least 9 and at most 12.
	 */
	if (p_boot->sect_size_bits < EXFAT_MIN_SECT_SIZE_BITS ||
	    p_boot->sect_size_bits > EXFAT_MAX_SECT_SIZE_BITS) {
		exfat_err(sb, "bogus sector size bits : %u",
				p_boot->sect_size_bits);
		return -EINVAL;
	}

	/*
	 * sect_per_clus_bits could be at least 0 and at most 25 - sect_size_bits.
	 */
	if (p_boot->sect_per_clus_bits > EXFAT_MAX_SECT_PER_CLUS_BITS(p_boot)) {
		exfat_err(sb, "bogus sectors bits per cluster : %u",
				p_boot->sect_per_clus_bits);
		return -EINVAL;
	}

	sbi->sect_per_clus = 1 << p_boot->sect_per_clus_bits;
	sbi->sect_per_clus_bits = p_boot->sect_per_clus_bits;
	sbi->cluster_size_bits = p_boot->sect_per_clus_bits +
		p_boot->sect_size_bits;
	sbi->cluster_size = 1 << sbi->cluster_size_bits;
	sbi->num_FAT_sectors = le32_to_cpu(p_boot->fat_length);
	sbi->FAT1_start_sector = le32_to_cpu(p_boot->fat_offset);
	sbi->FAT2_start_sector = le32_to_cpu(p_boot->fat_offset);
	if (p_boot->num_fats == 2)
		sbi->FAT2_start_sector += sbi->num_FAT_sectors;
	sbi->data_start_sector = le32_to_cpu(p_boot->clu_offset);
	sbi->data_start_bytes = sbi->data_start_sector << p_boot->sect_size_bits;
	sbi->num_sectors = le64_to_cpu(p_boot->vol_length);
	/* because the cluster index starts with 2 */
	sbi->num_clusters = le32_to_cpu(p_boot->clu_count) +
		EXFAT_RESERVED_CLUSTERS;

	sbi->root_dir = le32_to_cpu(p_boot->root_cluster);
	sbi->dentries_per_clu = 1 <<
		(sbi->cluster_size_bits - DENTRY_SIZE_BITS);

	sbi->vol_flags = le16_to_cpu(p_boot->vol_flags);
	sbi->active_fat = sbi->vol_flags & VOLUME_ACTIVE_FAT;
	sbi->vol_flags_persistent = sbi->vol_flags & (VOLUME_DIRTY | MEDIA_FAILURE);
	sbi->clu_srch_ptr = EXFAT_FIRST_CLUSTER;

	/* check consistencies */
	if ((u64)sbi->num_FAT_sectors << p_boot->sect_size_bits <
	    (u64)sbi->num_clusters * 4) {
		exfat_err(sb, "bogus fat length");
		return -EINVAL;
	}

	if (sbi->data_start_sector <
	    (u64)sbi->FAT1_start_sector +
	    (u64)sbi->num_FAT_sectors * p_boot->num_fats) {
		exfat_err(sb, "bogus data start sector");
		return -EINVAL;
	}

	if (sbi->vol_flags & VOLUME_DIRTY)
		exfat_warn(sb, "Volume was not properly unmounted. Some data may be corrupt. Please run fsck.");
	if (sbi->vol_flags & MEDIA_FAILURE)
		exfat_warn(sb, "Medium has reported failures. Some data may be lost.");

	/*
	 * Set to the max possible volume size for this volume's cluster size so
	 * that any integer overflow from bytes to cluster size conversion is
	 * checked in inode_newsize_ok(). Clamped to MAX_LFS_FILESIZE for 32-bit
	 * machines.
	 */
	sb->s_maxbytes = min(MAX_LFS_FILESIZE,
			     exfat_cluster_to_bytes(sbi, (loff_t)EXFAT_MAX_NUM_CLUSTER));

	/* check logical sector size */
	if (exfat_calibrate_blocksize(sb, 1 << p_boot->sect_size_bits))
		return -EIO;

	return 0;
}

int exfat_verify_boot_region(struct super_block *sb)
{
	struct buffer_head *bh = NULL;
	u32 chksum = 0;
	__le32 *p_sig, *p_chksum;
	int sn, i;

	/* read boot sector sub-regions */
	for (sn = 0; sn < 11; sn++) {
		bh = sb_bread(sb, sn);
		if (!bh)
			return -EIO;

		if (sn != 0 && sn <= 8) {
			/* extended boot sector sub-regions */
			p_sig = (__le32 *)&bh->b_data[sb->s_blocksize - 4];
			if (le32_to_cpu(*p_sig) != EXBOOT_SIGNATURE) {
				sb->boot_warning = true;
				exfat_warn(sb, "Invalid exboot-signature(sector = %d): 0x%08x",
					   sn, le32_to_cpu(*p_sig));
			}
		}

		chksum = exfat_calc_chksum32(bh->b_data, sb->s_blocksize,
			chksum, sn ? CS_DEFAULT : CS_BOOT_SECTOR);
		brelse(bh);
	}

	/* boot checksum sub-regions */
	bh = sb_bread(sb, sn);
	if (!bh)
		return -EIO;

	for (i = 0; i < sb->s_blocksize; i += sizeof(u32)) {
		p_chksum = (__le32 *)&bh->b_data[i];
		if (le32_to_cpu(*p_chksum) != chksum) {
			exfat_err(sb, "Invalid boot checksum (boot checksum : 0x%08x, checksum : 0x%08x)",
				  le32_to_cpu(*p_chksum), chksum);
			brelse(bh);
			return -EINVAL;
		}
	}
	brelse(bh);
	return 0;
}

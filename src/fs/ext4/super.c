// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/fs/ext4/super.c
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/inode.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Big-endian to little-endian byte-swapping/bitmaps by
 *        David S. Miller (davem@caip.rutgers.edu), 1995
 */

#include "ext4.h"

/* Read-only archive subset: no write mounts, VFS probing or DAX. Casefold
 * lookup is not performed; directory entries retain their stored names. */
static int ext4_feature_set_ok(struct super_block *sb, int readonly)
{
	if (ext4_has_unknown_ext4_incompat_features(sb))
		return 0;

	if (readonly)
		return 1;

	return 0;
}

int ext4_check_feature_compatibility(struct super_block *sb,
				   struct ext4_super_block *es)
{
	if (es->s_creator_os == cpu_to_le32(EXT4_OS_HURD)) {
		if (ext4_has_feature_64bit(sb))
			return -EINVAL;

		/*
		 * ea_inode feature uses l_i_version field which is not
		 * available in HURD_COMPAT mode.
		 */
		if (ext4_has_feature_ea_inode(sb))
			return -EINVAL;
	}

	if (!ext4_feature_set_ok(sb, 1))
		return -EINVAL;

	if (ext4_has_feature_encrypt(sb) && es->s_encryption_level)
		return -EINVAL;

	return 0;
}

__le32 ext4_superblock_csum(struct ext4_super_block *es)
{
	int offset = offsetof(struct ext4_super_block, s_checksum);
	__u32 csum;

	csum = ext4_chksum(~0, (char *)es, offset);

	return cpu_to_le32(csum);
}

int ext4_superblock_csum_verify(struct super_block *sb,
				       struct ext4_super_block *es)
{
	if (!ext4_has_feature_metadata_csum(sb))
		return 1;

	return es->s_checksum == ext4_superblock_csum(es);
}

static __le16 ext4_group_desc_csum(struct super_block *sb, __u32 block_group,
				   struct ext4_group_desc *gdp)
{
	int offset = offsetof(struct ext4_group_desc, bg_checksum);
	__u16 crc = 0;
	__le32 le_group = cpu_to_le32(block_group);
	struct ext4_sb_info *sbi = EXT4_SB(sb);

	if (ext4_has_feature_metadata_csum(sbi->s_sb)) {
		/* Use new metadata_csum algorithm */
		__u32 csum32;
		__u16 dummy_csum = 0;

		csum32 = ext4_chksum(sbi->s_csum_seed, (__u8 *)&le_group,
				     sizeof(le_group));
		csum32 = ext4_chksum(csum32, (__u8 *)gdp, offset);
		csum32 = ext4_chksum(csum32, (__u8 *)&dummy_csum,
				     sizeof(dummy_csum));
		offset += sizeof(dummy_csum);
		if (offset < sbi->s_desc_size)
			csum32 = ext4_chksum(csum32, (__u8 *)gdp + offset,
					     sbi->s_desc_size - offset);

		crc = csum32 & 0xFFFF;
		goto out;
	}

	/* old crc16 code */
	if (!ext4_has_feature_gdt_csum(sb))
		return 0;

	crc = crc16(~0, sbi->s_es->s_uuid, sizeof(sbi->s_es->s_uuid));
	crc = crc16(crc, (__u8 *)&le_group, sizeof(le_group));
	crc = crc16(crc, (__u8 *)gdp, offset);
	offset += sizeof(gdp->bg_checksum); /* skip checksum */
	/* for checksum of struct ext4_group_desc do the rest...*/
	if (ext4_has_feature_64bit(sb) && offset < sbi->s_desc_size)
		crc = crc16(crc, (__u8 *)gdp + offset,
			    sbi->s_desc_size - offset);

out:
	return cpu_to_le16(crc);
}

int ext4_group_desc_csum_verify(struct super_block *sb, __u32 block_group,
				struct ext4_group_desc *gdp)
{
	if (ext4_has_group_desc_csum(sb) &&
	    (gdp->bg_checksum != ext4_group_desc_csum(sb, block_group, gdp)))
		return 0;

	return 1;
}

ext4_fsblk_t descriptor_loc(struct super_block *sb,
				   ext4_fsblk_t logical_sb_block, int nr)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	ext4_group_t bg, first_meta_bg;
	int has_super = 0;

	first_meta_bg = le32_to_cpu(sbi->s_es->s_first_meta_bg);

	if (!ext4_has_feature_meta_bg(sb) || nr < first_meta_bg)
		return logical_sb_block + nr + 1;
	bg = sbi->s_desc_per_block * nr;
	if (ext4_bg_has_super(sb, bg))
		has_super = 1;

	/*
	 * If we have a meta_bg fs with 1k blocks, group 0's GDT is at
	 * block 2, not 1.  If s_first_data_block == 0 (bigalloc is enabled
	 * on modern mke2fs or blksize > 1k on older mke2fs) then we must
	 * compensate.
	 */
	if (sb->s_blocksize == 1024 && nr == 0 &&
	    le32_to_cpu(sbi->s_es->s_first_data_block) == 0)
		has_super++;

	return (has_super + ext4_group_first_block_no(sb, bg));
}

loff_t ext4_max_size(int blkbits, int has_huge_files)
{
	loff_t res;
	loff_t upper_limit = MAX_LFS_FILESIZE;

	/* Host archive block counts are explicitly u64, not VFS blkcnt_t. */

	if (!has_huge_files) {
		upper_limit = (1LL << 32) - 1;

		/* total blocks in file system block size */
		upper_limit >>= (blkbits - 9);
		upper_limit <<= blkbits;
	}

	/*
	 * 32-bit extent-start container, ee_block. We lower the maxbytes
	 * by one fs block, so ee_len can cover the extent of maximum file
	 * size
	 */
	res = (1LL << 32) - 1;
	res <<= blkbits;

	/* Sanity check against vm- & vfs- imposed limits */
	if (res > upper_limit)
		res = upper_limit;

	return res;
}

ext4_fsblk_t ext4_block_bitmap(struct super_block *sb,
			       struct ext4_group_desc *bg)
{
	return le32_to_cpu(bg->bg_block_bitmap_lo) |
		(EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
		 (ext4_fsblk_t)le32_to_cpu(bg->bg_block_bitmap_hi) << 32 : 0);
}

ext4_fsblk_t ext4_inode_bitmap(struct super_block *sb,
			       struct ext4_group_desc *bg)
{
	return le32_to_cpu(bg->bg_inode_bitmap_lo) |
		(EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
		 (ext4_fsblk_t)le32_to_cpu(bg->bg_inode_bitmap_hi) << 32 : 0);
}

ext4_fsblk_t ext4_inode_table(struct super_block *sb,
			      struct ext4_group_desc *bg)
{
	return le32_to_cpu(bg->bg_inode_table_lo) |
		(EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
		 (ext4_fsblk_t)le32_to_cpu(bg->bg_inode_table_hi) << 32 : 0);
}

int ext4_check_descriptors(struct super_block *sb,
				  ext4_fsblk_t sb_block,
				  ext4_group_t *first_not_zeroed)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	ext4_fsblk_t first_block = le32_to_cpu(sbi->s_es->s_first_data_block);
	ext4_fsblk_t last_block;
	ext4_fsblk_t last_bg_block = sb_block + ext4_bg_num_gdb(sb, 0);
	ext4_fsblk_t block_bitmap;
	ext4_fsblk_t inode_bitmap;
	ext4_fsblk_t inode_table;
	int flexbg_flag = 0;
	ext4_group_t i, grp = sbi->s_groups_count;

	if (ext4_has_feature_flex_bg(sb))
		flexbg_flag = 1;

	ext4_debug("Checking group descriptors");

	for (i = 0; i < sbi->s_groups_count; i++) {
		struct ext4_group_desc *gdp = ext4_get_group_desc(sb, i, NULL);

		if (i == sbi->s_groups_count - 1 || flexbg_flag)
			last_block = ext4_blocks_count(sbi->s_es) - 1;
		else
			last_block = first_block +
				(EXT4_BLOCKS_PER_GROUP(sb) - 1);

		if ((grp == sbi->s_groups_count) &&
		   !(gdp->bg_flags & cpu_to_le16(EXT4_BG_INODE_ZEROED)))
			grp = i;

		block_bitmap = ext4_block_bitmap(sb, gdp);
		if (block_bitmap == sb_block) {
			ext4_msg(sb, KERN_ERR, "ext4_check_descriptors: "
				 "Block bitmap for group %u overlaps "
				 "superblock", i);
			if (!sb_rdonly(sb))
				return 0;
		}
		if (block_bitmap >= sb_block + 1 &&
		    block_bitmap <= last_bg_block) {
			ext4_msg(sb, KERN_ERR, "ext4_check_descriptors: "
				 "Block bitmap for group %u overlaps "
				 "block group descriptors", i);
			if (!sb_rdonly(sb))
				return 0;
		}
		if (block_bitmap < first_block || block_bitmap > last_block) {
			ext4_msg(sb, KERN_ERR, "ext4_check_descriptors: "
			       "Block bitmap for group %u not in group "
			       "(block %llu)!", i, block_bitmap);
			return 0;
		}
		inode_bitmap = ext4_inode_bitmap(sb, gdp);
		if (inode_bitmap == sb_block) {
			ext4_msg(sb, KERN_ERR, "ext4_check_descriptors: "
				 "Inode bitmap for group %u overlaps "
				 "superblock", i);
			if (!sb_rdonly(sb))
				return 0;
		}
		if (inode_bitmap >= sb_block + 1 &&
		    inode_bitmap <= last_bg_block) {
			ext4_msg(sb, KERN_ERR, "ext4_check_descriptors: "
				 "Inode bitmap for group %u overlaps "
				 "block group descriptors", i);
			if (!sb_rdonly(sb))
				return 0;
		}
		if (inode_bitmap < first_block || inode_bitmap > last_block) {
			ext4_msg(sb, KERN_ERR, "ext4_check_descriptors: "
			       "Inode bitmap for group %u not in group "
			       "(block %llu)!", i, inode_bitmap);
			return 0;
		}
		inode_table = ext4_inode_table(sb, gdp);
		if (inode_table == sb_block) {
			ext4_msg(sb, KERN_ERR, "ext4_check_descriptors: "
				 "Inode table for group %u overlaps "
				 "superblock", i);
			if (!sb_rdonly(sb))
				return 0;
		}
		if (inode_table >= sb_block + 1 &&
		    inode_table <= last_bg_block) {
			ext4_msg(sb, KERN_ERR, "ext4_check_descriptors: "
				 "Inode table for group %u overlaps "
				 "block group descriptors", i);
			if (!sb_rdonly(sb))
				return 0;
		}
		if (inode_table < first_block ||
		    inode_table + sbi->s_itb_per_group - 1 > last_block) {
			ext4_msg(sb, KERN_ERR, "ext4_check_descriptors: "
			       "Inode table for group %u not in group "
			       "(block %llu)!", i, inode_table);
			return 0;
		}
		ext4_lock_group(sb, i);
		if (!ext4_group_desc_csum_verify(sb, i, gdp)) {
			ext4_msg(sb, KERN_ERR, "ext4_check_descriptors: "
				 "Checksum for group %u failed (%u!=%u)",
				 i, le16_to_cpu(ext4_group_desc_csum(sb, i,
				     gdp)), le16_to_cpu(gdp->bg_checksum));
			if (!sb_rdonly(sb)) {
				ext4_unlock_group(sb, i);
				return 0;
			}
		}
		ext4_unlock_group(sb, i);
		if (!flexbg_flag)
			first_block += EXT4_BLOCKS_PER_GROUP(sb);
	}
	if (NULL != first_not_zeroed)
		*first_not_zeroed = grp;
	return 1;
}

loff_t ext4_max_bitmap_size(int bits, int has_huge_files)
{
	loff_t upper_limit, res = EXT4_NDIR_BLOCKS;
	int meta_blocks;
	unsigned int ppb = 1 << (bits - 2);

	/*
	 * This is calculated to be the largest file size for a dense, block
	 * mapped file such that the file's total number of 512-byte sectors,
	 * including data and all indirect blocks, does not exceed (2^48 - 1).
	 *
	 * __u32 i_blocks_lo and _u16 i_blocks_high represent the total
	 * number of 512-byte sectors of the file.
	 */
	if (!has_huge_files) {
		/*
		 * !has_huge_files or implies that the inode i_block field
		 * represents total file blocks in 2^32 512-byte sectors ==
		 * size of vfs inode i_blocks * 8
		 */
		upper_limit = (1LL << 32) - 1;

		/* total blocks in file system block size */
		upper_limit >>= (bits - 9);

	} else {
		/*
		 * We use 48 bit ext4_inode i_blocks
		 * With EXT4_HUGE_FILE_FL set the i_blocks
		 * represent total number of blocks in
		 * file system block size
		 */
		upper_limit = (1LL << 48) - 1;

	}

	/* Compute how many blocks we can address by block tree */
	res += ppb;
	res += ppb * ppb;
	res += ((loff_t)ppb) * ppb * ppb;
	/* Compute how many metadata blocks are needed */
	meta_blocks = 1;
	meta_blocks += 1 + ppb;
	meta_blocks += 1 + ppb + ppb * ppb;
	/* Does block tree limit file size? */
	if (res + meta_blocks <= upper_limit)
		goto check_lfs;

	res = upper_limit;
	/* How many metadata blocks are needed for addressing upper_limit? */
	upper_limit -= EXT4_NDIR_BLOCKS;
	/* indirect blocks */
	meta_blocks = 1;
	upper_limit -= ppb;
	/* double indirect blocks */
	if (upper_limit < ppb * ppb) {
		meta_blocks += 1 + DIV_ROUND_UP_ULL(upper_limit, ppb);
		res -= meta_blocks;
		goto check_lfs;
	}
	meta_blocks += 1 + ppb;
	upper_limit -= ppb * ppb;
	/* tripple indirect blocks for the rest */
	meta_blocks += 1 + DIV_ROUND_UP_ULL(upper_limit, ppb) +
		DIV_ROUND_UP_ULL(upper_limit, ppb*ppb);
	res -= meta_blocks;
check_lfs:
	res <<= bits;
	if (res > MAX_LFS_FILESIZE)
		res = MAX_LFS_FILESIZE;

	return res;
}

int ext4_inode_info_init(struct super_block *sb,
				struct ext4_super_block *es)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);

	if (le32_to_cpu(es->s_rev_level) == EXT4_GOOD_OLD_REV) {
		sbi->s_inode_size = EXT4_GOOD_OLD_INODE_SIZE;
		sbi->s_first_ino = EXT4_GOOD_OLD_FIRST_INO;
	} else {
		sbi->s_inode_size = le16_to_cpu(es->s_inode_size);
		sbi->s_first_ino = le32_to_cpu(es->s_first_ino);
		if (sbi->s_first_ino < EXT4_GOOD_OLD_FIRST_INO) {
			ext4_msg(sb, KERN_ERR, "invalid first ino: %u",
				 sbi->s_first_ino);
			return -EINVAL;
		}
		if ((sbi->s_inode_size < EXT4_GOOD_OLD_INODE_SIZE) ||
		    (!is_power_of_2(sbi->s_inode_size)) ||
		    (sbi->s_inode_size > sb->s_blocksize)) {
			ext4_msg(sb, KERN_ERR,
			       "unsupported inode size: %d",
			       sbi->s_inode_size);
			ext4_msg(sb, KERN_ERR, "blocksize: %lu", sb->s_blocksize);
			return -EINVAL;
		}
		/*
		 * i_atime_extra is the last extra field available for
		 * [acm]times in struct ext4_inode. Checking for that
		 * field should suffice to ensure we have extra space
		 * for all three.
		 */
		if (sbi->s_inode_size >= offsetof(struct ext4_inode, i_atime_extra) +
			sizeof(((struct ext4_inode *)0)->i_atime_extra)) {
			sb->s_time_gran = 1;
			sb->s_time_max = EXT4_EXTRA_TIMESTAMP_MAX;
		} else {
			sb->s_time_gran = NSEC_PER_SEC;
			sb->s_time_max = EXT4_NON_EXTRA_TIMESTAMP_MAX;
		}
		sb->s_time_min = EXT4_TIMESTAMP_MIN;
	}

	if (sbi->s_inode_size > EXT4_GOOD_OLD_INODE_SIZE) {
		sbi->s_want_extra_isize = sizeof(struct ext4_inode) -
			EXT4_GOOD_OLD_INODE_SIZE;
		if (ext4_has_feature_extra_isize(sb)) {
			unsigned v, max = (sbi->s_inode_size -
					   EXT4_GOOD_OLD_INODE_SIZE);

			v = le16_to_cpu(es->s_want_extra_isize);
			if (v > max) {
				ext4_msg(sb, KERN_ERR,
					 "bad s_want_extra_isize: %d", v);
				return -EINVAL;
			}
			if (sbi->s_want_extra_isize < v)
				sbi->s_want_extra_isize = v;

			v = le16_to_cpu(es->s_min_extra_isize);
			if (v > max) {
				ext4_msg(sb, KERN_ERR,
					 "bad s_min_extra_isize: %d", v);
				return -EINVAL;
			}
			if (sbi->s_want_extra_isize < v)
				sbi->s_want_extra_isize = v;
		}
	}

	return 0;
}

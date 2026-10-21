// SPDX-License-Identifier: GPL-2.0-only
/*
 *  linux/fs/fat/inode.c
 *
 *  Written 1992,1993 by Werner Almesberger
 *  VFAT extensions by Gordon Chaffee, merged with msdos fs by Henrik Storner
 *  Rewritten for the constant inumbers support by Al Viro
 *
 *  Fixes:
 *
 *	Max Cohan: Fixed invalid FSINFO offset when info_sector is 0
 */

#include "fat.h"

struct fat_bios_param_block {
	u8	fat_media;
	u16	fat_sector_size;
	u8	fat_sec_per_clus;
	u16	fat_reserved;
	u8	fat_fats;
	u16	fat_dir_entries;
	u16	fat_sectors;
	u16	fat_fat_length;
	u32	fat_total_sect;

	u8	fat16_state;
	u32	fat16_vol_id;

	u32	fat32_length;
	u32	fat32_root_cluster;
	u16	fat32_info_sector;
	u8	fat32_state;
	u32	fat32_vol_id;
};

static struct fat_floppy_defaults {
	unsigned nr_sectors;
	unsigned sec_per_clus;
	unsigned dir_entries;
	unsigned media;
	unsigned fat_length;
} floppy_defaults[] = {
{
	.nr_sectors = 160 * KB_IN_SECTORS,
	.sec_per_clus = 1,
	.dir_entries = 64,
	.media = 0xFE,
	.fat_length = 1,
},
{
	.nr_sectors = 180 * KB_IN_SECTORS,
	.sec_per_clus = 1,
	.dir_entries = 64,
	.media = 0xFC,
	.fat_length = 2,
},
{
	.nr_sectors = 320 * KB_IN_SECTORS,
	.sec_per_clus = 2,
	.dir_entries = 112,
	.media = 0xFF,
	.fat_length = 1,
},
{
	.nr_sectors = 360 * KB_IN_SECTORS,
	.sec_per_clus = 2,
	.dir_entries = 112,
	.media = 0xFD,
	.fat_length = 2,
},
};

static bool fat_bpb_is_zero(struct fat_boot_sector *b)
{
	if (get_unaligned_le16(&b->sector_size))
		return false;
	if (b->sec_per_clus)
		return false;
	if (b->reserved)
		return false;
	if (b->fats)
		return false;
	if (get_unaligned_le16(&b->dir_entries))
		return false;
	if (get_unaligned_le16(&b->sectors))
		return false;
	if (b->media)
		return false;
	if (b->fat_length)
		return false;
	if (b->secs_track)
		return false;
	if (b->heads)
		return false;
	return true;
}

static int fat_read_bpb(struct super_block *sb, struct fat_boot_sector *b,
	int silent, struct fat_bios_param_block *bpb)
{
	int error = -EINVAL;

	/* Read in BPB ... */
	memset(bpb, 0, sizeof(*bpb));
	bpb->fat_sector_size = get_unaligned_le16(&b->sector_size);
	bpb->fat_sec_per_clus = b->sec_per_clus;
	bpb->fat_reserved = le16_to_cpu(b->reserved);
	bpb->fat_fats = b->fats;
	bpb->fat_dir_entries = get_unaligned_le16(&b->dir_entries);
	bpb->fat_sectors = get_unaligned_le16(&b->sectors);
	bpb->fat_fat_length = le16_to_cpu(b->fat_length);
	bpb->fat_total_sect = le32_to_cpu(b->total_sect);

	bpb->fat16_state = b->fat16.state;
	bpb->fat16_vol_id = get_unaligned_le32(b->fat16.vol_id);

	bpb->fat32_length = le32_to_cpu(b->fat32.length);
	bpb->fat32_root_cluster = le32_to_cpu(b->fat32.root_cluster);
	bpb->fat32_info_sector = le16_to_cpu(b->fat32.info_sector);
	bpb->fat32_state = b->fat32.state;
	bpb->fat32_vol_id = get_unaligned_le32(b->fat32.vol_id);

	/* Validate this looks like a FAT filesystem BPB */
	if (!bpb->fat_reserved) {
		if (!silent)
			fat_msg(sb, KERN_ERR,
				"bogus number of reserved sectors");
		goto out;
	}
	if (!bpb->fat_fats) {
		if (!silent)
			fat_msg(sb, KERN_ERR, "bogus number of FAT structure");
		goto out;
	}

	/*
	 * Earlier we checked here that b->secs_track and b->head are nonzero,
	 * but it turns out valid FAT filesystems can have zero there.
	 */

	if (!fat_valid_media(b->media)) {
		if (!silent)
			fat_msg(sb, KERN_ERR, "invalid media value (0x%02x)",
				(unsigned)b->media);
		goto out;
	}

	if (!is_power_of_2(bpb->fat_sector_size)
	    || (bpb->fat_sector_size < 512)
	    || (bpb->fat_sector_size > 4096)) {
		if (!silent)
			fat_msg(sb, KERN_ERR, "bogus logical sector size %u",
			       (unsigned)bpb->fat_sector_size);
		goto out;
	}

	if (!is_power_of_2(bpb->fat_sec_per_clus)) {
		if (!silent)
			fat_msg(sb, KERN_ERR, "bogus sectors per cluster %u",
				(unsigned)bpb->fat_sec_per_clus);
		goto out;
	}

	if (bpb->fat_fat_length == 0 && bpb->fat32_length == 0) {
		if (!silent)
			fat_msg(sb, KERN_ERR, "bogus number of FAT sectors");
		goto out;
	}

	error = 0;

out:
	return error;
}

static int fat_read_static_bpb(struct super_block *sb,
	struct fat_boot_sector *b, int silent,
	struct fat_bios_param_block *bpb)
{
	static const char *notdos1x = "This doesn't look like a DOS 1.x volume";
	sector_t bd_sects = bdev_nr_sectors(sb->s_bdev);
	struct fat_floppy_defaults *fdefaults = NULL;
	int error = -EINVAL;
	unsigned i;

	/* 16-bit DOS 1.x reliably wrote bootstrap short-jmp or near-jmp code */
	if ((b->ignored[0] != 0xeb || b->ignored[2] != 0x90) &&
	    (b->ignored[0] != 0xe9)) {
		if (!silent)
			fat_msg(sb, KERN_ERR,
				"%s; no bootstrapping code", notdos1x);
		goto out;
	}

	/*
	 * If any value in this region is non-zero, it isn't archaic
	 * DOS.
	 */
	if (!fat_bpb_is_zero(b)) {
		if (!silent)
			fat_msg(sb, KERN_ERR,
				"%s; DOS 2.x BPB is non-zero", notdos1x);
		goto out;
	}

	for (i = 0; i < ARRAY_SIZE(floppy_defaults); i++) {
		if (floppy_defaults[i].nr_sectors == bd_sects) {
			fdefaults = &floppy_defaults[i];
			break;
		}
	}

	if (fdefaults == NULL) {
		if (!silent)
			fat_msg(sb, KERN_WARNING,
				"This looks like a DOS 1.x volume, but isn't a recognized floppy size (%llu sectors)",
				(u64)bd_sects);
		goto out;
	}

	if (!silent)
		fat_msg(sb, KERN_INFO,
			"This looks like a DOS 1.x volume; assuming default BPB values");

	memset(bpb, 0, sizeof(*bpb));
	bpb->fat_media = fdefaults->media;
	bpb->fat_sector_size = SECTOR_SIZE;
	bpb->fat_sec_per_clus = fdefaults->sec_per_clus;
	bpb->fat_reserved = 1;
	bpb->fat_fats = 2;
	bpb->fat_dir_entries = fdefaults->dir_entries;
	bpb->fat_sectors = fdefaults->nr_sectors;
	bpb->fat_fat_length = fdefaults->fat_length;

	error = 0;

out:
	return error;
}

int fat_linux_volume_state(const void *boot, size_t size, int *dirty)
{
	struct fat_boot_sector b;
	struct fat_bios_param_block bpb;
	struct super_block sb = {0};
	if (!boot || !dirty || size < sizeof(b))
		return -EINVAL;
	memcpy(&b, boot, sizeof(b));
	if (fat_read_bpb(&sb, &b, 1, &bpb))
		return -EINVAL;
	/* fat_fill_super: some OSes set FAT_STATE_DIRTY until unmount. */
	if (!bpb.fat_fat_length && bpb.fat32_length)
		*dirty = bpb.fat32_state & FAT_STATE_DIRTY;
	else
		*dirty = bpb.fat16_state & FAT_STATE_DIRTY;
	return 0;
}

int fat_linux_legacy_header(const void *boot, uint64_t size, void *normalized)
{
	struct fat_boot_sector b;
	struct fat_bios_param_block bpb;
	struct super_block sb = {0};
	u8 *p = normalized;
	if (size % SECTOR_SIZE)
		return 0;
	memcpy(&b, boot, sizeof(b));
	sb.size = size;
	sb.s_bdev = &sb;
	if (fat_read_static_bpb(&sb, &b, 1, &bpb))
		return 0;
	memmove(p, boot, SECTOR_SIZE);
#define PUT16(offset, value) do { \
	u16 le = cpu_to_le16((u16)(value)); \
	memcpy(p + (offset), &le, sizeof(le)); \
} while (0)
	PUT16(11, bpb.fat_sector_size);
	p[13] = bpb.fat_sec_per_clus;
	PUT16(14, bpb.fat_reserved);
	p[16] = bpb.fat_fats;
	PUT16(17, bpb.fat_dir_entries);
	PUT16(19, bpb.fat_sectors);
	p[21] = bpb.fat_media;
	PUT16(22, bpb.fat_fat_length);
	/* DOS 1.x has no extended BPB or usable extended volume fields. */
	memset(p + 24, 0, 40);
#undef PUT16
	return 1;
}

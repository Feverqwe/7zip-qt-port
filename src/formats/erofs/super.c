// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2017-2018 HUAWEI, Inc.
 *             https://www.huawei.com/
 * Copyright (C) 2021, Alibaba Cloud
 */
#include "formats/erofs/internal.h"

static int erofs_superblock_csum_verify(struct super_block *sb, void *sbdata)
{
	struct erofs_super_block *dsb = (void *)((char *)sbdata + EROFS_SUPER_OFFSET);
	u32 len = 1 << EROFS_SB(sb)->blkszbits, crc;

	if (len > EROFS_SUPER_OFFSET)
		len -= EROFS_SUPER_OFFSET;
	len -= offsetof(struct erofs_super_block, checksum) +
			sizeof(dsb->checksum);

	/* skip .magic(pre-verified) and .checksum(0) fields */
	crc = crc32c(0x5045B54A, (&dsb->checksum) + 1, len);
	if (crc == le32_to_cpu(dsb->checksum))
		return 0;
	erofs_err(sb, "invalid checksum 0x%08x, 0x%08x expected",
		  crc, le32_to_cpu(dsb->checksum));
	return -EBADMSG;
}

/* read variable-sized metadata, offset will be aligned by 4-byte */
void *erofs_read_metadata(struct super_block *sb, struct erofs_buf *buf,
			  erofs_off_t *offset, int *lengthp)
{
	u8 *buffer, *ptr;
	int len, i, cnt;

	*offset = round_up(*offset, 4);
	ptr = erofs_bread(buf, *offset, true);
	if (IS_ERR(ptr))
		return ptr;

	len = le16_to_cpu(*(__le16 *)ptr);
	if (!len)
		len = U16_MAX + 1;
	buffer = kmalloc(len, GFP_KERNEL);
	if (!buffer)
		return ERR_PTR(-ENOMEM);
	*offset += sizeof(__le16);
	*lengthp = len;

	for (i = 0; i < len; i += cnt) {
		cnt = min_t(int, sb->s_blocksize - erofs_blkoff(sb, *offset),
			    len - i);
		ptr = erofs_bread(buf, *offset, true);
		if (IS_ERR(ptr)) {
			kfree(buffer);
			return ptr;
		}
		memcpy(buffer + i, ptr, cnt);
		*offset += cnt;
	}
	return buffer;
}

int erofs_read_superblock(struct super_block *sb)
{
	struct erofs_sb_info *sbi = EROFS_SB(sb);
	struct erofs_buf buf = __EROFS_BUF_INITIALIZER;
	struct erofs_super_block *dsb;
	void *data;
	int ret;

	data = erofs_read_metabuf(&buf, sb, 0, false);
	if (IS_ERR(data)) {
		erofs_err(sb, "cannot read erofs superblock");
		return PTR_ERR(data);
	}

	dsb = (struct erofs_super_block *)((char *)data + EROFS_SUPER_OFFSET);
	ret = -EINVAL;
	if (le32_to_cpu(dsb->magic) != EROFS_SUPER_MAGIC_V1) {
		erofs_err(sb, "cannot find valid erofs superblock");
		goto out;
	}

	sbi->blkszbits = dsb->blkszbits;
	if (sbi->blkszbits < 9 || sbi->blkszbits > PAGE_SHIFT) {
		erofs_err(sb, "blkszbits %u isn't supported", sbi->blkszbits);
		goto out;
	}
	if (dsb->dirblkbits) {
		erofs_err(sb, "dirblkbits %u isn't supported", dsb->dirblkbits);
		goto out;
	}
	/* Stream buffers have no native block-device geometry to update later. */
	sb->s_blocksize_bits = sbi->blkszbits;
	sb->s_blocksize = 1U << sbi->blkszbits;

	sbi->feature_compat = le32_to_cpu(dsb->feature_compat);
	if (erofs_sb_has_sb_chksum(sbi)) {
		ret = erofs_superblock_csum_verify(sb, data);
		if (ret)
			goto out;
	}

	ret = -EINVAL;
	sbi->feature_incompat = le32_to_cpu(dsb->feature_incompat);
	if (sbi->feature_incompat & ~EROFS_ALL_FEATURE_INCOMPAT) {
		erofs_err(sb, "unidentified incompatible feature %x, please upgrade kernel",
			  sbi->feature_incompat & ~EROFS_ALL_FEATURE_INCOMPAT);
		goto out;
	}

	sbi->sb_size = 128 + dsb->sb_extslots * EROFS_SB_EXTSLOT_SIZE;
	if (sbi->sb_size > PAGE_SIZE - EROFS_SUPER_OFFSET) {
		erofs_err(sb, "invalid sb_extslots %u (more than a fs block)",
			  sbi->sb_size);
		goto out;
	}
	sbi->dif0.blocks = le32_to_cpu(dsb->blocks_lo);
	sbi->meta_blkaddr = le32_to_cpu(dsb->meta_blkaddr);
#ifdef CONFIG_EROFS_FS_XATTR
	sbi->xattr_blkaddr = le32_to_cpu(dsb->xattr_blkaddr);
	sbi->xattr_prefix_start = le32_to_cpu(dsb->xattr_prefix_start);
	sbi->xattr_prefix_count = dsb->xattr_prefix_count;
	sbi->xattr_filter_reserved = dsb->xattr_filter_reserved;
	if (erofs_sb_has_ishare_xattrs(sbi)) {
		if (dsb->ishare_xattr_prefix_id >= sbi->xattr_prefix_count) {
			erofs_err(sb, "invalid ishare xattr prefix id %u",
				  dsb->ishare_xattr_prefix_id);
			ret = -EFSCORRUPTED;
			goto out;
		}
		sbi->ishare_xattr_prefix_id = dsb->ishare_xattr_prefix_id;
	}
#endif
	sbi->islotbits = ilog2(sizeof(struct erofs_inode_compact));
	if (erofs_sb_has_48bit(sbi) && dsb->rootnid_8b) {
		sbi->root_nid = le64_to_cpu(dsb->rootnid_8b);
		sbi->dif0.blocks = sbi->dif0.blocks |
				((u64)le16_to_cpu(dsb->rb.blocks_hi) << 32);
	} else {
		sbi->root_nid = le16_to_cpu(dsb->rb.rootnid_2b);
	}
	sbi->packed_nid = le64_to_cpu(dsb->packed_nid);
	if (erofs_sb_has_metabox(sbi)) {
		ret = -EFSCORRUPTED;
		if (sbi->sb_size <= offsetof(struct erofs_super_block,
					     metabox_nid))
			goto out;
		sbi->metabox_nid = le64_to_cpu(dsb->metabox_nid);
		if (sbi->metabox_nid & BIT_ULL(EROFS_DIRENT_NID_METABOX_BIT))
			goto out;		/* self-loop detection */
	}
	sbi->inos = le64_to_cpu(dsb->inos);

	sbi->epoch = (s64)le64_to_cpu(dsb->epoch);
	sbi->fixed_nsec = le32_to_cpu(dsb->fixed_nsec);
	super_set_uuid(sb, (void *)dsb->uuid, sizeof(dsb->uuid));

	if (dsb->volume_name[0]) {
		sbi->volume_name = kstrndup((const char *)dsb->volume_name,
					    sizeof(dsb->volume_name), GFP_KERNEL);
		if (!sbi->volume_name) {
			ret = -ENOMEM;
			goto out;
		}
	}

	if (IS_ENABLED(CONFIG_EROFS_FS_ZIP)) {
		ret = z_erofs_parse_cfgs(sb, dsb);
		if (ret < 0)
			goto out;
	} else if (dsb->u1.available_compr_algs ||
		   erofs_sb_has_lz4_0padding(sbi)) {
		erofs_err(sb, "compression disabled, unable to mount compressed EROFS");
		ret = -EOPNOTSUPP;
		goto out;
	}

	ret = erofs_scan_devices(sb, dsb);

	if (erofs_sb_has_48bit(sbi))
		erofs_info(sb, "EXPERIMENTAL 48-bit layout support in use. Use at your own risk!");
	if (erofs_sb_has_metabox(sbi))
		erofs_info(sb, "EXPERIMENTAL metadata compression support in use. Use at your own risk!");
out:
	erofs_put_metabuf(&buf);
	return ret;
}

static int erofs_init_device(struct erofs_buf *buf, struct super_block *sb,
			     struct erofs_device_info *dif, erofs_off_t *pos)
{
	struct erofs_sb_info *sbi = EROFS_SB(sb);
	struct erofs_deviceslot *dis;
	bool _48bit;

	dis = erofs_read_metabuf(buf, sb, *pos, false);
	if (IS_ERR(dis))
		return PTR_ERR(dis);

	/* Archive callbacks replace path allocation and read-only file/bdev opens. */
	memcpy(dif->tag, dis->tag, sizeof(dif->tag));
	_48bit = erofs_sb_has_48bit(sbi);
	dif->blocks = le32_to_cpu(dis->blocks_lo) |
		(_48bit ? (u64)le16_to_cpu(dis->blocks_hi) << 32 : 0);
	dif->uniaddr = le32_to_cpu(dis->uniaddr_lo) |
		(_48bit ? (u64)le16_to_cpu(dis->uniaddr_hi) << 32 : 0);
	sbi->total_blocks += dif->blocks;
	if (!sbi->flatdev) {
		char tag[65];
		memcpy(tag, dif->tag, 64);
		tag[64] = 0;
		if (!sb->open_device)
			return -ENODEV;
		int err = sb->open_device(sb->context, dif->id, tag,
					 erofs_pos(sb, dif->blocks));
		if (err)
			return err;
	}
	*pos += EROFS_DEVT_SLOT_SIZE;
	return 0;
}

int erofs_scan_devices(struct super_block *sb, struct erofs_super_block *dsb)
{
	struct erofs_sb_info *sbi = EROFS_SB(sb);
	struct erofs_buf buf = __EROFS_BUF_INITIALIZER;
	erofs_off_t pos = le16_to_cpu(dsb->devt_slotoff) * EROFS_DEVT_SLOT_SIZE;
	unsigned int count = erofs_sb_has_device_table(sbi) ?
		le16_to_cpu(dsb->extra_devices) : 0;
	int err = 0;

	sbi->total_blocks = sbi->dif0.blocks;
	if (!count)
		return 0;
	if (pos > sb->image_size || (u64)count * EROFS_DEVT_SLOT_SIZE > sb->image_size - pos)
		return -EFSCORRUPTED;
	sbi->devices = calloc(count, sizeof(*sbi->devices));
	if (!sbi->devices)
		return -ENOMEM;
	sbi->extra_devices = count;
	sbi->device_id_mask = (u16)((1U << (ilog2(count) + 1)) - 1);
	for (unsigned int id = 0; id < count; ++id) {
		struct erofs_device_info *dif = &sbi->devices[id];
		dif->id = id + 1;
		err = erofs_init_device(&buf, sb, dif, &pos);
		if (err)
			break;
	}
	erofs_put_metabuf(&buf);
	return err;
}

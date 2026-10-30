// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2012-2013 Samsung Electronics Co., Ltd.
 */

/* Read-only userspace adaptation; see docs/upstream-port-audit/exfat-linux.md. */

#include "formats/exfat/exfat_fs.h"

static int exfat_extract_uni_name(struct exfat_dentry *ep,
		unsigned short *uniname)
{
	int i, len = 0;

	for (i = 0; i < EXFAT_FILE_NAME_LEN; i++) {
		*uniname = le16_to_cpu(ep->dentry.name.unicode_0_14[i]);
		if (*uniname == 0x0)
			return len;
		uniname++;
		len++;
	}

	*uniname = 0x0;
	return len;

}

static int exfat_get_uniname_from_ext_entry(struct super_block *sb,
		struct exfat_chain *p_dir, int entry, unsigned short *uniname)
{
	int i, err;
	struct exfat_entry_set_cache es;
	unsigned int uni_len = 0, len;

	err = exfat_get_dentry_set(&es, sb, p_dir, entry, ES_ALL_ENTRIES);
	if (err)
		return err;

	/*
	 * First entry  : file entry
	 * Second entry : stream-extension entry
	 * Third entry  : first file-name entry
	 * So, the index of first file-name dentry should start from 2.
	 */
	if (es.num_entries < 3) {
		exfat_put_dentry_set(&es, false);
		return -EIO;
	}

	for (i = ES_IDX_FIRST_FILENAME; i < es.num_entries; i++) {
		struct exfat_dentry *ep = exfat_get_dentry_cached(&es, i);

		/* end of name entry */
		if (exfat_get_entry_type(ep) != TYPE_EXTEND)
			break;

		len = exfat_extract_uni_name(ep, uniname);
		uni_len += len;
		if (len != EXFAT_FILE_NAME_LEN || uni_len >= MAX_NAME_LENGTH)
			break;
		uniname += EXFAT_FILE_NAME_LEN;
	}

	exfat_put_dentry_set(&es, false);
	return 0;
}

int exfat_readdir(struct inode *inode, loff_t *cpos, struct exfat_dir_entry *dir_entry)
{
	int i, dentries_per_clu, num_ext, err;
	unsigned int type, clu_offset, max_dentries;
	struct exfat_chain dir, clu;
	struct exfat_uni_name uni_name;
	struct exfat_dentry *ep;
	struct super_block *sb = inode->i_sb;
	struct exfat_sb_info *sbi = EXFAT_SB(sb);
	struct exfat_inode_info *ei = EXFAT_I(inode);
	unsigned int dentry = exfat_bytes_to_dentries(*cpos) & 0xFFFFFFFF;
	struct buffer_head *bh;

	/* check if the given file ID is opened */
	if (ei->type != TYPE_DIR)
		return -EPERM;

	exfat_chain_set(&dir, ei->start_clu,
		exfat_bytes_to_cluster(sbi, i_size_read(inode)), ei->flags);

	dentries_per_clu = sbi->dentries_per_clu;
	max_dentries = (unsigned int)min_t(u64, MAX_EXFAT_DENTRIES,
			exfat_cluster_to_dentries(sbi, sbi->num_clusters));
	max_dentries = min_t(u64, max_dentries, i_size_read(inode) >> DENTRY_SIZE_BITS);

	clu_offset = exfat_dentries_to_cluster(sbi, dentry);
	exfat_chain_dup(&clu, &dir);

	if (clu.flags == ALLOC_FAT_CHAIN) {
		/* hint_information */
		if (clu_offset > 0 && ei->hint_bmap.off != EXFAT_EOF_CLUSTER &&
		    ei->hint_bmap.off > 0 && clu_offset >= ei->hint_bmap.off) {
			clu_offset -= ei->hint_bmap.off;
			clu.dir = ei->hint_bmap.clu;
			clu.size -= ei->hint_bmap.off;
		}
	}

	if (exfat_chain_advance(sb, &clu, clu_offset))
		return -EIO;

	while (clu.dir != EXFAT_EOF_CLUSTER && dentry < max_dentries) {
		i = dentry & (dentries_per_clu - 1);

		for ( ; i < dentries_per_clu; i++, dentry++) {
			ep = exfat_get_dentry(sb, &clu, i, &bh);
			if (!ep)
				return -EIO;

			type = exfat_get_entry_type(ep);
			if (type == TYPE_UNUSED) {
				brelse(bh);
				goto out;
			}

			if (type != TYPE_FILE && type != TYPE_DIR) {
				brelse(bh);
				continue;
			}

			num_ext = ep->dentry.file.num_ext;
			dir_entry->attr = le16_to_cpu(ep->dentry.file.attr);

			*uni_name.name = 0x0;
			err = exfat_get_uniname_from_ext_entry(sb, &clu, i,
				uni_name.name);
			if (err) {
				brelse(bh);
				if (err == -ENOMEM)
					return err;
				sb->directory_error = true;
				continue;
			}
			exfat_utf16_to_nls(sb, &uni_name,
				(unsigned char *)dir_entry->namebuf.lfn,
				dir_entry->namebuf.lfnbuf_len);
			brelse(bh);

			ep = exfat_get_dentry(sb, &clu, i + 1, &bh);
			if (!ep)
				return -EIO;
			dir_entry->entry = i;
			dir_entry->dir = clu;
			brelse(bh);

			ei->hint_bmap.off = exfat_dentries_to_cluster(sbi, dentry);
			ei->hint_bmap.clu = clu.dir;

			*cpos = exfat_dentries_to_bytes(dentry + 1 + num_ext);
			return 0;
		}

		if (exfat_chain_advance(sb, &clu, 1))
			return -EIO;
	}

out:
	dir_entry->namebuf.lfn[0] = '\0';
	*cpos = exfat_dentries_to_bytes(dentry);
	return 0;
}

unsigned int exfat_get_entry_type(struct exfat_dentry *ep)
{
	if (ep->type == EXFAT_UNUSED)
		return TYPE_UNUSED;
	if (IS_EXFAT_DELETED(ep->type))
		return TYPE_DELETED;
	if (ep->type == EXFAT_INVAL)
		return TYPE_INVALID;
	if (IS_EXFAT_CRITICAL_PRI(ep->type)) {
		if (ep->type == EXFAT_BITMAP)
			return TYPE_BITMAP;
		if (ep->type == EXFAT_UPCASE)
			return TYPE_UPCASE;
		if (ep->type == EXFAT_VOLUME)
			return TYPE_VOLUME;
		if (ep->type == EXFAT_FILE) {
			if (le16_to_cpu(ep->dentry.file.attr) & EXFAT_ATTR_SUBDIR)
				return TYPE_DIR;
			return TYPE_FILE;
		}
		return TYPE_CRITICAL_PRI;
	}
	if (IS_EXFAT_BENIGN_PRI(ep->type)) {
		if (ep->type == EXFAT_GUID)
			return TYPE_GUID;
		if (ep->type == EXFAT_PADDING)
			return TYPE_PADDING;
		if (ep->type == EXFAT_ACLTAB)
			return TYPE_ACLTAB;
		return TYPE_BENIGN_PRI;
	}
	if (IS_EXFAT_CRITICAL_SEC(ep->type)) {
		if (ep->type == EXFAT_STREAM)
			return TYPE_STREAM;
		if (ep->type == EXFAT_NAME)
			return TYPE_EXTEND;
		if (ep->type == EXFAT_ACL)
			return TYPE_ACL;
		return TYPE_CRITICAL_SEC;
	}

	if (ep->type == EXFAT_VENDOR_EXT)
		return TYPE_VENDOR_EXT;
	if (ep->type == EXFAT_VENDOR_ALLOC)
		return TYPE_VENDOR_ALLOC;

	return TYPE_BENIGN_SEC;
}

int exfat_put_dentry_set(struct exfat_entry_set_cache *es, int sync)
{
	int i, err = 0;

	if (es->modified)
		err = -EROFS /* No write transport exists. */;

	for (i = 0; i < es->num_bh; i++)
		if (err)
			bforget(es->bh[i]);
		else
			brelse(es->bh[i]);

	if (IS_DYNAMIC_ES(es))
		kfree(es->bh);

	return err;
}

static int exfat_find_location(struct super_block *sb, struct exfat_chain *p_dir,
			       int entry, sector_t *sector, int *offset)
{
	int ret;
	unsigned int off, clu = 0;
	struct exfat_sb_info *sbi = EXFAT_SB(sb);

	off = exfat_dentries_to_bytes(entry);

	clu = p_dir->dir;
	ret = exfat_cluster_walk(sb, &clu, exfat_bytes_to_cluster(sbi, off),
			p_dir->flags);
	if (ret)
		return ret;

	if (clu == EXFAT_EOF_CLUSTER) {
		exfat_fs_error(sb,
			"unexpected early break in cluster chain (clu : %u, len : %d)",
			p_dir->dir,
			exfat_bytes_to_cluster(sbi, off));
		return -EIO;
	}

	if (!exfat_test_bitmap(sb, clu)) {
		exfat_err(sb, "failed to test cluster bit(%u)", clu);
		return -EIO;
	}

	/* byte offset in cluster */
	off = exfat_cluster_offset(sbi, off);

	/* byte offset in sector    */
	*offset = exfat_block_offset(sb, off);

	/* sector offset in cluster */
	*sector = exfat_bytes_to_block(sb, off);
	*sector += exfat_cluster_to_sector(sbi, clu);
	return 0;
}

struct exfat_dentry *exfat_get_dentry(struct super_block *sb,
		struct exfat_chain *p_dir, int entry, struct buffer_head **bh)
{
	struct exfat_sb_info *sbi = EXFAT_SB(sb);
	unsigned int sect_per_clus = sbi->sect_per_clus;
	unsigned int dentries_per_page = exfat_bytes_to_dentries(PAGE_SIZE);
	int off;
	sector_t sec;

	if (p_dir->dir == DIR_DELETED) {
		exfat_err(sb, "abnormal access to deleted dentry");
		return NULL;
	}

	if (exfat_find_location(sb, p_dir, entry, &sec, &off))
		return NULL;

	if (sect_per_clus > 1 &&
	    (entry & (dentries_per_page - 1)) == 0) {
		sector_t ra = sec;
		blkcnt_t cnt = 0;
		unsigned int ra_count = sect_per_clus;

		/* Not sector aligned with ra_count, resize ra_count to page size */
		if ((sec - sbi->data_start_sector) & (ra_count - 1))
			ra_count = PAGE_SIZE >> sb->s_blocksize_bits;

		exfat_blk_readahead(sb, sec, &ra, &cnt, sec + ra_count - 1);
	}

	*bh = sb_bread(sb, sec);
	if (!*bh)
		return NULL;

	return (struct exfat_dentry *)((*bh)->b_data + off);
}

enum exfat_validate_dentry_mode {
 ES_MODE_GET_FILE_ENTRY, ES_MODE_GET_STRM_ENTRY, ES_MODE_GET_NAME_ENTRY,
 ES_MODE_GET_CRITICAL_SEC_ENTRY, ES_MODE_GET_BENIGN_SEC_ENTRY
};

static bool exfat_validate_entry(unsigned int type,
		enum exfat_validate_dentry_mode *mode)
{
	if (type == TYPE_UNUSED || type == TYPE_DELETED)
		return false;

	switch (*mode) {
	case ES_MODE_GET_FILE_ENTRY:
		if (type != TYPE_STREAM)
			return false;
		*mode = ES_MODE_GET_STRM_ENTRY;
		break;
	case ES_MODE_GET_STRM_ENTRY:
		if (type != TYPE_EXTEND)
			return false;
		*mode = ES_MODE_GET_NAME_ENTRY;
		break;
	case ES_MODE_GET_NAME_ENTRY:
		if (type & TYPE_BENIGN_SEC)
			*mode = ES_MODE_GET_BENIGN_SEC_ENTRY;
		else if (type != TYPE_EXTEND)
			return false;
		break;
	case ES_MODE_GET_BENIGN_SEC_ENTRY:
		/* Assume unreconized benign secondary entry */
		if (!(type & TYPE_BENIGN_SEC))
			return false;
		break;
	default:
		return false;
	}

	return true;
}

struct exfat_dentry *exfat_get_dentry_cached(
	struct exfat_entry_set_cache *es, int num)
{
	int off = es->start_off + num * DENTRY_SIZE;
	struct buffer_head *bh = es->bh[exfat_bytes_to_block(es->sb, off)];
	char *p = bh->b_data + exfat_block_offset(es->sb, off);

	return (struct exfat_dentry *)p;
}

static int __exfat_get_dentry_set(struct exfat_entry_set_cache *es,
		struct super_block *sb, struct exfat_chain *p_dir, int entry,
		unsigned int num_entries)
{
	int ret, i, num_bh;
	int off;
	sector_t sec;
	struct exfat_sb_info *sbi = EXFAT_SB(sb);
	struct buffer_head *bh;

	if (p_dir->dir == DIR_DELETED) {
		exfat_err(sb, "access to deleted dentry");
		return -EIO;
	}

	ret = exfat_find_location(sb, p_dir, entry, &sec, &off);
	if (ret)
		return ret;

	memset(es, 0, sizeof(*es));
	es->sb = sb;
	es->modified = false;
	es->start_off = off;
	es->bh = es->__bh;

	bh = sb_bread(sb, sec);
	if (!bh)
		return -EIO;
	es->bh[es->num_bh++] = bh;

	if (num_entries == ES_ALL_ENTRIES) {
		struct exfat_dentry *ep;

		ep = exfat_get_dentry_cached(es, ES_IDX_FILE);
		if (ep->type != EXFAT_FILE) {
			brelse(bh);
			return -EIO;
		}

		num_entries = ep->dentry.file.num_ext + 1;
	}

	es->num_entries = num_entries;

	num_bh = exfat_bytes_to_block_round_up(sb, off + num_entries * DENTRY_SIZE);
	if (num_bh > ARRAY_SIZE(es->__bh)) {
		es->bh = kmalloc_objs(*es->bh, num_bh, GFP_NOFS);
		if (!es->bh) {
			brelse(bh);
			return -ENOMEM;
		}
		es->bh[0] = bh;
	}

	for (i = 1; i < num_bh; i++) {
		/* get the next sector */
		if (exfat_is_last_sector_in_cluster(sbi, sec)) {
			unsigned int clu = exfat_sector_to_cluster(sbi, sec);

			if (exfat_cluster_walk(sb, &clu, 1, p_dir->flags))
				goto put_es;
			sec = exfat_cluster_to_sector(sbi, clu);
		} else {
			sec++;
		}

		bh = sb_bread(sb, sec);
		if (!bh)
			goto put_es;
		es->bh[es->num_bh++] = bh;
	}

	return 0;

put_es:
	exfat_put_dentry_set(es, false);
	return -EIO;
}

int exfat_get_dentry_set(struct exfat_entry_set_cache *es,
		struct super_block *sb, struct exfat_chain *p_dir,
		int entry, unsigned int num_entries)
{
	int ret, i;
	struct exfat_dentry *ep;
	enum exfat_validate_dentry_mode mode = ES_MODE_GET_FILE_ENTRY;

	ret = __exfat_get_dentry_set(es, sb, p_dir, entry, num_entries);
	if (ret < 0)
		return ret;

	if (es->num_entries < 3)
		goto put_es;

	/* validate cached dentries */
	for (i = ES_IDX_STREAM; i < es->num_entries; i++) {
		ep = exfat_get_dentry_cached(es, i);
		if (!exfat_validate_entry(exfat_get_entry_type(ep), &mode))
			goto put_es;
	}
	/* A damaged set must not silently publish names or file extents. */
	u16 checksum = 0;
	for (i = 0; i < es->num_entries; i++)
		checksum = exfat_calc_chksum16(exfat_get_dentry_cached(es, i),
			DENTRY_SIZE, checksum, i ? CS_DEFAULT : CS_DIR_ENTRY);
	ep = exfat_get_dentry_cached(es, ES_IDX_FILE);
	if (checksum != le16_to_cpu(ep->dentry.file.checksum))
		goto put_es;
	return 0;

put_es:
	exfat_put_dentry_set(es, false);
	return -EIO;
}

/* Read-only adaptation of the volume-label reader. Free-slot hints belong to
 * the omitted write path; volume access is serialized by the archive owner. */
static int exfat_get_volume_label_dentry(struct super_block *sb,
		struct exfat_entry_set_cache *es)
{
	int i;
	unsigned int type, walked = 0;
	struct exfat_sb_info *sbi = EXFAT_SB(sb);
	struct exfat_chain clu;
	struct exfat_dentry *ep;
	struct buffer_head *bh;

	exfat_chain_set(&clu, sbi->root_dir, 0, ALLOC_FAT_CHAIN);

	while (clu.dir != EXFAT_EOF_CLUSTER) {
		if (walked++ >= sb->root_clusters)
			break;
		for (i = 0; i < sbi->dentries_per_clu; i++) {
			ep = exfat_get_dentry(sb, &clu, i, &bh);
			if (!ep)
				return -EIO;

			type = exfat_get_entry_type(ep);
			if (type == TYPE_UNUSED) {
				brelse(bh);
				goto not_found;
			}

			if (type != TYPE_VOLUME) {
				brelse(bh);
				continue;
			}

			memset(es, 0, sizeof(*es));
			es->sb = sb;
			es->bh = es->__bh;
			es->bh[0] = bh;
			es->num_bh = 1;
			es->start_off = exfat_dentries_to_bytes(i) % sb->s_blocksize;

			return 0;
		}

		if (walked == sb->root_clusters)
			break;
		if (exfat_get_next_cluster(sb, &(clu.dir)))
			return -EIO;
	}

not_found:
	return -ENOENT;
}

int exfat_read_volume_label(struct super_block *sb, struct exfat_uni_name *label_out)
{
	int ret, i;
	struct exfat_entry_set_cache es;
	struct exfat_dentry *ep;

	memset(label_out, 0, sizeof(*label_out));
	ret = exfat_get_volume_label_dentry(sb, &es);
	if (ret < 0) {
		/* ENOENT is an empty volume label, as in the original ioctl reader. */
		if (ret == -ENOENT)
			ret = 0;
		goto unlock;
	}

	ep = exfat_get_dentry_cached(&es, 0);
	label_out->name_len = ep->dentry.volume_label.char_count;
	if (label_out->name_len > EXFAT_VOLUME_LABEL_LEN) {
		ret = -EIO;
		exfat_put_dentry_set(&es, false);
		goto unlock;
	}

	for (i = 0; i < label_out->name_len; i++)
		label_out->name[i] = le16_to_cpu(ep->dentry.volume_label.volume_label[i]);

	exfat_put_dentry_set(&es, false);
unlock:
	return ret;
}

/*
 *  linux/fs/hfs/catalog.c
 *
 * Copyright (C) 1995-1997  Paul H. Hargrove
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 * This file may be distributed under the terms of the GNU General Public License.
 *
 * This file contains the functions related to the catalog B-tree.
 *
 * Cache code shamelessly stolen from
 *     linux/fs/inode.c Copyright (C) 1991, 1992  Linus Torvalds
 *     re-shamelessly stolen Copyright (C) 1997 Linus Torvalds
 */

#include "btree.h"

/* CNID-only read branch of hfs_cat_build_key; inverse NLS/write names omitted. */
static void hfs_cat_build_key(btree_key *key, u32 parent)
{
    key->cat.reserved = 0;
    key->cat.ParID = cpu_to_be32(parent);
    memset(&key->cat.CName, 0, sizeof(struct hfs_name));
    key->key_len = 6;
}

int hfs_cat_keycmp(const btree_key *key1, const btree_key *key2)
{
	__be32 k1p, k2p;

	k1p = key1->cat.ParID;
	k2p = key2->cat.ParID;

	if (k1p != k2p)
		return be32_to_cpu(k1p) < be32_to_cpu(k2p) ? -1 : 1;

	return hfs_strcmp(key1->cat.CName.name, key1->cat.CName.len,
			  key2->cat.CName.name, key2->cat.CName.len);
}

static int hfs_cat_validate_found_cnid(struct hfs_find_data *fd, u32 cnid)
{
	hfs_cat_rec rec;
	u32 found_cnid;
	int rec_len;

	rec_len = fd->entrylength;
	if (rec_len <= 0 || (size_t)rec_len > sizeof(rec))
		return -EIO;

	memset(&rec, 0, sizeof(rec));
	hfs_bnode_read(fd->bnode, &rec, fd->entryoffset, rec_len);

	switch (rec.type) {
	case HFS_CDR_FIL:
		if ((size_t)rec_len != sizeof(struct hfs_cat_file))
			return -EIO;
		found_cnid = be32_to_cpu(rec.file.FlNum);
		break;
	case HFS_CDR_DIR:
		if ((size_t)rec_len != sizeof(struct hfs_cat_dir))
			return -EIO;
		found_cnid = be32_to_cpu(rec.dir.DirID);
		break;
	default:
		return -EIO;
	}

	if (found_cnid != cnid)
		return -EIO;

	return 0;
}

int hfs_cat_find_brec(struct super_block *sb, u32 cnid,
		      struct hfs_find_data *fd)
{
	hfs_cat_rec rec;
	int res, len, type;
	(void)sb;

	hfs_cat_build_key(fd->search_key, cnid);
	memset(&rec, 0, sizeof(rec));
	res = hfs_brec_read(fd, &rec, sizeof(rec));
	if (res)
		return res;
	if (fd->entrylength < sizeof(struct hfs_cat_thread))
		return -EIO;

	type = rec.type;
	if (type != HFS_CDR_THD && type != HFS_CDR_FTH) {
		pr_err("found bad thread record in catalog\n");
		return -EIO;
	}

	fd->search_key->cat.ParID = rec.thread.ParID;
	len = fd->search_key->cat.CName.len = rec.thread.CName.len;
	if (len > HFS_NAMELEN) {
		pr_err("bad catalog namelength\n");
		return -EIO;
	}
	memcpy(fd->search_key->cat.CName.name, rec.thread.CName.name, len);

	res = hfs_brec_find(fd);
	if (res)
		return res;

	return hfs_cat_validate_found_cnid(fd, cnid);
}

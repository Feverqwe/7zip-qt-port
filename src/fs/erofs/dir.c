// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2017-2018 HUAWEI, Inc.
 *             https://www.huawei.com/
 * Copyright (C) 2022, Alibaba Cloud
 */
#include "internal.h"

int erofs_fill_dentries(struct inode *dir, struct dir_context *ctx,
			       void *dentry_blk, struct erofs_dirent *de,
			       unsigned int nameoff0, unsigned int maxsize)
{
	const struct erofs_dirent *end = (void *)((char *)dentry_blk + nameoff0);

	while (de < end) {
		unsigned char d_type = fs_ftype_to_dtype(de->file_type);
		unsigned int nameoff = le16_to_cpu(de->nameoff);
		const char *de_name = (char *)dentry_blk + nameoff;
		unsigned int de_namelen;

		/* non-trailing dirent in the directory block? */
		if (de + 1 < end)
			de_namelen = le16_to_cpu(de[1].nameoff) - nameoff;
		else if (maxsize <= nameoff)
			goto err_bogus;
		else
			de_namelen = strnlen(de_name, maxsize - nameoff);

		/* a corrupted entry is found (including negative namelen) */
		if (!in_range32(de_namelen, 1, EROFS_NAME_LEN) ||
		    nameoff + de_namelen > maxsize)
			goto err_bogus;

		if (!dir_emit(ctx, de_name, de_namelen,
			      erofs_nid_to_ino64(EROFS_SB(dir->i_sb),
						 le64_to_cpu(de->nid)), d_type))
			return 1;
		++de;
		ctx->pos += sizeof(struct erofs_dirent);
	}
	return 0;
err_bogus:
	erofs_err(dir->i_sb, "bogus dirent @ nid %llu", EROFS_I(dir)->nid);
	DBG_BUGON(1);
	return -EFSCORRUPTED;
}

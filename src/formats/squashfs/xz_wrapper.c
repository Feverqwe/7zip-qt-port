// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Squashfs - a compressed read only filesystem for Linux
 *
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
 * Phillip Lougher <phillip@squashfs.org.uk>
 *
 * xz_wrapper.c
 */


#include "formats/squashfs/squashfscompat.h"
#include "formats/squashfs/squashfs_fs.h"
#include "formats/squashfs/squashfs_fs_sb.h"
#include "formats/squashfs/decompressor.h"

struct disk_comp_opts {
	__le32 dictionary_size;
	__le32 flags;
};

struct comp_opts {
	int dict_size;
};

static void *squashfs_xz_comp_opts(struct squashfs_sb_info *msblk,
	void *buff, int len)
{
	struct disk_comp_opts *comp_opts = buff;
	struct comp_opts *opts;
	int err = 0, n;

	opts = kmalloc_obj(*opts);
	if (opts == NULL) {
		err = -ENOMEM;
		goto out2;
	}

	if (comp_opts) {
		/* check compressor options are the expected length */
		if (len < sizeof(*comp_opts)) {
			err = -EIO;
			goto out;
		}

		opts->dict_size = le32_to_cpu(comp_opts->dictionary_size);

		/* the dictionary size should be positive and 2^n or 2^n+2^(n+1) */
		n = ffs(opts->dict_size) - 1;
		if (opts->dict_size <= 0 || (opts->dict_size != (1 << n) &&
			opts->dict_size != (1 << n) + (1 << (n + 1)))) {
			err = -EIO;
			goto out;
		}
	} else
		/* use defaults */
		opts->dict_size = max_t(int, msblk->block_size,
							SQUASHFS_METADATA_SIZE);

	return opts;

out:
	kfree(opts);
out2:
	return ERR_PTR(err);
}


/* The archive reader retains 7-Zip's bounded XZ decoder. */
const struct squashfs_decompressor squashfs_xz_comp_ops = {
 .init = sevenzip_squashfs_external_init, .free = sevenzip_squashfs_external_free,
 .comp_opts = squashfs_xz_comp_opts, .id = XZ_COMPRESSION,
 .name = "xz", .supported = 1
};

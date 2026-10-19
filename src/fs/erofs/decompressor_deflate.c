// SPDX-License-Identifier: GPL-2.0-or-later
#include "internal.h"
#define MAX_WBITS 15

int z_erofs_load_deflate_config(struct super_block *sb,
			struct erofs_super_block *dsb, void *data, int size)
{
	struct z_erofs_deflate_cfgs *dfl = data;

	if (!dfl || size < sizeof(struct z_erofs_deflate_cfgs)) {
		erofs_err(sb, "invalid deflate cfgs, size=%u", size);
		return -EINVAL;
	}

	if (dfl->windowbits > MAX_WBITS) {
		erofs_err(sb, "unsupported windowbits %u", dfl->windowbits);
		return -EOPNOTSUPP;
	}
	return 0;
}

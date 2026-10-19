// SPDX-License-Identifier: GPL-2.0-or-later
#include "internal.h"

int z_erofs_load_zstd_config(struct super_block *sb,
			struct erofs_super_block *dsb, void *data, int size)
{
	struct z_erofs_zstd_cfgs *zstd = data;

	if (!zstd || size < sizeof(struct z_erofs_zstd_cfgs) || zstd->format) {
		erofs_err(sb, "unsupported zstd format, size=%u", size);
		return -EINVAL;
	}

	if (zstd->windowlog > ilog2(Z_EROFS_ZSTD_MAX_DICT_SIZE) - 10) {
		erofs_err(sb, "unsupported zstd window log %u", zstd->windowlog);
		return -EINVAL;
	}
	EROFS_SB(sb)->zstd_dict_size = 1U << (zstd->windowlog + 10);
	return 0;
}

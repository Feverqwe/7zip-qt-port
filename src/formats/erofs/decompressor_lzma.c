// SPDX-License-Identifier: GPL-2.0-or-later
#include "formats/erofs/internal.h"

int z_erofs_load_lzma_config(struct super_block *sb,
			struct erofs_super_block *dsb, void *data, int size)
{
	struct z_erofs_lzma_cfgs *lzma = data;
	unsigned int dict_size;

	if (!lzma || size < sizeof(struct z_erofs_lzma_cfgs)) {
		erofs_err(sb, "invalid lzma cfgs, size=%u", size);
		return -EINVAL;
	}
	if (lzma->format) {
		erofs_err(sb, "unidentified lzma format %x, please check kernel version",
			  le16_to_cpu(lzma->format));
		return -EINVAL;
	}
	dict_size = le32_to_cpu(lzma->dict_size);
	if (dict_size > Z_EROFS_LZMA_MAX_DICT_SIZE || dict_size < 4096) {
		erofs_err(sb, "unsupported lzma dictionary size %u",
			  dict_size);
		return -EINVAL;
	}

	EROFS_SB(sb)->lzma_dict_size = dict_size;
	return 0;
}

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 */

#include "ntfslinkcompat.h"

int ntfs_utf16_to_nls(struct ntfs_sb_info *sbi, const __le16 *name, u32 len,
		      u8 *buf, int buf_len)
{
	int ret, warn;
	u8 *op;
	struct nls_table *nls = sbi->options->nls;

	static_assert(sizeof(fat_wchar_t) == sizeof(__le16));

	if (buf_len <= 0)
		return -EINVAL;

	buf_len -= 1;

	if (!nls) {
		/* UTF-16 -> UTF-8 */
		ret = utf16s_to_utf8s((fat_wchar_t *)name, len, UTF16_LITTLE_ENDIAN,
				      buf, buf_len);
		buf[ret] = '\0';
		return ret;
	}

	op = buf;
	warn = 0;

	while (len--) {
		u16 ec;
		int charlen;
		char dump[5];

		if (buf_len < NLS_MAX_CHARSET_SIZE) {
			ntfs_warn(sbi->sb,
				  "filename was truncated while converting.");
			break;
		}

		ec = le16_to_cpu(*name++);
		charlen = nls->uni2char(ec, op, buf_len);

		if (charlen > 0) {
			op += charlen;
			buf_len -= charlen;
			continue;
		}

		*op++ = '_';
		buf_len -= 1;
		if (warn)
			continue;

		warn = 1;
		hex_byte_pack(&dump[0], ec >> 8);
		hex_byte_pack(&dump[2], ec);
		dump[4] = 0;

		ntfs_err(sbi->sb, "failed to convert \"%s\" to %s", dump,
			 nls->charset);
	}

	*op = '\0';
	return op - buf;
}

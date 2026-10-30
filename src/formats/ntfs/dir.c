// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 */

#ifdef SEVENZIP_NTFS_DIRECTORY
#include "formats/ntfs/ntfsdirectorycompat.h"

static inline bool de_fname_fits(const struct NTFS_DE *e, u32 e_size,
				 const struct ATTR_FILE_NAME *fname)
{
	return sizeof(struct NTFS_DE) + fname_full_size(fname) <= e_size;
}

static inline const struct ATTR_FILE_NAME *
de_countable_fname(const struct NTFS_DE *e, u32 e_size)
{
	const struct ATTR_FILE_NAME *fname;

	fname = de_get_fname(e);
	if (!fname || fname->type == FILE_NAME_DOS ||
	    !de_fname_fits(e, e_size, fname))
		return NULL;

	return fname;
}

static int ntfs_dir_count(struct inode *dir, bool *is_empty, size_t *dirs,
			  size_t *files)
{
	int err = 0;
	struct ntfs_inode *ni = ntfs_i(dir);
	struct NTFS_DE *e = NULL;
	struct INDEX_ROOT *root;
	struct INDEX_HDR *hdr;
	const struct ATTR_FILE_NAME *fname;
	u32 e_size, off, end;
	size_t drs = 0, fles = 0, bit = 0;
	struct indx_node *node = NULL;
	size_t max_indx = i_size_read(&ni->vfs_inode) >> ni->dir.index_bits;

	if (is_empty)
		*is_empty = true;

	root = indx_get_root(&ni->dir, ni, NULL, NULL);
	if (!root)
		return -EINVAL;

	hdr = &root->ihdr;

	for (;;) {
		end = le32_to_cpu(hdr->used);
		off = le32_to_cpu(hdr->de_off);

		for (; off + sizeof(struct NTFS_DE) <= end; off += e_size) {
			e = Add2Ptr(hdr, off);
			e_size = le16_to_cpu(e->size);
			if (e_size < sizeof(struct NTFS_DE) ||
			    off + e_size > end) {
				/* Looks like corruption. */
				break;
			}

			ntfs_directory_check_child(ni, e, node);
			if (de_is_last(e))
				break;

			fname = de_countable_fname(e, e_size);
			if (!fname) {
				const struct ATTR_FILE_NAME *raw = de_get_fname(e);
				if (!raw || raw->type != FILE_NAME_DOS)
					ni->directory_unverified = true;
				continue;
			}

			err = ntfs_directory_emit(ni, e, fname);
			if (err)
				goto out;

			if (is_empty) {
				*is_empty = false;
				if (!dirs && !files)
					goto out;
			}

			if (fname->dup.fa & FILE_ATTRIBUTE_DIRECTORY)
				drs += 1;
			else
				fles += 1;
		}

		if (bit >= max_indx)
			goto out;

		err = indx_used_bit(&ni->dir, ni, &bit);
		if (err)
			goto out;

		if (bit == MINUS_ONE_T)
			goto out;

		if (bit >= max_indx) {
			err = -EINVAL;
			goto out;
		}

		err = indx_read(&ni->dir, ni, bit << ni->dir.idx2vbn_bits,
				&node);
		if (err)
			goto out;

		hdr = &node->index->ihdr;
		bit += 1;
	}

out:
	put_indx_node(node);
	if (dirs)
		*dirs = drs;
	if (files)
		*files = fles;

	return err;
}
int ntfs_directory_count(struct ntfs_inode *ni)
{
	int err = ntfs_dir_count(&ni->vfs_inode, NULL, NULL, NULL);
	return err ? err : ni->directory_unverified ? -EINVAL : 0;
}

#else
#include "formats/ntfs/ntfslinkcompat.h"

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

#endif

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 */

#include "formats/ntfs/ntfslinkcompat.h"

/* ntfs_read_mft ATTR_DATA projection. Enumeration and stream ownership stay
 * with the archive owner; the original little-endian predicates/branches remain. */
uint32_t sevenzip_ntfs_data_attributes(uint32_t attributes, uint16_t flags)
{
	struct ATTRIB value = {0};
	const struct ATTRIB *attr = &value;
	__le32 std_fa = cpu_to_le32(attributes);
	value.flags = cpu_to_le16(flags);

	if (is_attr_sparsed(attr))
		std_fa |= FILE_ATTRIBUTE_SPARSE_FILE;
	else
		std_fa &= ~FILE_ATTRIBUTE_SPARSE_FILE;

	if (is_attr_compressed(attr))
		std_fa |= FILE_ATTRIBUTE_COMPRESSED;
	else
		std_fa &= ~FILE_ATTRIBUTE_COMPRESSED;

	if (is_attr_encrypted(attr))
		std_fa |= FILE_ATTRIBUTE_ENCRYPTED;
	else
		std_fa &= ~FILE_ATTRIBUTE_ENCRYPTED;

	return le32_to_cpu(std_fa);
}

static int ntfs_translate_junction(const struct super_block *sb,
				   const struct dentry *link_de, char *target,
				   int target_len, int target_max)
{
	int tl_len, err = target_len;
	char *link_path_buffer = NULL, *link_path;
	char *translated = NULL;
	char *target_start;
	int copy_len;

	link_path_buffer = kmalloc(PATH_MAX, GFP_NOFS);
	if (!link_path_buffer) {
		err = -ENOMEM;
		goto out;
	}
	/* Get link path, relative to mount point */
	link_path = dentry_path_raw(link_de, link_path_buffer, PATH_MAX);
	if (IS_ERR(link_path)) {
		ntfs_err(sb, "Error getting link path");
		err = -EINVAL;
		goto out;
	}

	translated = kmalloc(PATH_MAX, GFP_NOFS);
	if (!translated) {
		err = -ENOMEM;
		goto out;
	}

	/* Make translated path a relative path to mount point */
	strcpy(translated, "./");
	++link_path; /* Skip leading / */
	for (tl_len = sizeof("./") - 1; *link_path; ++link_path) {
		if (*link_path == '/') {
			if (PATH_MAX - tl_len < sizeof("../")) {
				ntfs_err(sb,
					 "Link path %s has too many components",
					 link_path);
				err = -EINVAL;
				goto out;
			}
			strcpy(translated + tl_len, "../");
			tl_len += sizeof("../") - 1;
		}
	}

	/* Skip drive letter */
	target_start = target;
	while (*target_start && *target_start != ':')
		++target_start;

	if (!*target_start) {
		ntfs_err(sb, "Link target (%s) missing drive separator",
			 target);
		err = -EINVAL;
		goto out;
	}

	/* Skip drive separator and leading /, if exists */
	target_start += 1 + (target_start[1] == '/');
	copy_len = target_len - (target_start - target);

	if (PATH_MAX - tl_len <= copy_len) {
		ntfs_err(sb, "Link target %s too large for buffer (%d <= %d)",
			 target_start, PATH_MAX - tl_len, copy_len);
		err = -EINVAL;
		goto out;
	}

	/* translated path has a trailing / and target_start does not */
	strcpy(translated + tl_len, target_start);
	tl_len += copy_len;
	if (target_max <= tl_len) {
		ntfs_err(sb, "Target path %s too large for buffer (%d <= %d)",
			 translated, target_max, tl_len);
		err = -EINVAL;
		goto out;
	}
	strcpy(target, translated);
	err = tl_len;

out:
	kfree(link_path_buffer);
	kfree(translated);
	return err;
}

static noinline int ntfs_readlink_hlp(const struct dentry *link_de,
				      struct inode *inode, char *buffer,
				      int buflen)
{
	int i, err = -EINVAL;
	struct ntfs_inode *ni = ntfs_i(inode);
	struct super_block *sb = inode->i_sb;
	struct ntfs_sb_info *sbi = sb->s_fs_info;
	u64 size;
	u16 ulen = 0;
	void *to_free = NULL;
	struct REPARSE_DATA_BUFFER *rp;
	const __le16 *uname;
	struct ATTRIB *attr;

	/* Reparse data present. Try to parse it. */
	static_assert(offsetof(struct REPARSE_DATA_BUFFER, ReparseTag) == 0);
	static_assert(sizeof(u32) == sizeof(rp->ReparseTag));

	*buffer = 0;

	attr = ni_find_attr(ni, NULL, NULL, ATTR_REPARSE, NULL, 0, NULL, NULL);
	if (!attr)
		goto out;

	if (!attr->non_res) {
		rp = resident_data_ex(attr, sizeof(struct REPARSE_DATA_BUFFER));
		if (!rp)
			goto out;
		size = le32_to_cpu(attr->res.data_size);
	} else {
		size = le64_to_cpu(attr->nres.data_size);
		rp = NULL;
	}

	if (size > sbi->reparse.max_size || size <= sizeof(u32))
		goto out;

	if (!rp) {
		rp = kmalloc(size, GFP_NOFS);
		if (!rp) {
			err = -ENOMEM;
			goto out;
		}
		to_free = rp;
		/* Read into temporal buffer. */
		err = ntfs_read_run_nb(sbi, &ni->file.run, 0, rp, size, NULL);
		if (err)
			goto out;
	}

	/* Microsoft Tag. */
	switch (rp->ReparseTag) {
	case IO_REPARSE_TAG_MOUNT_POINT:
		/* Mount points and junctions. */
		/* Can we use 'Rp->MountPointReparseBuffer.PrintNameLength'? */
		if (size <= offsetof(struct REPARSE_DATA_BUFFER,
				     MountPointReparseBuffer.PathBuffer))
			goto out;
		if (le16_to_cpu(rp->MountPointReparseBuffer.PrintNameOffset) >
		    size - offsetof(struct REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer))
			goto out;
		uname = Add2Ptr(rp,
				offsetof(struct REPARSE_DATA_BUFFER,
					 MountPointReparseBuffer.PathBuffer) +
					le16_to_cpu(rp->MountPointReparseBuffer
							    .PrintNameOffset));
		ulen = le16_to_cpu(rp->MountPointReparseBuffer.PrintNameLength);
		break;

	case IO_REPARSE_TAG_SYMLINK:
		/* FolderSymbolicLink */
		/* Can we use 'Rp->SymbolicLinkReparseBuffer.PrintNameLength'? */
		if (size <= offsetof(struct REPARSE_DATA_BUFFER,
				     SymbolicLinkReparseBuffer.PathBuffer))
			goto out;
		if (le16_to_cpu(rp->SymbolicLinkReparseBuffer.PrintNameOffset) >
		    size - offsetof(struct REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer))
			goto out;
		uname = Add2Ptr(
			rp, offsetof(struct REPARSE_DATA_BUFFER,
				     SymbolicLinkReparseBuffer.PathBuffer) +
				    le16_to_cpu(rp->SymbolicLinkReparseBuffer
							.PrintNameOffset));
		ulen = le16_to_cpu(
			rp->SymbolicLinkReparseBuffer.PrintNameLength);
		break;

	case IO_REPARSE_TAG_CLOUD:
	case IO_REPARSE_TAG_CLOUD_1:
	case IO_REPARSE_TAG_CLOUD_2:
	case IO_REPARSE_TAG_CLOUD_3:
	case IO_REPARSE_TAG_CLOUD_4:
	case IO_REPARSE_TAG_CLOUD_5:
	case IO_REPARSE_TAG_CLOUD_6:
	case IO_REPARSE_TAG_CLOUD_7:
	case IO_REPARSE_TAG_CLOUD_8:
	case IO_REPARSE_TAG_CLOUD_9:
	case IO_REPARSE_TAG_CLOUD_A:
	case IO_REPARSE_TAG_CLOUD_B:
	case IO_REPARSE_TAG_CLOUD_C:
	case IO_REPARSE_TAG_CLOUD_D:
	case IO_REPARSE_TAG_CLOUD_E:
	case IO_REPARSE_TAG_CLOUD_F:
		err = sizeof("OneDrive") - 1;
		if (err > buflen)
			err = buflen;
		memcpy(buffer, "OneDrive", err);
		goto out;

	default:
		if (IsReparseTagMicrosoft(rp->ReparseTag)) {
			/* Unknown Microsoft Tag. */
			goto out;
		}
		if (!IsReparseTagNameSurrogate(rp->ReparseTag) ||
		    size <= sizeof(struct REPARSE_POINT)) {
			goto out;
		}

		/* Users tag. */
		uname = Add2Ptr(rp, sizeof(struct REPARSE_POINT));
		ulen = le16_to_cpu(rp->ReparseDataLength) -
		       sizeof(struct REPARSE_POINT);
	}

	/* Convert nlen from bytes to UNICODE chars. */
	ulen >>= 1;

	/* Check that name is available. */
	if (!ulen || (size_t)ulen * sizeof(*uname) >
	    size - ((const u8 *)uname - (const u8 *)rp))
		goto out;

	/* If name is already zero terminated then truncate it now. */
	if (!get_unaligned_le16((const u8 *)uname + (ulen - 1) * sizeof(*uname)))
		ulen -= 1;

	err = ntfs_utf16_to_nls(sbi, uname, ulen, (u8 *)buffer, buflen);

	if (err < 0)
		goto out;

	/* Translate Windows '\' into Linux '/'. */
	for (i = 0; i < err; i++) {
		if (buffer[i] == '\\')
			buffer[i] = '/';
	}

	/* Always set last zero. */
	buffer[err] = 0;

	/* If this is a junction, translate the link target. */
	if (rp->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT)
		err = ntfs_translate_junction(sb, link_de, buffer, err, buflen);

out:
	kfree(to_free);
	return err;
}

/* Archive entry point; VFS get_link ownership is supplied by the caller. */
int ntfs_archive_readlink(struct dentry *de, struct inode *inode, char *buffer, int capacity)
{
    return ntfs_readlink_hlp(de, inode, buffer, capacity);
}

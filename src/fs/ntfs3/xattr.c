// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 *
 */

#include "ntfseacompat.h"

static inline size_t unpacked_ea_size(const struct EA_FULL *ea)
{
	return ea->size ? le32_to_cpu(ea->size) :
			  ALIGN(struct_size(ea, name,
					    1 + ea->name_len +
						    le16_to_cpu(ea->elength)),
				4);
}

/*
 * find_ea
 *
 * Assume there is at least one xattr in the list.
 */
static inline bool find_ea(const struct EA_FULL *ea_all, u32 bytes,
			   const char *name, u8 name_len, u32 *off, u32 *ea_sz)
{
	u32 ea_size;

	*off = 0;
	if (!ea_all)
		return false;

	for (; *off < bytes; *off += ea_size) {
		const struct EA_FULL *ea = Add2Ptr(ea_all, *off);
		ea_size = unpacked_ea_size(ea);
		if (ea->name_len == name_len &&
		    !memcmp(ea->name, name, name_len)) {
			if (ea_sz)
				*ea_sz = ea_size;
			return true;
		}
	}

	return false;
}

/*
 * ntfs_read_ea - Read all extended attributes.
 * @ea:		New allocated memory.
 * @info:	Pointer into resident data.
 */
static int ntfs_read_ea(struct ntfs_inode *ni, struct EA_FULL **ea,
			size_t add_bytes, const struct EA_INFO **info)
{
	int err = -EINVAL;
	struct ntfs_sb_info *sbi = ni->mi.sbi;
	struct ATTR_LIST_ENTRY *le = NULL;
	struct ATTRIB *attr_info, *attr_ea;
	void *ea_p;
	u32 size, off, ea_size;

	static_assert(le32_to_cpu(ATTR_EA_INFO) < le32_to_cpu(ATTR_EA));

	*ea = NULL;
	*info = NULL;

	attr_info =
		ni_find_attr(ni, NULL, &le, ATTR_EA_INFO, NULL, 0, NULL, NULL);
	attr_ea =
		ni_find_attr(ni, attr_info, &le, ATTR_EA, NULL, 0, NULL, NULL);

	if (!attr_ea || !attr_info)
		return 0;

	*info = resident_data_ex(attr_info, sizeof(struct EA_INFO));
	if (!*info)
		goto out;

	/* Check Ea limit. */
	size = le32_to_cpu((*info)->size);
	if (size > sbi->ea_max_size) {
		err = -EFBIG;
		goto out;
	}

	if (attr_size(attr_ea) > sbi->ea_max_size) {
		err = -EFBIG;
		goto out;
	}

	if (!size) {
		/* EA info persists, but xattr is empty. Looks like EA problem. */
		goto out;
	}

	/* Allocate memory for packed Ea. */
	ea_p = kmalloc(size_add(size, add_bytes), GFP_NOFS);
	if (!ea_p)
		return -ENOMEM;

	if (attr_ea->non_res) {
		struct runs_tree run;

		run_init(&run);

		err = attr_load_runs_range(ni, ATTR_EA, NULL, 0, &run, 0, size);
		if (!err)
			err = ntfs_read_run_nb(sbi, &run, 0, ea_p, size, NULL);
		run_close(&run);

		if (err)
			goto out1;
	} else {
		void *p = resident_data_ex(attr_ea, size);

		if (!p)
			goto out1;
		memcpy(ea_p, p, size);
	}

	memset(Add2Ptr(ea_p, size), 0, add_bytes);

	err = -EINVAL;
	/* Check all attributes for consistency. */
	for (off = 0; off < size; off += ea_size) {
		const struct EA_FULL *ef = Add2Ptr(ea_p, off);
		u32 bytes = size - off;
		size_t need;

		/* Check if we can use field ea->size. */
		if (bytes < sizeof(ef->size))
			goto out1;

		/* Check if we can use fields ef->name_len and ef->elength. */
		if (bytes < offsetof(struct EA_FULL, name))
			goto out1;

		/* Size needed to hold this record's name and value. */
		need = struct_size(ef, name,
				   1 + ef->name_len + le16_to_cpu(ef->elength));

		if (ef->size) {
			ea_size = le32_to_cpu(ef->size);
			/* ef->size must fit the list and cover the record. */
			if (ea_size > bytes || ea_size < need)
				goto out1;
			continue;
		}

		ea_size = ALIGN(need, 4);
		if (ea_size > bytes)
			goto out1;
	}

	*ea = ea_p;
	return 0;

out1:
	kfree(ea_p);
out:
	ntfs_set_state(sbi, NTFS_DIRTY_DIRTY);
	return err;
}


/* ntfs_list_ea: emit each validated name/value directly to the archive adapter
 * instead of building a VFS name buffer and rereading the EA list per value. */
static int ntfs_list_ea(struct ntfs_inode *ni, sevenzip_ntfs_ea_visit_fn visit, void *context)
{
	const struct EA_INFO *info;
	struct EA_FULL *ea_all = NULL;
	u32 off, size;
	int err;

	err = ntfs_read_ea(ni, &ea_all, 0, &info);
	if (err)
		return err;
	if (!info || !ea_all)
		return 0;

	size = le32_to_cpu(info->size);
	off = 0;
	while (off + sizeof(struct EA_FULL) < size) {
		const struct EA_FULL *ea = Add2Ptr(ea_all, off);
		int ea_size = unpacked_ea_size(ea);
		u8 name_len = ea->name_len;

		if (!name_len)
			break;
		if (name_len > ea_size) {
			ntfs_set_state(ni->mi.sbi, NTFS_DIRTY_ERROR);
			err = -EINVAL;
			break;
		}
		if (off + ea_size > size)
			break;
		err = visit(context, (const char *)ea->name, name_len, ea->name + name_len + 1,
			    le16_to_cpu(ea->elength));
		if (err)
			goto out;
		off += ea_size;
	}
out:
	kfree(ea_all);
	return err;
}

static int ntfs_get_ea(struct inode *inode, const char *name, size_t name_len,
		       void *buffer, size_t size, size_t *required)
{
	struct ntfs_inode *ni = ntfs_i(inode);
	const struct EA_INFO *info;
	struct EA_FULL *ea_all = NULL;
	const struct EA_FULL *ea;
	u32 off, len;
	int err;

	if (!(ni->ni_flags & NI_FLAG_EA))
		return -ENODATA;

	if (!required)
		ni_lock(ni);

	len = 0;

	if (name_len > 255) {
		err = -ENAMETOOLONG;
		goto out;
	}

	err = ntfs_read_ea(ni, &ea_all, 0, &info);
	if (err)
		goto out;

	if (!info)
		goto out;

	/* Enumerate all xattrs. */
	if (!find_ea(ea_all, le32_to_cpu(info->size), name, name_len, &off,
		     NULL)) {
		err = -ENODATA;
		goto out;
	}
	ea = Add2Ptr(ea_all, off);

	len = le16_to_cpu(ea->elength);
	if (!buffer) {
		err = 0;
		goto out;
	}

	if (len > size) {
		err = -ERANGE;
		if (required)
			*required = len;
		goto out;
	}

	memcpy(buffer, ea->name + ea->name_len + 1, len);
	err = 0;

out:
	kfree(ea_all);
	if (!required)
		ni_unlock(ni);

	return err ? err : len;
}


void ntfs_get_wsl_perm(struct inode *inode)
{
	size_t sz;
	__le32 value[3];

	if (ntfs_get_ea(inode, "$LXUID", sizeof("$LXUID") - 1, &value[0],
			sizeof(value[0]), &sz) == sizeof(value[0]) &&
	    ntfs_get_ea(inode, "$LXGID", sizeof("$LXGID") - 1, &value[1],
			sizeof(value[1]), &sz) == sizeof(value[1]) &&
	    ntfs_get_ea(inode, "$LXMOD", sizeof("$LXMOD") - 1, &value[2],
			sizeof(value[2]), &sz) == sizeof(value[2])) {
		i_uid_write(inode, (uid_t)le32_to_cpu(value[0]));
		i_gid_write(inode, (gid_t)le32_to_cpu(value[1]));
		inode->i_mode = le32_to_cpu(value[2]);

		if (ntfs_get_ea(inode, "$LXDEV", sizeof("$LXDEV") - 1,
				&value[0], sizeof(value),
				&sz) == sizeof(value[0])) {
			inode->i_rdev = le32_to_cpu(value[0]);
		}
	}
}


static bool init_ea_context(const struct sevenzip_ntfs_ea_input *input,
                            struct ntfs_sb_info *sbi, struct ntfs_inode *ni)
{
    /* ntfs_read_mft enables the EA path only for a resident complete EA_INFO. */
    if (!input->info || input->info_size < sizeof(struct EA_INFO))
        return false;
    sbi->input = input;
    sbi->ea_max_size = input->max_size;
    ni->mi.sbi = sbi;
    ni->info.data = input->info;
    ni->info.bytes = input->info_size;
    ni->ea.data = input->resident;
    ni->ea.bytes = input->data_size;
    ni->ea.non_res = input->nonresident;
    if (!ni->ea.non_res && !ni->ea.data)
        return false; /* No matching ATTR_EA, as in ntfs_read_ea. */
    ni->vfs_inode.owner = ni;
    ni->ni_flags = NI_FLAG_EA;
    return true;
}

int sevenzip_ntfs_ea_visit(const struct sevenzip_ntfs_ea_input *input,
                          sevenzip_ntfs_ea_visit_fn visit, void *context)
{
    struct ntfs_sb_info sbi = {0};
    struct ntfs_inode ni = {0};
    if (!visit)
        return -EINVAL;
    if (!init_ea_context(input, &sbi, &ni))
        return 0;
    return ntfs_list_ea(&ni, visit, context);
}

int sevenzip_ntfs_wsl_read(const struct sevenzip_ntfs_ea_input *input,
                          struct sevenzip_ntfs_wsl_perm *result)
{
    struct ntfs_sb_info sbi = {0};
    struct ntfs_inode ni = {0};
    struct EA_FULL *ea = NULL;
    const struct EA_INFO *info = NULL;
    int err;
    memset(result, 0, sizeof(*result));
    if (!init_ea_context(input, &sbi, &ni))
        return 0;
    /* Eager archive validation reports corrupt EA/source errors at Open. */
    err = ntfs_read_ea(&ni, &ea, 0, &info);
    kfree(ea);
    if (err)
        return err;
    ntfs_get_wsl_perm(&ni.vfs_inode);
    if (sbi.error)
        return sbi.error;
    result->uid = ni.vfs_inode.i_uid;
    result->gid = ni.vfs_inode.i_gid;
    result->mode = ni.vfs_inode.i_mode;
    result->device = ni.vfs_inode.i_rdev;
    result->present = ni.vfs_inode.permissions;
    return 0;
}

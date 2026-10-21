// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 *
 */

#include "ntfswofcompat.h"

struct ATTR_LIST_ENTRY *al_enumerate(struct ntfs_inode *ni,
				     struct ATTR_LIST_ENTRY *le)
{
	size_t off;
	u16 sz;
	const unsigned le_min_size = le_size(0);

	if (!le) {
		le = ni->attr_list.le;
	} else {
		sz = le16_to_cpu(le->size);
		if (sz < le_min_size) {
			/* Impossible 'cause we should not return such le. */
			return NULL;
		}
		le = Add2Ptr(le, sz);
	}

	/* Check boundary. */
	off = PtrOffset(ni->attr_list.le, le);
	if (off + le_min_size > ni->attr_list.size) {
		/* The regular end of list. */
		return NULL;
	}

	sz = le16_to_cpu(le->size);

	/* Check le for errors. */
	if (sz < le_min_size || off + sz > ni->attr_list.size ||
	    sz < le->name_off + le->name_len * sizeof(short)) {
		return NULL;
	}

	return le;
}

struct ATTR_LIST_ENTRY *al_find_ex(struct ntfs_inode *ni,
				   struct ATTR_LIST_ENTRY *le,
				   enum ATTR_TYPE type, const __le16 *name,
				   u8 name_len, const CLST *vcn)
{
	struct ATTR_LIST_ENTRY *ret = NULL;
	u32 type_in = le32_to_cpu(type);

	ni = ni->base;
	while ((le = al_enumerate(ni, le))) {
		u64 le_vcn;
		int diff = le32_to_cpu(le->type) - type_in;

		/* List entries are sorted by type, name and VCN. */
		if (diff < 0)
			continue;

		if (diff > 0)
			return ret;

		if (le->name_len != name_len)
			continue;

		le_vcn = le64_to_cpu(le->vcn);
		if (!le_vcn) {
			/*
			 * Compare entry names only for entry with vcn == 0.
			 */
			diff = ntfs_cmp_names(le_name(le), name_len, name,
					      name_len, ni->mi.sbi->upcase,
					      true);
			if (diff < 0)
				continue;

			if (diff > 0)
				return ret;
		}

		if (!vcn)
			return le;

		if (*vcn == le_vcn)
			return le;

		if (*vcn < le_vcn)
			return ret;

		ret = le;
	}

	return ret;
}

static void project_entry(const struct ATTR_LIST_ENTRY *le,
                           struct sevenzip_ntfs_list_entry *entry)
{
    entry->type = le32_to_cpu(le->type);
    entry->ref = get_unaligned_le64(&le->ref);
    entry->vcn = le64_to_cpu(le->vcn);
    entry->id = le16_to_cpu(le->id);
    entry->name_len = le->name_len;
    entry->name = le_name(le);
}

int sevenzip_ntfs_list_next(const void *data, size_t size, size_t *cursor,
                            struct sevenzip_ntfs_list_entry *entry)
{
    struct ntfs_inode ni = {0};
    struct ATTR_LIST_ENTRY *le, *prev;
    size_t next;
    if (*cursor != SIZE_MAX && *cursor >= size) return -EINVAL;
    ni.attr_list.le = (struct ATTR_LIST_ENTRY *)data;
    ni.attr_list.size = size;
    prev = *cursor == SIZE_MAX ? NULL : Add2Ptr(data, *cursor);
    next = prev ? *cursor + le16_to_cpu(prev->size) : 0;
    le = al_enumerate(&ni, prev);
    if (!le) return next <= size && size - next < le_size(0) ? 0 : -EINVAL;
    *cursor = PtrOffset(data, le);
    project_entry(le, entry);
    return 1;
}

int sevenzip_ntfs_list_find_unnamed(const void *data, size_t size, uint32_t type,
                                    struct sevenzip_ntfs_list_entry *entry)
{
    struct ntfs_sb_info sbi = {0};
    struct ntfs_inode ni = {0};
    struct ATTR_LIST_ENTRY *le;
    struct sevenzip_ntfs_list_entry checked;
    size_t cursor = SIZE_MAX;
    uint32_t previous = 0;
    int next;
    /* Validate the complete bounded list before its sorted lookup is used. */
    while ((next = sevenzip_ntfs_list_next(data, size, &cursor, &checked)) > 0) {
        if (checked.type < previous)
            return -EINVAL;
        previous = checked.type;
    }
    if (next < 0)
        return next;
    ni.base = &ni;
    ni.mi.sbi = &sbi;
    ni.attr_list.le = (struct ATTR_LIST_ENTRY *)data;
    ni.attr_list.size = size;
    le = al_find_ex(&ni, NULL, cpu_to_le32(type), NULL, 0, NULL);
    if (!le)
        return 0;
    project_entry(le, entry);
    return 1;
}

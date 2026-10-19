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
    entry->type = le32_to_cpu(le->type);
    entry->ref = get_unaligned_le64(&le->ref);
    entry->vcn = le64_to_cpu(le->vcn);
    entry->id = le16_to_cpu(le->id);
    entry->name_len = le->name_len;
    return 1;
}

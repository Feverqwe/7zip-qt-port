// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2002,2003 by Andreas Gruenbacher <a.gruenbacher@computer.org>
 *
 * Fixes from William Schumacher incorporated on 15 March 2001.
 *    (Reported by Charles Bertsch, <CBertsch@microtest.com>).
 */

/*
 *  This file contains generic functions for manipulating
 *  POSIX 1003.1e draft standard 17 ACLs.
 */

#include "ext4/aclcompat.h"

void *
posix_acl_to_xattr(struct user_namespace *user_ns, const struct posix_acl *acl,
		   size_t *sizep, gfp_t gfp)
{
	struct posix_acl_xattr_header *ext_acl;
	struct posix_acl_xattr_entry *ext_entry;
	size_t size;
	int n;

	size = posix_acl_xattr_size(acl->a_count);
	ext_acl = kmalloc(size, gfp);
	if (!ext_acl)
		return NULL;

	ext_entry = (void *)(ext_acl + 1);
	ext_acl->a_version = cpu_to_le32(POSIX_ACL_XATTR_VERSION);

	for (n=0; n < acl->a_count; n++, ext_entry++) {
		const struct posix_acl_entry *acl_e = &acl->a_entries[n];
		ext_entry->e_tag  = cpu_to_le16(acl_e->e_tag);
		ext_entry->e_perm = cpu_to_le16(acl_e->e_perm);
		switch(acl_e->e_tag) {
		case ACL_USER:
			ext_entry->e_id =
				cpu_to_le32(from_kuid(user_ns, acl_e->e_uid));
			break;
		case ACL_GROUP:
			ext_entry->e_id =
				cpu_to_le32(from_kgid(user_ns, acl_e->e_gid));
			break;
		default:
			ext_entry->e_id = cpu_to_le32(ACL_UNDEFINED_ID);
			break;
		}
	}
	*sizep = size;
	return ext_acl;
}

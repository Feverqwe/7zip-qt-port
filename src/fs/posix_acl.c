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

#include "posix_acl_compat.h"

static int posix_acl_fix_xattr_common(const void *value, size_t size)
{
	const struct posix_acl_xattr_header *header = value;
	int count;

	if (!header)
		return -EINVAL;
	if (size < sizeof(struct posix_acl_xattr_header))
		return -EINVAL;
	if (header->a_version != cpu_to_le32(POSIX_ACL_XATTR_VERSION))
		return -EOPNOTSUPP;

	count = posix_acl_xattr_count(size);
	if (count < 0)
		return -EINVAL;

	return count;
}

/**
 * posix_acl_from_xattr - convert POSIX ACLs from backing store to VFS format
 * @userns: the filesystem's idmapping
 * @value: the uapi representation of POSIX ACLs
 * @size: the size of @void
 *
 * Filesystems that store POSIX ACLs in the unaltered uapi format should use
 * posix_acl_from_xattr() when reading them from the backing store and
 * converting them into the struct posix_acl VFS format. The helper is
 * specifically intended to be called from the acl inode operation.
 *
 * The posix_acl_from_xattr() function will map the raw {g,u}id values stored
 * in ACL_{GROUP,USER} entries into idmapping in @userns.
 *
 * Note that posix_acl_from_xattr() does not take idmapped mounts into account.
 * If it did it calling it from the get acl inode operation would return POSIX
 * ACLs mapped according to an idmapped mount which would mean that the value
 * couldn't be cached for the filesystem. Idmapped mounts are taken into
 * account on the fly during permission checking or right at the VFS -
 * userspace boundary before reporting them to the user.
 *
 * Return: Allocated struct posix_acl on success, NULL for a valid header but
 *         without actual POSIX ACL entries, or ERR_PTR() encoded error code.
 */
struct posix_acl *posix_acl_from_xattr(struct user_namespace *userns,
				       const void *value, size_t size)
{
	const struct posix_acl_xattr_header *header = value;
	const struct posix_acl_xattr_entry *entry = (const void *)(header + 1), *end;
	int count;
	struct posix_acl *acl;
	struct posix_acl_entry *acl_e;

	count = posix_acl_fix_xattr_common(value, size);
	if (count < 0)
		return ERR_PTR(count);
	if (count == 0)
		return NULL;

	acl = posix_acl_alloc(count, GFP_NOFS);
	if (!acl)
		return ERR_PTR(-ENOMEM);
	acl_e = acl->a_entries;

	for (end = entry + count; entry != end; acl_e++, entry++) {
		acl_e->e_tag  = le16_to_cpu(entry->e_tag);
		acl_e->e_perm = le16_to_cpu(entry->e_perm);

		switch(acl_e->e_tag) {
			case ACL_USER_OBJ:
			case ACL_GROUP_OBJ:
			case ACL_MASK:
			case ACL_OTHER:
				break;

			case ACL_USER:
				acl_e->e_uid = make_kuid(userns,
						le32_to_cpu(entry->e_id));
				if (!uid_valid(acl_e->e_uid))
					goto fail;
				break;
			case ACL_GROUP:
				acl_e->e_gid = make_kgid(userns,
						le32_to_cpu(entry->e_id));
				if (!gid_valid(acl_e->e_gid))
					goto fail;
				break;

			default:
				goto fail;
		}
	}
	return acl;

fail:
	posix_acl_release(acl);
	return ERR_PTR(-EINVAL);
}

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

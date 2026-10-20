/* SPDX-License-Identifier: GPL-2.0-only */
/* Numeric filesystem IDs and allocation for the transferred ACL converters. */
#ifndef SEVENZIP_EXT4_ACL_COMPAT_H
#define SEVENZIP_EXT4_ACL_COMPAT_H
#include "ext4.h"
#ifndef ERR_PTR
#define ERR_PTR(error) ((void *)(intptr_t)(error))
#define PTR_ERR(pointer) ((int)(intptr_t)(pointer))
#define IS_ERR(pointer) ((uintptr_t)(pointer) >= (uintptr_t)-4095)
#endif
#define ACL_USER_OBJ 0x01
#define ACL_USER 0x02
#define ACL_GROUP_OBJ 0x04
#define ACL_GROUP 0x08
#define ACL_MASK 0x10
#define ACL_OTHER 0x20
#define ACL_UNDEFINED_ID (-1)
#define POSIX_ACL_XATTR_VERSION 0x0002
typedef int gfp_t;
struct user_namespace { int unused; };
static struct user_namespace init_user_ns;
struct posix_acl_entry { int e_tag; u16 e_perm; u32 e_uid, e_gid; };
struct posix_acl { unsigned a_count; struct posix_acl_entry a_entries[]; };
struct posix_acl_xattr_entry { __le16 e_tag, e_perm; __le32 e_id; };
struct posix_acl_xattr_header { __le32 a_version; };
static inline struct posix_acl *posix_acl_alloc(size_t count, int flags) {
    struct posix_acl *acl;
    (void)flags;
    if (count > UINT_MAX || count > (SIZE_MAX - sizeof(*acl)) / sizeof(acl->a_entries[0])) return NULL;
    acl = calloc(1, sizeof(*acl) + count * sizeof(acl->a_entries[0]));
    if (acl) acl->a_count = (unsigned)count;
    return acl;
}
#define posix_acl_release free
#define make_kuid(ns, id) (id)
#define make_kgid(ns, id) (id)
#define from_kuid(ns, id) (id)
#define from_kgid(ns, id) (id)
static inline size_t posix_acl_xattr_size(unsigned count) {
    return sizeof(struct posix_acl_xattr_header) + count * sizeof(struct posix_acl_xattr_entry);
}
void *posix_acl_to_xattr(struct user_namespace *, const struct posix_acl *, size_t *, gfp_t);
int ext4_acl_xattr_value(const void *, size_t, void **, size_t *);
#endif

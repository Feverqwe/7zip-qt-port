/* SPDX-License-Identifier: GPL-2.0-only */
/* Numeric filesystem IDs and allocation for the transferred ACL converters. */
#ifndef SEVENZIP_POSIX_ACL_COMPAT_H
#define SEVENZIP_POSIX_ACL_COMPAT_H
#include "compat/linux/include/uapi/linux/posix_acl.h"
#include "compat/linux/include/uapi/linux/posix_acl_xattr.h"
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#ifndef cpu_to_le32
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define cpu_to_le16(x) __builtin_bswap16(x)
#define cpu_to_le32(x) __builtin_bswap32(x)
#define le16_to_cpu(x) __builtin_bswap16(x)
#define le32_to_cpu(x) __builtin_bswap32(x)
#else
#define cpu_to_le16(x) (x)
#define cpu_to_le32(x) (x)
#define le16_to_cpu(x) (x)
#define le32_to_cpu(x) (x)
#endif
#endif
#ifndef GFP_NOFS
#define GFP_NOFS 0
#endif
#ifndef kmalloc
#define kmalloc(size, flags) malloc(size)
#endif
#ifndef ERR_PTR
#define ERR_PTR(error) ((void *)(intptr_t)(error))
#define PTR_ERR(pointer) ((int)(intptr_t)(pointer))
#define IS_ERR(pointer) ((uintptr_t)(pointer) >= (uintptr_t)-4095)
#endif
typedef int gfp_t;
struct user_namespace {
    int unused;
};
static struct user_namespace init_user_ns;
struct posix_acl_entry {
    int e_tag;
    uint16_t e_perm;
    uint32_t e_uid, e_gid;
};
struct posix_acl {
    unsigned a_count;
    struct posix_acl_entry a_entries[];
};
static inline struct posix_acl *posix_acl_alloc(size_t count, int flags) {
    struct posix_acl *acl;
    (void)flags;
    if (count > UINT_MAX || count > (SIZE_MAX - sizeof(*acl)) / sizeof(acl->a_entries[0]))
        return NULL;
    acl = calloc(1, sizeof(*acl) + count * sizeof(acl->a_entries[0]));
    if (acl)
        acl->a_count = (unsigned)count;
    return acl;
}
#define posix_acl_release free
#define make_kuid(ns, id) (id)
#define make_kgid(ns, id) (id)
#define from_kuid(ns, id) (id)
#define from_kgid(ns, id) (id)
#define uid_valid(id) ((id) != UINT32_MAX)
#define gid_valid(id) ((id) != UINT32_MAX)
static inline int posix_acl_xattr_count(size_t size) {
    if (size < sizeof(struct posix_acl_xattr_header))
        return -1;
    size -= sizeof(struct posix_acl_xattr_header);
    if (size % sizeof(struct posix_acl_xattr_entry))
        return -1;
    return (int)(size / sizeof(struct posix_acl_xattr_entry));
}
static inline size_t posix_acl_xattr_size(unsigned count) {
    return sizeof(struct posix_acl_xattr_header) + count * sizeof(struct posix_acl_xattr_entry);
}
void *posix_acl_to_xattr(struct user_namespace *, const struct posix_acl *, size_t *, gfp_t);
struct posix_acl *posix_acl_from_xattr(struct user_namespace *, const void *, size_t);
#endif

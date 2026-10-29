/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2017-2018 HUAWEI, Inc.
 *             https://www.huawei.com/
 */
#ifndef __EROFS_XATTR_H
#define __EROFS_XATTR_H
#include "internal.h"

// Load EROFS services before the generic ACL fallback macros.
#include "../posix_acl_compat.h"
/* Private VFS namespace services: archive reads expose privileged metadata. */
struct dentry {
    struct super_block *d_sb;
    struct inode *inode;
};
#define d_inode(entry) ((entry)->inode)
struct xattr_handler {
    const char *prefix, *name;
    int flags;
    bool (*list)(struct dentry *);
    int (*get)(const struct xattr_handler *, struct dentry *, struct inode *, const char *, void *,
               size_t);
};
#define XATTR_USER_PREFIX "user."
#define XATTR_TRUSTED_PREFIX "trusted."
#define XATTR_SECURITY_PREFIX "security."
#define capable(cap) true
static const struct xattr_handler nop_posix_acl_access = {.name = "system.posix_acl_access"};
static const struct xattr_handler nop_posix_acl_default = {.name = "system.posix_acl_default"};
static inline bool xattr_handler_can_list(const struct xattr_handler *handler,
                                          struct dentry *entry) {
    return handler && (!handler->list || handler->list(entry));
}
static inline const char *xattr_prefix(const struct xattr_handler *handler) {
    return handler->prefix ? handler->prefix : handler->name;
}
extern const struct xattr_handler *const erofs_xattr_handlers[];
int erofs_xattr_prefixes_init(struct super_block *);
void erofs_xattr_prefixes_cleanup(struct super_block *);
ptrdiff_t erofs_listxattr(struct dentry *, char *, size_t);
int erofs_getxattr(struct inode *, int, const char *, void *, size_t);
struct posix_acl *erofs_get_acl(struct inode *, int, bool);
u32 xxh32(const void *, size_t, u32);
#endif

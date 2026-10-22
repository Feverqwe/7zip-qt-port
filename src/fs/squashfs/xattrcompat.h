// SPDX-License-Identifier: GPL-2.0-or-later
// Archive visibility and callback services for the Linux xattr reader.
#ifndef SEVENZIP_SQUASHFS_XATTR_COMPAT_H
#define SEVENZIP_SQUASHFS_XATTR_COMPAT_H
#ifdef _MSC_VER
typedef intptr_t ssize_t;
#else
#include <sys/types.h>
#endif
#ifndef ENODATA
#define ENODATA 61
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP 95
#endif
#define XATTR_USER_PREFIX "user."
#define XATTR_TRUSTED_PREFIX "trusted."
#define XATTR_SECURITY_PREFIX "security."
struct dentry { struct inode *inode; };
static inline struct inode *d_inode(struct dentry *d) { return d->inode; }
// Archive metadata does not depend on the host's namespace privileges.
#define CAP_SYS_ADMIN 21
#define capable(cap) (true)
struct xattr_handler {
    const char *prefix;
    const char *name;
    int flags;
    bool (*list)(struct dentry *);
    int (*get)(const struct xattr_handler *, struct dentry *, struct inode *, const char *, void *, size_t);
};
extern const struct xattr_handler * const squashfs_xattr_handlers[];
ssize_t squashfs_listxattr(struct dentry *, char *, size_t);
ssize_t squashfs_listxattr_visit(struct dentry *, char *, size_t,
    int (*)(void *, const char *), void *);
#endif

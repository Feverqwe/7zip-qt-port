/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SEVENZIP_EXT4_ACL_COMPAT_H
#define SEVENZIP_EXT4_ACL_COMPAT_H
#include "ext4.h"

// Load ext4 endian functions before the generic ACL fallback macros.
#include "../posix_acl_compat.h"
int ext4_acl_xattr_value(const void *, size_t, void **, size_t *);
#endif

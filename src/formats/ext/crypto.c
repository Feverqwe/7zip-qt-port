// SPDX-License-Identifier: GPL-2.0

#include "formats/ext/ext4.h"
#include "formats/ext/xattr.h"

int ext4_get_context(struct inode *inode, void *ctx, size_t len)
{
	return ext4_xattr_get(inode, EXT4_XATTR_INDEX_ENCRYPTION,
				 EXT4_XATTR_NAME_ENCRYPTION_CONTEXT, ctx, len);
}

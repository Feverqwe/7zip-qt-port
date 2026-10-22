/* SPDX-License-Identifier: GPL-2.0 */
/* Selected superblock UUID helper; VFS types/services are supplied by the host. */
#ifndef SEVENZIP_LINUX_FS_H
#define SEVENZIP_LINUX_FS_H

static inline void super_set_uuid(struct super_block *sb, const u8 *uuid, unsigned len)
{
	if (WARN_ON(len > sizeof(sb->s_uuid)))
		len = sizeof(sb->s_uuid);
	sb->s_uuid_len = len;
	memcpy(&sb->s_uuid, uuid, len);
}

#endif

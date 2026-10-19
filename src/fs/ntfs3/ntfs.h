/* SPDX-License-Identifier: GPL-2.0 */
/*
 *
 * Copyright (C) 2019-2021 Paragon Software GmbH, All rights reserved.
 *
 * on-disk ntfs structs
 */

#ifndef SEVENZIP_LINUX_NTFS_READ_TYPES_H
#define SEVENZIP_LINUX_NTFS_READ_TYPES_H
#include "ntfscompat.h"

enum ATTR_TYPE {
	ATTR_ZERO		= cpu_to_le32(0x00),
	ATTR_STD		= cpu_to_le32(0x10),
	ATTR_LIST		= cpu_to_le32(0x20),
	ATTR_NAME		= cpu_to_le32(0x30),
	ATTR_ID			= cpu_to_le32(0x40),
	ATTR_SECURE		= cpu_to_le32(0x50),
	ATTR_LABEL		= cpu_to_le32(0x60),
	ATTR_VOL_INFO		= cpu_to_le32(0x70),
	ATTR_DATA		= cpu_to_le32(0x80),
	ATTR_ROOT		= cpu_to_le32(0x90),
	ATTR_ALLOC		= cpu_to_le32(0xA0),
	ATTR_BITMAP		= cpu_to_le32(0xB0),
	ATTR_REPARSE		= cpu_to_le32(0xC0),
	ATTR_EA_INFO		= cpu_to_le32(0xD0),
	ATTR_EA			= cpu_to_le32(0xE0),
	ATTR_PROPERTYSET	= cpu_to_le32(0xF0),
	ATTR_LOGGED_UTILITY_STREAM = cpu_to_le32(0x100),
	ATTR_END		= cpu_to_le32(0xFFFFFFFF)
};

struct MFT_REF {
	__le32 low;	// The low part of the number.
	__le16 high;	// The high part of the number.
	__le16 seq;	// The sequence number of MFT record.
};

struct ATTR_LIST_ENTRY {
	enum ATTR_TYPE type;	// 0x00: The type of attribute.
	__le16 size;		// 0x04: The size of this record.
	u8 name_len;		// 0x06: The length of attribute name.
	u8 name_off;		// 0x07: The offset to attribute name.
	__le64 vcn;		// 0x08: Starting VCN of this attribute.
	struct MFT_REF ref;	// 0x10: MFT record number with attribute.
	__le16 id;		// 0x18: struct ATTRIB ID.
	__le16 name[];		// 0x1A: To get real name use name_off.

};

static inline u32 le_size(u8 name_len)
{
	return ALIGN(offsetof(struct ATTR_LIST_ENTRY, name) +
		     name_len * sizeof(short), 8);
}
#endif

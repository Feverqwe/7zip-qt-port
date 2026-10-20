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

enum COLLATION_RULE {
	NTFS_COLLATION_TYPE_BINARY	= cpu_to_le32(0),
	// $I30
	NTFS_COLLATION_TYPE_FILENAME	= cpu_to_le32(0x01),
	// $SII of $Secure and $Q of Quota
	NTFS_COLLATION_TYPE_UINT	= cpu_to_le32(0x10),
	// $O of Quota
	NTFS_COLLATION_TYPE_SID		= cpu_to_le32(0x11),
	// $SDH of $Secure
	NTFS_COLLATION_TYPE_SECURITY_HASH = cpu_to_le32(0x12),
	// $O of ObjId and "$R" for Reparse
	NTFS_COLLATION_TYPE_UINTS	= cpu_to_le32(0x13)
};

_Static_assert(sizeof(enum COLLATION_RULE) == 4, "COLLATION_RULE");

/* $AttrDef file entry. */
struct ATTR_DEF_ENTRY {
	__le16 name[0x40];	// 0x00: Attr name.
	enum ATTR_TYPE type;	// 0x80: struct ATTRIB type.
	__le32 res;		// 0x84:
	enum COLLATION_RULE rule; // 0x88:
	__le32 flags;		// 0x8C: NTFS_ATTR_XXX (see above).
	__le64 min_sz;		// 0x90: Minimum attribute data size.
	__le64 max_sz;		// 0x98: Maximum attribute data size.
};

_Static_assert(sizeof(struct ATTR_DEF_ENTRY) == 0xa0, "ATTR_DEF_ENTRY");

/* ATTR_EA_INFO (0xD0) */

#define FILE_NEED_EA 0x80 // See ntifs.h
/*
 * FILE_NEED_EA, indicates that the file to which the EA belongs cannot be
 * interpreted without understanding the associated extended attributes.
 */
struct EA_INFO {
	__le16 size_pack;	// 0x00: Size of buffer to hold in packed form.
	__le16 count;		// 0x02: Count of EA's with FILE_NEED_EA bit set.
	__le32 size;		// 0x04: Size of buffer to hold in unpacked form.
};

_Static_assert(sizeof(struct EA_INFO) == 8, "EA_INFO");

/* ATTR_EA (0xE0) */
struct EA_FULL {
	__le32 size;		// 0x00: (not in packed)
	u8 flags;		// 0x04:
	u8 name_len;		// 0x05:
	__le16 elength;		// 0x06:
	u8 name[];		// 0x08:
};

_Static_assert(offsetof(struct EA_FULL, name) == 8, "EA_FULL");
#endif

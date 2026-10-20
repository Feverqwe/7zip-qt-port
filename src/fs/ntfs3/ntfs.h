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

typedef u64 CLST;

enum NTFS_SIGNATURE {
	NTFS_FILE_SIGNATURE = cpu_to_le32(0x454C4946), // 'FILE'
	NTFS_INDX_SIGNATURE = cpu_to_le32(0x58444E49), // 'INDX'
	NTFS_CHKD_SIGNATURE = cpu_to_le32(0x444B4843), // 'CHKD'
	NTFS_RSTR_SIGNATURE = cpu_to_le32(0x52545352), // 'RSTR'
	NTFS_RCRD_SIGNATURE = cpu_to_le32(0x44524352), // 'RCRD'
	NTFS_BAAD_SIGNATURE = cpu_to_le32(0x44414142), // 'BAAD'
	NTFS_HOLE_SIGNATURE = cpu_to_le32(0x454C4F48), // 'HOLE'
	NTFS_FFFF_SIGNATURE = cpu_to_le32(0xffffffff),
};

_Static_assert(sizeof(enum NTFS_SIGNATURE) == 4, "NTFS disk layout");

/* MFT Record header structure. */
struct NTFS_RECORD_HEADER {
	/* Record magic number, equals 'FILE'/'INDX'/'RSTR'/'RCRD'. */
	enum NTFS_SIGNATURE sign; // 0x00:
	__le16 fix_off;		// 0x04:
	__le16 fix_num;		// 0x06:
	__le64 lsn;		// 0x08: Log file sequence number,
};

_Static_assert(sizeof(struct NTFS_RECORD_HEADER) == 0x10, "NTFS disk layout");

/* Index entry defines ( the field flags in NtfsDirEntry ). */
#define NTFS_IE_HAS_SUBNODES	cpu_to_le16(1)
#define NTFS_IE_LAST		cpu_to_le16(2)

/* Directory entry structure. */
struct NTFS_DE {
	union {
		struct MFT_REF ref; // 0x00: MFT record number with this file.
		struct {
			__le16 data_off;  // 0x00:
			__le16 data_size; // 0x02:
			__le32 res;	  // 0x04: Must be 0.
		} view;
	};
	__le16 size;		// 0x08: The size of this entry.
	__le16 key_size;	// 0x0A: The size of File name length in bytes + 0x42.
	__le16 flags;		// 0x0C: Entry flags: NTFS_IE_XXX.
	__le16 res;		// 0x0E:

	// Here any indexed attribute can be placed.
	// One of them is:
	// struct ATTR_FILE_NAME AttrFileName;
	//

	// The last 8 bytes of this structure contains
	// the VBN of subnode.
	// !!! Note !!!
	// This field is presented only if (flags & NTFS_IE_HAS_SUBNODES)
	// __le64 vbn;
};

_Static_assert(sizeof(struct NTFS_DE) == 0x10, "NTFS disk layout");

static inline __le64 de_get_vbn_le(const struct NTFS_DE *e)
{
	return *(__le64 *)Add2Ptr(e, le16_to_cpu(e->size) - sizeof(__le64));
}

static inline CLST de_get_vbn(const struct NTFS_DE *e)
{
	__le64 *v = Add2Ptr(e, le16_to_cpu(e->size) - sizeof(__le64));

	return le64_to_cpu(*v);
}

static inline struct NTFS_DE *de_get_next(const struct NTFS_DE *e)
{
	return Add2Ptr(e, le16_to_cpu(e->size));
}

static inline bool de_is_last(const struct NTFS_DE *e)
{
	return e->flags & NTFS_IE_LAST;
}

static inline bool de_has_vcn(const struct NTFS_DE *e)
{
	return e->flags & NTFS_IE_HAS_SUBNODES;
}

static inline bool de_has_vcn_ex(const struct NTFS_DE *e)
{
	return (e->flags & NTFS_IE_HAS_SUBNODES) &&
	       (u64)(-1) != *((u64 *)Add2Ptr(e, le16_to_cpu(e->size) -
							sizeof(__le64)));
}

#define MAX_BYTES_PER_NAME_ENTRY \
	ALIGN(sizeof(struct NTFS_DE) + \
	      offsetof(struct ATTR_FILE_NAME, name) + \
	      NTFS_NAME_LEN * sizeof(short), 8)

#define NTFS_INDEX_HDR_HAS_SUBNODES cpu_to_le32(1)

struct INDEX_HDR {
	__le32 de_off;	// 0x00: The offset from the start of this structure
			// to the first NTFS_DE.
	__le32 used;	// 0x04: The size of this structure plus all
			// entries (quad-word aligned).
	__le32 total;	// 0x08: The allocated size of for this structure plus all entries.
	__le32 flags;	// 0x0C: 0x00 = Small directory, 0x01 = Large directory.

	//
	// de_off + used <= total
	//
};

_Static_assert(sizeof(struct INDEX_HDR) == 0x10, "NTFS disk layout");

static inline struct NTFS_DE *hdr_first_de(const struct INDEX_HDR *hdr)
{
	u32 de_off = le32_to_cpu(hdr->de_off);
	u32 used = le32_to_cpu(hdr->used);
	struct NTFS_DE *e;
	u16 esize;

	if (de_off >= used || size_add(de_off, sizeof(struct NTFS_DE)) > used)
		return NULL;

	e = Add2Ptr(hdr, de_off);
	esize = le16_to_cpu(e->size);
	if (esize < sizeof(struct NTFS_DE) || de_off + esize > used)
		return NULL;

	return e;
}

static inline struct NTFS_DE *hdr_next_de(const struct INDEX_HDR *hdr,
					  const struct NTFS_DE *e)
{
	size_t off = PtrOffset(hdr, e);
	u32 used = le32_to_cpu(hdr->used);
	u16 esize;

	if (off >= used)
		return NULL;

	esize = le16_to_cpu(e->size);

	if (esize < sizeof(struct NTFS_DE) ||
	    off + esize + sizeof(struct NTFS_DE) > used)
		return NULL;

	return Add2Ptr(e, esize);
}

static inline bool hdr_has_subnode(const struct INDEX_HDR *hdr)
{
	return hdr->flags & NTFS_INDEX_HDR_HAS_SUBNODES;
}

struct INDEX_BUFFER {
	struct NTFS_RECORD_HEADER rhdr; // 'INDX'
	__le64 vbn; // 0x10: vcn if index >= cluster or vsn id index < cluster
	struct INDEX_HDR ihdr; // 0x18:
};

_Static_assert(sizeof(struct INDEX_BUFFER) == 0x28, "NTFS disk layout");

struct INDEX_ROOT {
	enum ATTR_TYPE type;	// 0x00: The type of attribute to index on.
	enum COLLATION_RULE rule; // 0x04: The rule.
	__le32 index_block_size;// 0x08: The size of index record.
	u8 index_block_clst;	// 0x0C: The number of clusters or sectors per index.
	u8 res[3];
	struct INDEX_HDR ihdr;	// 0x10:
};

_Static_assert(sizeof(struct INDEX_ROOT) == 0x20, "NTFS disk layout");

#ifdef _MSC_VER
#pragma pack(push, 1)
#define sevenzip_ntfs_packed
#else
#define sevenzip_ntfs_packed __attribute__((__packed__))
#endif
struct SECURITY_KEY {
	__le32 hash; //  Hash value for descriptor
	__le32 sec_id; //  Security Id (guaranteed unique)
};

/* Security descriptors (the content of $Secure::SDS data stream) */
struct SECURITY_HDR {
	struct SECURITY_KEY key;	// 0x00: Security Key.
	__le64 off;			// 0x08: Offset of this entry in the file.
	__le32 size;			// 0x10: Size of this entry, 8 byte aligned.
	/*
	 * Security descriptor itself is placed here.
	 * Total size is 16 byte aligned.
	 */
} sevenzip_ntfs_packed;

_Static_assert(sizeof(struct SECURITY_HDR) == 0x14, "NTFS disk layout");

/* SII Directory entry structure */
struct NTFS_DE_SII {
	struct NTFS_DE de;
	__le32 sec_id;			// 0x10: Key: sizeof(security_id) = wKeySize
	struct SECURITY_HDR sec_hdr;	// 0x14:
} sevenzip_ntfs_packed;

_Static_assert(offsetof(struct NTFS_DE_SII, sec_hdr) == 0x14, "NTFS disk layout");
_Static_assert(sizeof(struct NTFS_DE_SII) == 0x28, "NTFS disk layout");
#ifdef _MSC_VER
#pragma pack(pop)
#endif
#undef sevenzip_ntfs_packed

#define ACL_REVISION	2
#define ACL_REVISION_DS 4

#define SE_SELF_RELATIVE cpu_to_le16(0x8000)

struct SECURITY_DESCRIPTOR_RELATIVE {
	u8 Revision;
	u8 Sbz1;
	__le16 Control;
	__le32 Owner;
	__le32 Group;
	__le32 Sacl;
	__le32 Dacl;
};
_Static_assert(sizeof(struct SECURITY_DESCRIPTOR_RELATIVE) == 0x14, "NTFS disk layout");

struct ACE_HEADER {
	u8 AceType;
	u8 AceFlags;
	__le16 AceSize;
};
_Static_assert(sizeof(struct ACE_HEADER) == 4, "NTFS disk layout");

struct ACL {
	u8 AclRevision;
	u8 Sbz1;
	__le16 AclSize;
	__le16 AceCount;
	__le16 Sbz2;
};
_Static_assert(sizeof(struct ACL) == 8, "NTFS disk layout");

struct SID {
	u8 Revision;
	u8 SubAuthorityCount;
	u8 IdentifierAuthority[6];
	__le32 SubAuthority[];
};
_Static_assert(offsetof(struct SID, SubAuthority) == 8, "NTFS disk layout");
struct GUID {
	__le32 Data1;
	__le16 Data2;
	__le16 Data3;
	u8 Data4[8];
};

#define WOF_CURRENT_VERSION		cpu_to_le32(1)
/* CompressReparseBuffer.WofProvider */
#define WOF_PROVIDER_WIM		cpu_to_le32(1)
/* CompressReparseBuffer.WofProvider */
#define WOF_PROVIDER_SYSTEM		cpu_to_le32(2)
/* CompressReparseBuffer.ProviderVer */
#define WOF_PROVIDER_CURRENT_VERSION	cpu_to_le32(1)

#define WOF_COMPRESSION_XPRESS4K	cpu_to_le32(0) // 4k
#define WOF_COMPRESSION_LZX32K		cpu_to_le32(1) // 32k
#define WOF_COMPRESSION_XPRESS8K	cpu_to_le32(2) // 8k
#define WOF_COMPRESSION_XPRESS16K	cpu_to_le32(3) // 16k

/*
 * ATTR_REPARSE (0xC0)
 *
 * The reparse struct GUID structure is used by all 3rd party layered drivers to
 * store data in a reparse point. For non-Microsoft tags, The struct GUID field
 * cannot be GUID_NULL.
 * The constraints on reparse tags are defined below.
 * Microsoft tags can also be used with this format of the reparse point buffer.
 */
struct REPARSE_POINT {
	__le32 ReparseTag;	// 0x00:
	__le16 ReparseDataLength;// 0x04:
	__le16 Reserved;

	struct GUID Guid;	// 0x08:

	//
	// Here GenericReparseBuffer is placed
	//
};

_Static_assert(sizeof(struct REPARSE_POINT) == 0x18, "NTFS reparse layout");

/*
 * The value of the following constant needs to satisfy the following
 * conditions:
 *  (1) Be at least as large as the largest of the reserved tags.
 *  (2) Be strictly smaller than all the tags in use.
 */
#define IO_REPARSE_TAG_RESERVED_RANGE		1

/*
 * The reparse tags are a ULONG. The 32 bits are laid out as follows:
 *
 *   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *  +-+-+-+-+-----------------------+-------------------------------+
 *  |M|R|N|R|	  Reserved bits     |	    Reparse Tag Value	    |
 *  +-+-+-+-+-----------------------+-------------------------------+
 *
 * M is the Microsoft bit. When set to 1, it denotes a tag owned by Microsoft.
 *   All ISVs must use a tag with a 0 in this position.
 *   Note: If a Microsoft tag is used by non-Microsoft software, the
 *   behavior is not defined.
 *
 * R is reserved.  Must be zero for non-Microsoft tags.
 *
 * N is name surrogate. When set to 1, the file represents another named
 *   entity in the system.
 *
 * The M and N bits are OR-able.
 * The following macros check for the M and N bit values:
 */

/*
 * Macro to determine whether a reparse point tag corresponds to a tag
 * owned by Microsoft.
 */
#define IsReparseTagMicrosoft(_tag)	(((_tag)&IO_REPARSE_TAG_MICROSOFT))

/* Macro to determine whether a reparse point tag is a name surrogate. */
#define IsReparseTagNameSurrogate(_tag)	(((_tag)&IO_REPARSE_TAG_NAME_SURROGATE))

/*
 * The following constant represents the bits that are valid to use in
 * reparse tags.
 */
#define IO_REPARSE_TAG_VALID_VALUES	0xF000FFFF

/*
 * Macro to determine whether a reparse tag is a valid tag.
 */
#define IsReparseTagValid(_tag)						       \
	(!((_tag) & ~IO_REPARSE_TAG_VALID_VALUES) &&			       \
	 ((_tag) > IO_REPARSE_TAG_RESERVED_RANGE))

/* Microsoft tags for reparse points. */

enum IO_REPARSE_TAG {
	IO_REPARSE_TAG_SYMBOLIC_LINK	= cpu_to_le32(0),
	IO_REPARSE_TAG_NAME_SURROGATE	= cpu_to_le32(0x20000000),
	IO_REPARSE_TAG_MICROSOFT	= cpu_to_le32(0x80000000),
	IO_REPARSE_TAG_MOUNT_POINT	= cpu_to_le32(0xA0000003),
	IO_REPARSE_TAG_SYMLINK		= cpu_to_le32(0xA000000C),
	IO_REPARSE_TAG_HSM		= cpu_to_le32(0xC0000004),
	IO_REPARSE_TAG_SIS		= cpu_to_le32(0x80000007),
	IO_REPARSE_TAG_DEDUP		= cpu_to_le32(0x80000013),
	IO_REPARSE_TAG_COMPRESS		= cpu_to_le32(0x80000017),

	/*
	 * The reparse tag 0x80000008 is reserved for Microsoft internal use.
	 * May be published in the future.
	 */

	/* Microsoft reparse tag reserved for DFS */
	IO_REPARSE_TAG_DFS	= cpu_to_le32(0x8000000A),

	/* Microsoft reparse tag reserved for the file system filter manager. */
	IO_REPARSE_TAG_FILTER_MANAGER	= cpu_to_le32(0x8000000B),

	/* Non-Microsoft tags for reparse points */

	/* Tag allocated to CONGRUENT, May 2000. Used by IFSTEST. */
	IO_REPARSE_TAG_IFSTEST_CONGRUENT = cpu_to_le32(0x00000009),

	/* Tag allocated to ARKIVIO. */
	IO_REPARSE_TAG_ARKIVIO	= cpu_to_le32(0x0000000C),

	/* Tag allocated to SOLUTIONSOFT. */
	IO_REPARSE_TAG_SOLUTIONSOFT	= cpu_to_le32(0x2000000D),

	/* Tag allocated to COMMVAULT. */
	IO_REPARSE_TAG_COMMVAULT	= cpu_to_le32(0x0000000E),

	/* OneDrive?? */
	IO_REPARSE_TAG_CLOUD	= cpu_to_le32(0x9000001A),
	IO_REPARSE_TAG_CLOUD_1	= cpu_to_le32(0x9000101A),
	IO_REPARSE_TAG_CLOUD_2	= cpu_to_le32(0x9000201A),
	IO_REPARSE_TAG_CLOUD_3	= cpu_to_le32(0x9000301A),
	IO_REPARSE_TAG_CLOUD_4	= cpu_to_le32(0x9000401A),
	IO_REPARSE_TAG_CLOUD_5	= cpu_to_le32(0x9000501A),
	IO_REPARSE_TAG_CLOUD_6	= cpu_to_le32(0x9000601A),
	IO_REPARSE_TAG_CLOUD_7	= cpu_to_le32(0x9000701A),
	IO_REPARSE_TAG_CLOUD_8	= cpu_to_le32(0x9000801A),
	IO_REPARSE_TAG_CLOUD_9	= cpu_to_le32(0x9000901A),
	IO_REPARSE_TAG_CLOUD_A	= cpu_to_le32(0x9000A01A),
	IO_REPARSE_TAG_CLOUD_B	= cpu_to_le32(0x9000B01A),
	IO_REPARSE_TAG_CLOUD_C	= cpu_to_le32(0x9000C01A),
	IO_REPARSE_TAG_CLOUD_D	= cpu_to_le32(0x9000D01A),
	IO_REPARSE_TAG_CLOUD_E	= cpu_to_le32(0x9000E01A),
	IO_REPARSE_TAG_CLOUD_F	= cpu_to_le32(0x9000F01A),

};

#define SYMLINK_FLAG_RELATIVE		1

/* Microsoft reparse buffer. (see DDK for details) */
struct REPARSE_DATA_BUFFER {
	__le32 ReparseTag;		// 0x00:
	__le16 ReparseDataLength;	// 0x04:
	__le16 Reserved;

	union {
		/* If ReparseTag == 0xA0000003 (IO_REPARSE_TAG_MOUNT_POINT) */
		struct {
			__le16 SubstituteNameOffset; // 0x08
			__le16 SubstituteNameLength; // 0x0A
			__le16 PrintNameOffset;      // 0x0C
			__le16 PrintNameLength;      // 0x0E
			__le16 PathBuffer[];	     // 0x10
		} MountPointReparseBuffer;

		/*
		 * If ReparseTag == 0xA000000C (IO_REPARSE_TAG_SYMLINK)
		 * https://msdn.microsoft.com/en-us/library/cc232006.aspx
		 */
		struct {
			__le16 SubstituteNameOffset; // 0x08
			__le16 SubstituteNameLength; // 0x0A
			__le16 PrintNameOffset;      // 0x0C
			__le16 PrintNameLength;      // 0x0E
			// 0-absolute path 1- relative path, SYMLINK_FLAG_RELATIVE
			__le32 Flags;		     // 0x10
			__le16 PathBuffer[];	     // 0x14
		} SymbolicLinkReparseBuffer;

		/* If ReparseTag == 0x80000017U */
		struct NTFS_COMPRESS_REPARSE {
			__le32 WofVersion;  // 0x08 == 1
			/*
			 * 1 - WIM backing provider ("WIMBoot"),
			 * 2 - System compressed file provider
			 */
			__le32 WofProvider; // 0x0C:
			__le32 ProviderVer; // 0x10: == 1 WOF_FILE_PROVIDER_CURRENT_VERSION == 1
			__le32 CompressionFormat; // 0x14: 0, 1, 2, 3. See WOF_COMPRESSION_XXX
		} CompressReparseBuffer;

		struct {
			u8 DataBuffer[1];   // 0x08:
		} GenericReparseBuffer;
	};
};


#endif

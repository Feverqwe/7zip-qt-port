// SquashfsHandler.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"
#include "../../../C/CpuArch.h"
#include "../../../C/LzmaDec.h"
#include "../../../C/Xz.h"
#include "../../../C/ZstdDec.h"

#include "../../Common/ComTry.h"
#include "../../Common/IntToString.h"
#include "../../Common/MyLinux.h"
#include "../../Common/StringConvert.h"
#include "../../Common/UTFConvert.h"

#include "../../Windows/PropVariantUtils.h"
#include "../../Windows/TimeUtils.h"

#include "../Common/CWrappers.h"
#include "../Common/LimitedStreams.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamObjects.h"
#include "../Common/StreamUtils.h"

#include "../Compress/CopyCoder.h"
#include "../Compress/ZlibDecoder.h"
#ifdef SEVENZIP_LINUX_SQUASHFS
#include "../../../squashfsdecoder.h"
#include "../../../squashfsmetadata.h"
#include "archiveattributes.h"
#include <algorithm>
#include <errno.h>
#include <map>
#include <memory>
#include <set>
#endif
// #include "../Compress/LzmaDecoder.h"

namespace NArchive {
namespace NSquashfs {

static const UInt32 kNumFilesMax = 1 << 28;
static const unsigned kNumDirLevelsMax = 1 << 10;

// Layout: Header, Data, inodes, Directories, Fragments, UIDs, GIDs

/*
#define Get16(p) (be ? GetBe16(p) : GetUi16(p))
#define Get32(p) (be ? GetBe32(p) : GetUi32(p))
#define Get64(p) (be ? GetBe64(p) : GetUi64(p))
*/

static UInt16 Get16b(const Byte *p, bool be) { return be ? GetBe16(p) : GetUi16(p); }
static UInt32 Get32b(const Byte *p, bool be) { return be ? GetBe32(p) : GetUi32(p); }
static UInt64 Get64b(const Byte *p, bool be) { return be ? GetBe64(p) : GetUi64(p); }

#define Get16(p) Get16b(p, be)
#define Get32(p) Get32b(p, be)
#define Get64(p) Get64b(p, be)

#define LE_16(offs, dest) dest = GetUi16(p + (offs))
#define LE_32(offs, dest) dest = GetUi32(p + (offs))
#define LE_64(offs, dest) dest = GetUi64(p + (offs))

#define GET_16(offs, dest) dest = Get16(p + (offs))
#define GET_32(offs, dest) dest = Get32(p + (offs))
#define GET_64(offs, dest) dest = Get64(p + (offs))

static const UInt32 kSignature32_LE = 0x73717368;
static const UInt32 kSignature32_BE = 0x68737173;
static const UInt32 kSignature32_LZ = 0x71736873;
static const UInt32 kSignature32_B2 = 0x73687371;

#define kMethod_ZLIB 1
#define kMethod_LZMA 2
#define kMethod_LZO 3
#define kMethod_XZ 4
#define kMethod_LZ4 5
#define kMethod_ZSTD 6

static const char *const k_Methods[] = {"0", "ZLIB", "LZMA", "LZO", "XZ", "LZ4", "ZSTD"};

static const unsigned kMetadataBlockSizeLog = 13;
static const UInt32 kMetadataBlockSize = (1 << kMetadataBlockSizeLog);

enum { kType_IPC, kType_DIR, kType_FILE, kType_LNK, kType_BLK, kType_CHR, kType_FIFO, kType_SOCK };

static const UInt32 k_TypeToMode[] = {0,
                                      MY_LIN_S_IFDIR,
                                      MY_LIN_S_IFREG,
                                      MY_LIN_S_IFLNK,
                                      MY_LIN_S_IFBLK,
                                      MY_LIN_S_IFCHR,
                                      MY_LIN_S_IFIFO,
                                      MY_LIN_S_IFSOCK,
                                      MY_LIN_S_IFDIR,
                                      MY_LIN_S_IFREG,
                                      MY_LIN_S_IFLNK,
                                      MY_LIN_S_IFBLK,
                                      MY_LIN_S_IFCHR,
                                      MY_LIN_S_IFIFO,
                                      MY_LIN_S_IFSOCK};

enum {
    kFlag_UNC_INODES,
    kFlag_UNC_DATA,
    kFlag_CHECK,
    kFlag_UNC_FRAGS,
    kFlag_NO_FRAGS,
    kFlag_ALWAYS_FRAG,
    kFlag_DUPLICATE,
    kFlag_EXPORT
};

static const char *const k_Flags[] = {"UNCOMPRESSED_INODES",
                                      "UNCOMPRESSED_DATA",
                                      "CHECK",
                                      "UNCOMPRESSED_FRAGMENTS",
                                      "NO_FRAGMENTS",
                                      "ALWAYS_FRAGMENTS",
                                      "DUPLICATES_REMOVED",
                                      "EXPORTABLE",
                                      "UNCOMPRESSED_XATTRS",
                                      "NO_XATTRS",
                                      "COMPRESSOR_OPTIONS",
                                      "UNCOMPRESSED_IDS"};

static const UInt32 kNotCompressedBit16 = 1 << 15;
static const UInt32 kNotCompressedBit32 = 1 << 24;

#define GET_COMPRESSED_BLOCK_SIZE(size) ((size) & ~kNotCompressedBit32)
#define IS_COMPRESSED_BLOCK(size) (((size) & kNotCompressedBit32) == 0)

// static const UInt32 kHeaderSize1 = 0x33;
// static const UInt32 kHeaderSize2 = 0x3F;
static const UInt32 kHeaderSize3 = 0x77;
// static const UInt32 kHeaderSize4 = 0x60;

struct CHeader {
    bool be;
    bool SeveralMethods;
    Byte NumUids;
    Byte NumGids;

    UInt32 NumInodes;
    UInt32 CTime;
    UInt32 BlockSize;
    UInt32 NumFrags;
    UInt16 Method;
    UInt16 BlockSizeLog;
    UInt16 Flags;
    UInt16 NumIDs;
    UInt16 Major;
    UInt16 Minor;
    UInt64 RootInode;
    UInt64 Size;
    UInt64 UidTable;
    UInt64 GidTable;
    UInt64 XattrIdTable;
    UInt64 InodeTable;
    UInt64 DirTable;
    UInt64 FragTable;
    UInt64 LookupTable;

    void Parse3(const Byte *p) {
        Method = kMethod_ZLIB;
        GET_32(0x08, Size);
        GET_32(0x0C, UidTable);
        GET_32(0x10, GidTable);
        GET_32(0x14, InodeTable);
        GET_32(0x18, DirTable);
        GET_16(0x20, BlockSize);
        GET_16(0x22, BlockSizeLog);
        Flags = p[0x24];
        NumUids = p[0x25];
        NumGids = p[0x26];
        GET_32(0x27, CTime);
        GET_64(0x2B, RootInode);
        NumFrags = 0;
        FragTable = UidTable;

        if (Major >= 2) {
            GET_32(0x33, BlockSize);
            GET_32(0x37, NumFrags);
            GET_32(0x3B, FragTable);
            if (Major == 3) {
                GET_64(0x3F, Size);
                GET_64(0x47, UidTable);
                GET_64(0x4F, GidTable);
                GET_64(0x57, InodeTable);
                GET_64(0x5F, DirTable);
                GET_64(0x67, FragTable);
                GET_64(0x6F, LookupTable);
            }
        }
    }

    void Parse4(const Byte *p) {
        LE_32(0x08, CTime);
        LE_32(0x0C, BlockSize);
        LE_32(0x10, NumFrags);
        LE_16(0x14, Method);
        LE_16(0x16, BlockSizeLog);
        LE_16(0x18, Flags);
        LE_16(0x1A, NumIDs);
        LE_64(0x20, RootInode);
        LE_64(0x28, Size);
        LE_64(0x30, UidTable);
        LE_64(0x38, XattrIdTable);
        LE_64(0x40, InodeTable);
        LE_64(0x48, DirTable);
        LE_64(0x50, FragTable);
        LE_64(0x58, LookupTable);
        GidTable = 0;
    }

    bool Parse(const Byte *p) {
        be = false;
        SeveralMethods = false;
        switch (GetUi32(p)) {
        case kSignature32_LE:
            break;
        case kSignature32_BE:
            be = true;
            break;
        case kSignature32_LZ:
            SeveralMethods = true;
            break;
        case kSignature32_B2:
            SeveralMethods = true;
            be = true;
            break;
        default:
            return false;
        }
        GET_32(4, NumInodes);
        GET_16(0x1C, Major);
        GET_16(0x1E, Minor);
        if (Major <= 3)
            Parse3(p);
        else {
            if (be)
                return false;
            Parse4(p);
        }
        bool indexesInRange =
            (Major == 4 && NumFrags == 0 ? true : DirTable <= FragTable && FragTable <= Size) &&
            UidTable <= Size;
#ifdef SEVENZIP_LINUX_SQUASHFS
        if (Major == 4)
            indexesInRange = true; // Ancillary indexes have independent bounded Linux loaders.
#endif
        return InodeTable < DirTable && DirTable <= Size && indexesInRange && BlockSizeLog >= 12 &&
               BlockSizeLog < 31 && BlockSize == ((UInt32)1 << BlockSizeLog);
    }

    bool IsSupported() const {
        return Major > 0 && Major <= 4 && BlockSizeLog <= (Major == 4 ? 20 : 23) &&
               (Major != 4 || Minor == 0);
    }
    bool IsOldVersion() const { return Major < 4; }
    bool NeedCheckData() const { return Major < 4 && (Flags & (1 << kFlag_CHECK)) != 0; }
    unsigned GetFileNameOffset() const { return Major <= 2 ? 3 : (Major == 3 ? 5 : 8); }
    unsigned GetSymLinkOffset() const {
        return Major <= 1 ? 5 : (Major <= 2 ? 6 : (Major == 3 ? 18 : 24));
    }
    unsigned GetSpecGuidIndex() const { return Major <= 1 ? 0xF : 0xFF; }
};

static const UInt32 kFrag_Empty = (UInt32)(Int32)-1;
// static const UInt32 kXattr_Empty = (UInt32)(Int32)-1;

struct CNode {
    UInt16 Type;
    UInt16 Mode;
    UInt16 Uid;
    UInt16 Gid;
    UInt32 Frag;
    UInt32 Offset;
    // UInt32 MTime;
    UInt32 Number;
    UInt32 NumLinks;
    // UInt32 RDev;
    UInt32 Xattr;
    // UInt32 Parent;

    UInt64 FileSize;
    UInt64 StartBlock;
    // UInt64 Sparse;

    UInt32 Parse1(const Byte *p, UInt32 size, const CHeader &_h);
    UInt32 Parse2(const Byte *p, UInt32 size, const CHeader &_h);
    UInt32 Parse3(const Byte *p, UInt32 size, const CHeader &_h);
    UInt32 Parse4(const Byte *p, UInt32 size, const CHeader &_h, UInt32 *needed = NULL,
                  bool headerOnly = false);

    bool IsDir() const { return (Type == kType_DIR || Type == kType_DIR + 7); }
    bool IsLink() const { return (Type == kType_LNK || Type == kType_LNK + 7); }
    UInt64 GetSize() const { return IsDir() ? 0 : FileSize; }

    bool ThereAreFrags() const { return Frag != kFrag_Empty; }
    UInt64 GetNumBlocks(const CHeader &_h) const {
        return (FileSize >> _h.BlockSizeLog) +
               (!ThereAreFrags() && (FileSize & (_h.BlockSize - 1)) != 0);
    }
};

UInt32 CNode::Parse1(const Byte *p, UInt32 size, const CHeader &_h) {
    const bool be = _h.be;
    if (size < 4)
        return 0;
    {
        const UInt32 t = Get16(p);
        if (be) {
            Type = (UInt16)(t >> 12);
            Mode = (UInt16)(t & 0xFFF);
            Uid = (UInt16)(p[2] >> 4);
            Gid = (UInt16)(p[2] & 0xF);
        } else {
            Type = (UInt16)(t & 0xF);
            Mode = (UInt16)(t >> 4);
            Uid = (UInt16)(p[2] & 0xF);
            Gid = (UInt16)(p[2] >> 4);
        }
    }

    // Xattr = kXattr_Empty;
    // MTime = 0;
    FileSize = 0;
    StartBlock = 0;
    Frag = kFrag_Empty;

    if (Type == 0) {
        Byte t = p[3];
        if (be) {
            Type = (UInt16)(t >> 4);
            Offset = (UInt16)(t & 0xF);
        } else {
            Type = (UInt16)(t & 0xF);
            Offset = (UInt16)(t >> 4);
        }
        return (Type == kType_FIFO || Type == kType_SOCK) ? 4 : 0;
    }

    Type--;
    Uid = (UInt16)(Uid + (Type / 5) * 16);
    Type = (UInt16)((Type % 5) + 1);

    if (Type == kType_FILE) {
        if (size < 15)
            return 0;
        // GET_32 (3, MTime);
        GET_32(7, StartBlock);
        UInt32 t;
        GET_32(11, t);
        FileSize = t;
        UInt32 numBlocks = t >> _h.BlockSizeLog;
        if ((t & (_h.BlockSize - 1)) != 0)
            numBlocks++;
        UInt32 pos = numBlocks * 2 + 15;
        return (pos <= size) ? pos : 0;
    }

    if (Type == kType_DIR) {
        if (size < 14)
            return 0;
        UInt32 t = Get32(p + 3);
        if (be) {
            FileSize = t >> 13;
            Offset = t & 0x1FFF;
        } else {
            FileSize = t & 0x7FFFF;
            Offset = t >> 19;
        }
        // GET_32 (7, MTime);
        GET_32(10, StartBlock);
        if (be)
            StartBlock &= 0xFFFFFF;
        else
            StartBlock >>= 8;
        return 14;
    }

    if (size < 5)
        return 0;

    if (Type == kType_LNK) {
        UInt32 len;
        GET_16(3, len);
        FileSize = len;
        len += 5;
        return (len <= size) ? len : 0;
    }

    // GET_32 (3, RDev);
    return 5;
}

UInt32 CNode::Parse2(const Byte *p, UInt32 size, const CHeader &_h) {
    const bool be = _h.be;
    if (size < 4)
        return 0;
    {
        const UInt32 t = Get16(p);
        if (be) {
            Type = (UInt16)(t >> 12);
            Mode = (UInt16)(t & 0xFFF);
        } else {
            Type = (UInt16)(t & 0xF);
            Mode = (UInt16)(t >> 4);
        }
    }

    Uid = p[2];
    Gid = p[3];

    // Xattr = kXattr_Empty;

    if (Type == kType_FILE) {
        if (size < 24)
            return 0;
        // GET_32 (4, MTime);
        GET_32(8, StartBlock);
        GET_32(12, Frag);
        GET_32(16, Offset);
        UInt32 t;
        GET_32(20, t);
        FileSize = t;
        UInt32 numBlocks = t >> _h.BlockSizeLog;
        if (!ThereAreFrags() && (t & (_h.BlockSize - 1)) != 0)
            numBlocks++;
        UInt32 pos = numBlocks * 4 + 24;
        return (pos <= size) ? (UInt32)pos : 0;
    }

    FileSize = 0;
    // MTime = 0;
    StartBlock = 0;
    Frag = kFrag_Empty;

    if (Type == kType_DIR) {
        if (size < 15)
            return 0;
        UInt32 t = Get32(p + 4);
        if (be) {
            FileSize = t >> 13;
            Offset = t & 0x1FFF;
        } else {
            FileSize = t & 0x7FFFF;
            Offset = t >> 19;
        }
        // GET_32 (8, MTime);
        GET_32(11, StartBlock);
        if (be)
            StartBlock &= 0xFFFFFF;
        else
            StartBlock >>= 8;
        return 15;
    }

    if (Type == kType_DIR + 7) {
        if (size < 18)
            return 0;
        UInt32 t = Get32(p + 4);
        UInt32 t2 = Get16(p + 7);
        if (be) {
            FileSize = t >> 5;
            Offset = t2 & 0x1FFF;
        } else {
            FileSize = t & 0x7FFFFFF;
            Offset = t2 >> 3;
        }
        // GET_32 (9, MTime);
        GET_32(12, StartBlock);
        if (be)
            StartBlock &= 0xFFFFFF;
        else
            StartBlock >>= 8;
        UInt32 iCount;
        GET_16(16, iCount);
        UInt32 pos = 18;
        for (UInt32 i = 0; i < iCount; i++) {
            // 27 bits: index
            // 29 bits: startBlock
            if (pos + 8 > size)
                return 0;
            pos += 8 + (UInt32)p[pos + 7] + 1; // nameSize
            if (pos > size)
                return 0;
        }
        return pos;
    }

    if (Type == kType_FIFO || Type == kType_SOCK)
        return 4;

    if (size < 6)
        return 0;

    if (Type == kType_LNK) {
        UInt32 len;
        GET_16(4, len);
        FileSize = len;
        len += 6;
        return (len <= size) ? len : 0;
    }

    if (Type == kType_BLK || Type == kType_CHR) {
        // GET_16 (4, RDev);
        return 6;
    }

    return 0;
}

UInt32 CNode::Parse3(const Byte *p, UInt32 size, const CHeader &_h) {
    const bool be = _h.be;
    if (size < 12)
        return 0;

    {
        const UInt32 t = Get16(p);
        if (be) {
            Type = (UInt16)(t >> 12);
            Mode = (UInt16)(t & 0xFFF);
        } else {
            Type = (UInt16)(t & 0xF);
            Mode = (UInt16)(t >> 4);
        }
    }

    Uid = p[2];
    Gid = p[3];
    // GET_32 (4, MTime);
    // GET_32 (8, Number);
    // Xattr = kXattr_Empty;
    FileSize = 0;
    StartBlock = 0;

    if (Type == kType_FILE || Type == kType_FILE + 7) {
        UInt32 offset;
        if (Type == kType_FILE) {
            if (size < 32)
                return 0;
            GET_64(12, StartBlock);
            GET_32(20, Frag);
            GET_32(24, Offset);
            GET_32(28, FileSize);
            offset = 32;
        } else {
            if (size < 40)
                return 0;
            // GET_32 (12, NumLinks);
            GET_64(16, StartBlock);
            GET_32(24, Frag);
            GET_32(28, Offset);
            GET_64(32, FileSize);
            offset = 40;
        }
        UInt64 pos = GetNumBlocks(_h) * 4 + offset;
        return (pos <= size) ? (UInt32)pos : 0;
    }

    if (size < 16)
        return 0;
    // GET_32 (12, NumLinks);

    if (Type == kType_DIR) {
        if (size < 28)
            return 0;
        UInt32 t = Get32(p + 16);
        if (be) {
            FileSize = t >> 13;
            Offset = t & 0x1FFF;
        } else {
            FileSize = t & 0x7FFFF;
            Offset = t >> 19;
        }
        GET_32(20, StartBlock);
        // GET_32 (24, Parent);
        return 28;
    }

    if (Type == kType_DIR + 7) {
        if (size < 31)
            return 0;
        UInt32 t = Get32(p + 16);
        UInt32 t2 = Get16(p + 19);
        if (be) {
            FileSize = t >> 5;
            Offset = t2 & 0x1FFF;
        } else {
            FileSize = t & 0x7FFFFFF;
            Offset = t2 >> 3;
        }
        GET_32(21, StartBlock);
        UInt32 iCount;
        GET_16(25, iCount);
        // GET_32 (27, Parent);
        UInt32 pos = 31;
        for (UInt32 i = 0; i < iCount; i++) {
            // UInt32 index
            // UInt32 startBlock
            if (pos + 9 > size)
                return 0;
            pos += 9 + (unsigned)p[pos + 8] + 1; // nameSize
            if (pos > size)
                return 0;
        }
        return pos;
    }

    if (Type == kType_FIFO || Type == kType_SOCK)
        return 16;

    if (size < 18)
        return 0;
    if (Type == kType_LNK) {
        UInt32 len;
        GET_16(16, len);
        FileSize = len;
        len += 18;
        return (len <= size) ? len : 0;
    }

    if (Type == kType_BLK || Type == kType_CHR) {
        // GET_16 (16, RDev);
        return 18;
    }

    return 0;
}

UInt32 CNode::Parse4(const Byte *p, UInt32 size, const CHeader &_h, UInt32 *needed,
                     bool headerOnly) {
    if (needed)
        *needed = 0;
    const auto available = [&](UInt64 required) {
        if (required <= size)
            return true;
        if (needed && required <= UINT32_MAX)
            *needed = (UInt32)required;
        return false;
    };
    if (!available(20))
        return 0;
    LE_16(0, Type);
    LE_16(2, Mode);
    LE_16(4, Uid);
    LE_16(6, Gid);
    // LE_32 (8, MTime);
    LE_32(12, Number);
    if ((Mode & MY_LIN_S_IFMT) != 0 || GetUi32(p + 12) == 0)
        return 0;
#ifndef SEVENZIP_LINUX_SQUASHFS
    if (Uid >= _h.NumIDs || Gid >= _h.NumIDs)
        return 0;
#endif

    Xattr = (UInt32)-1;
    NumLinks = 1;
    FileSize = 0;
    StartBlock = 0;

    if (Type == kType_FILE || Type == kType_FILE + 7) {
        UInt32 offset;
        if (Type == kType_FILE) {
            if (!available(32))
                return 0;
            LE_32(16, StartBlock);
            LE_32(20, Frag);
            LE_32(24, Offset);
            LE_32(28, FileSize);
            offset = 32;
        } else {
            if (!available(56))
                return 0;
            LE_64(16, StartBlock);
            LE_64(24, FileSize);
            // LE_64 (32, Sparse);
            LE_32(40, NumLinks);
            LE_32(44, Frag);
            LE_32(48, Offset);
            LE_32(52, Xattr);
            offset = 56;
        }
        if ((Int64)FileSize < 0 ||
            (ThereAreFrags() && ((FileSize & (_h.BlockSize - 1)) == 0 || Offset > _h.BlockSize ||
                                 (FileSize & (_h.BlockSize - 1)) > _h.BlockSize - Offset)))
            return 0;
        UInt64 pos = GetNumBlocks(_h) * 4 + offset;
        if (headerOnly)
            return offset;
        return available(pos) ? (UInt32)pos : 0;
    }

    if (Type == kType_DIR) {
        if (!available(32))
            return 0;
        LE_32(16, StartBlock);
        LE_32(20, NumLinks);
        LE_16(24, FileSize);
        LE_16(26, Offset);
        // LE_32 (28, Parent);
        return 32;
    }

    LE_32(16, NumLinks);

    if (Type == kType_DIR + 7) {
        if (!available(40))
            return 0;
        LE_32(20, FileSize);
        LE_32(24, StartBlock);
        // LE_32 (28, Parent);
        UInt32 iCount;
        LE_16(32, iCount);
        LE_16(34, Offset);
        LE_32(36, Xattr);

        if (headerOnly)
            return 40;
        UInt32 pos = 40;
        for (UInt32 i = 0; i < iCount; i++) {
            // UInt32 index
            // UInt32 startBlock
            if (!available(pos + 12))
                return 0;
            UInt32 nameLen = GetUi32(p + pos + 8);
            if (nameLen >= 256)
                return 0;
            pos += 12 + nameLen + 1;
            if (!available(pos))
                return 0;
        }
        return pos;
    }

    unsigned offset = 20;
    switch (Type) {
    case kType_FIFO:
    case kType_FIFO + 7:
    case kType_SOCK:
    case kType_SOCK + 7:
        break;
    case kType_LNK:
    case kType_LNK + 7: {
        if (!available(24))
            return 0;
        UInt32 len;
        LE_32(20, len);
        FileSize = len;
        if (len > (1 << 16))
            return 0;
        offset = len + 24;
        if (!available(offset))
            return 0;
        break;
    }
    case kType_BLK:
    case kType_BLK + 7:
    case kType_CHR:
    case kType_CHR + 7:
        if (!available(24))
            return 0;
        // LE_32 (20, RDev);
        offset = 24;
        break;
    default:
        return 0;
    }

    if (Type >= 8) {
        if (!available(offset + 4))
            return 0;
        LE_32(offset, Xattr);
        offset += 4;
    }
    return offset;
}

struct CItem {
    int Node;
    int Parent;
    UInt32 Ptr;
#ifdef SEVENZIP_LINUX_SQUASHFS
    int Attribute;
    UInt32 AttributeSuffix;
    UInt16 UnavailableType;
    bool RecoveredInode;
#endif

    void Construct() {
        Node = -1;
        Parent = -1;
        Ptr = 0;
#ifdef SEVENZIP_LINUX_SQUASHFS
        Attribute = -1;
        AttributeSuffix = 0;
        UnavailableType = 0;
        RecoveredInode = false;
#endif
    }
};

struct CData {
    CByteBuffer Data;
    size_t ValidSize = 0;
    bool HasValidSize = false;
    CRecordVector<UInt64> PackPos;
    CRecordVector<UInt64> PackEnd;
    CRecordVector<UInt32> UnpackPos;
    CRecordVector<UInt32> UnpackEnd;

    UInt32 ContiguousEnd(unsigned block) const {
        return UnpackEnd.IsEmpty() ? (UInt32)Size() : UnpackEnd[block];
    }
    size_t Size() const { return HasValidSize ? ValidSize : Data.Size(); }
    HRESULT Append(const Byte *bytes, size_t length) {
        const size_t before = Size();
        if (before >= UINT32_MAX || length >= UINT32_MAX - before)
            return S_FALSE;
        const size_t end = before + length;
        if (end > Data.Size()) {
            const UInt64 growth = (UInt64)Data.Size() + Data.Size() / 2 + kMetadataBlockSize;
            const size_t capacity = (size_t)MyMax((UInt64)end, MyMin(growth, (UInt64)UINT32_MAX));
            Data.ChangeSize_KeepData(capacity, before);
        }
        if (length)
            memcpy(Data + before, bytes, length);
        ValidSize = end;
        HasValidSize = true;
        return S_OK;
    }

    UInt32 GetNumBlocks() const { return PackPos.Size(); }
    void Clear() {
        Data.Free();
        ValidSize = 0;
        HasValidSize = false;
        PackPos.Clear();
        PackEnd.Clear();
        UnpackPos.Clear();
        UnpackEnd.Clear();
    }
};

struct CFrag {
    UInt64 StartBlock;
    UInt32 Size;
#ifdef SEVENZIP_LINUX_SQUASHFS
    bool Available = true;
#endif
};

// clang-format off
struct CTempItem;
#ifdef SEVENZIP_LINUX_SQUASHFS
Z7_CLASS_IMP_CHandler_IInArchive_2(
  IInArchiveGetStream,
  IArchiveGetRawProps
)
#else
Z7_CLASS_IMP_CHandler_IInArchive_1(
  IInArchiveGetStream
)
#endif
  bool _noPropsLZMA;
  bool _needCheckLzma;

  CRecordVector<CItem> _items;
#ifdef SEVENZIP_LINUX_SQUASHFS
  CObjectVector<AString> _itemNames;
#endif
  CRecordVector<CNode> _nodes;
  CRecordVector<UInt32> _nodesPos;
  CRecordVector<bool> _activeDirs;
  CData _inodesData;
  CData _dirs;
  CRecordVector<CFrag> _frags;
  CByteBuffer _uids;
  CByteBuffer _gids;
  CHeader _h;

  UInt64 _sizeCalculated;
  CMyComPtr<IInStream> _stream;

  IArchiveOpenCallback *_openCallback;
  HRESULT _directoryCallbackResult = S_OK;

  UInt32 _openCodePage;
  int _nodeIndex;
  CRecordVector<bool> _blockCompressed;
  CRecordVector<UInt64> _blockOffsets;

  CByteBuffer _cachedBlock;
  UInt64 _cachedBlockStartPos;
  UInt64 _processedPackSize = 0;
  UInt32 _cachedPackBlockSize;
  UInt32 _cachedUnpackBlockSize;

  CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> _limitedInStream;
  CMyComPtr2_Create<ISequentialOutStream, CBufPtrSeqOutStream> _outStream;
  CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> _dynOutStream;

  // CMyComPtr2<ICompressCoder, NCompress::NLzma::CDecoder> _lzmaDecoder;
  CMyComPtr2<ICompressCoder, NCompress::NZlib::CDecoder> _zlibDecoder;

  CXzUnpacker _xz;
  CZstdDecHandle _zstd;

  CByteBuffer _inputBuffer;
#ifdef SEVENZIP_LINUX_SQUASHFS
  sevenzip_squashfs_decoder *_linuxDecoder = NULL;
  sevenzip_squashfs_metadata *_linuxMetadata = NULL;
  sevenzip_squashfs_file *_linuxFile = NULL;
  CRecordVector<UInt64> _nodeReferences;
  CRecordVector<UInt64> _nodeRecordSizes;
  std::set<unsigned> _incompleteBlocklists;
  HRESULT CreateLinuxFile(unsigned node, sevenzip_squashfs_file **file);
  HRESULT ReadLinuxFileBlock(sevenzip_squashfs_file *file, UInt64 index,
                             uint64_t *start, unsigned *size);
  HRESULT GetLinuxPackSize(unsigned node, UInt64 &total);
  HRESULT _linuxReadResult = S_OK;
  UInt64 _metadataReadEnd = 0;
  static int ReadLinux(void *opaque, uint64_t offset, void *data, size_t size);
  HRESULT InitLinuxDecoder();
  static int ReadLinuxMetadata(void *, uint64_t, void *, unsigned *, uint64_t *);
  HRESULT LoadLinuxTables(UInt64 &directoryEnd);
  HRESULT LoadFragment(CNode &node);
  HRESULT LoadId(UInt32 index);
  HRESULT LoadNode(unsigned index);
  CRecordVector<bool> _loadedNodes;
  std::set<UInt32> _loadedIds;
  std::map<UInt32, UInt32> _fragmentIndexes;
  HRESULT LinuxResult(int result) const;
  using CAttributes = std::shared_ptr<std::vector<ArchiveAttribute>>;
  std::vector<CAttributes> _attributes;
  std::map<UInt32, CAttributes> _attributeSets;
  std::set<UInt32> _incompleteAttributes;
  std::set<UInt32> _unavailableIds;
  std::set<UInt32> _incompleteDirectories;
  std::map<UInt32, std::pair<UInt64, UInt16>> _inodeReferences;
  std::map<UInt32, std::pair<unsigned, UInt64>> _inodePositions;
  std::map<UInt64, std::pair<unsigned, UInt64>> _inodeNodes;
  std::map<UInt64, unsigned> _inodeBlockIndexes;
  std::set<UInt32> _unavailableInodeBlocks;
  std::set<UInt32> _badInodePositions;
  bool _inodeTableIncomplete = false;
  bool _inodeInventoryScanned = false;
  HRESULT ScanCachedInodes(bool forRecovery = false);
  HRESULT EnsureInodeInventory();
  UInt64 _directoryDataEnd = 0;
  HRESULT ReadNode(UInt32 position, int &nodeIndex, UInt64 *length = NULL, UInt32 *needed = NULL);
  bool _inodeMappingLimitations = false;
  bool InodeIdentityUnavailable(const CItem &item) const {
    if (item.RecoveredInode)
      return true;
    if (item.Node < 0)
      return false;
    const auto found = _inodeReferences.find(_nodes[item.Node].Number);
    return found != _inodeReferences.end() && found->second.first == UINT64_MAX;
  }
  bool _ownershipLimitations = false;
  bool _fragmentLimitations = false;
  bool _indexLimitations = false;
  bool OwnershipUnavailable(const CNode &node) const {
    return !_h.IsOldVersion() && (node.Uid >= _h.NumIDs || node.Gid >= _h.NumIDs ||
        _unavailableIds.count(node.Uid) || _unavailableIds.count(node.Gid));
  }
  bool FragmentUnavailable(const CNode &node) const {
    return !_h.IsOldVersion() && (node.Type == kType_FILE || node.Type == kType_FILE + 7) && node.ThereAreFrags() &&
        (node.Frag >= _frags.Size() || !_frags[node.Frag].Available);
  }
  AString _rootAttributeName;
  struct CAttributeContext { CHandler *Owner; unsigned Node; HRESULT Result; };
  static int EmitAttribute(void *, const char *, const void *, size_t) noexcept;
  HRESULT AddAttributeItems(int root);
#endif
  UInt64 GetItemSize(unsigned index) const;
  HRESULT FindNode(UInt32 block, UInt32 offset, int &nodeIndex);
  HRESULT ReadDirItems(int parent, const CNode &node, CRecordVector<CTempItem> &items,
                       HRESULT &callbackResult);
  bool ItemIsDir(unsigned index) const;

  void ClearCache()
  {
    _cachedBlockStartPos = 0;
    _cachedPackBlockSize = 0;
    _cachedUnpackBlockSize = 0;
  }

  HRESULT Seek2(UInt64 offset)
  {
    if (offset > _h.Size)
      return S_FALSE;
    return InStream_SeekSet(_stream, offset);
  }

  HRESULT Decompress(ISequentialOutStream *outStream, Byte *outBuf, bool *outBufWasWritten, UInt32 *outBufWasWrittenSize,
      UInt32 inSize, UInt32 outSizeMax);
  HRESULT ReadMetadataBlock(UInt32 &packSize, bool *boundaryKnown = NULL,
                            CDynBufSeqOutStream *output = NULL);
  HRESULT ReadMetadataBlock2();
  HRESULT ReadData(CData &data, UInt64 start, UInt64 end);
#ifdef SEVENZIP_LINUX_SQUASHFS
  HRESULT ReadDirectoryData(UInt64 end);
  HRESULT ReadMetadataRun(CData &data, std::map<UInt64, unsigned> &indexes,
                          UInt64 start, UInt64 end, UInt64 &position, UInt64 &next, UInt64 needed);
#endif

  HRESULT OpenDir(int parent, UInt32 startBlock, UInt32 offset, unsigned level, int &nodeIndex);
  HRESULT ScanInodes(UInt64 ptr);
  HRESULT ReadUids(UInt64 start, UInt32 num, CByteBuffer &ids);
  HRESULT Open2(IInStream *inStream);
  AString GetPath(unsigned index) const;
  const Byte *GetItemName(unsigned index, unsigned &size) const;
  bool GetPackSize(unsigned index, UInt64 &res, bool fillOffsets);
  HRESULT CreateReadStream(UInt32 index, ISequentialInStream **stream, CHandler **reader = NULL);

public:
  CHandler();
  ~CHandler()
  {
#ifdef SEVENZIP_LINUX_SQUASHFS
    sevenzip_squashfs_decoder_destroy(_linuxDecoder);
    sevenzip_squashfs_metadata_destroy(_linuxMetadata);
    sevenzip_squashfs_file_destroy(_linuxFile);
#endif
    XzUnpacker_Free(&_xz);
    if (_zstd)
      ZstdDec_Destroy(_zstd);
  }

  HRESULT ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize);
};
// clang-format on

CHandler::CHandler() : _zstd(NULL) { XzUnpacker_Construct(&_xz, &g_Alloc); }

#ifdef SEVENZIP_LINUX_SQUASHFS
int CHandler::ReadLinux(void *opaque, uint64_t offset, void *data, size_t size) {
    CHandler *handler = static_cast<CHandler *>(opaque);
    if (offset > handler->_h.Size || size > handler->_h.Size - offset) {
        handler->_linuxReadResult = S_FALSE;
        return -1;
    }
    try {
        HRESULT result = handler->Seek2(offset);
        if (result == S_OK)
            result = ReadStream_FALSE(handler->_stream, data, size);
        handler->_linuxReadResult = result;
        return result == S_OK            ? 0
               : result == E_ABORT       ? -ECANCELED
               : result == E_OUTOFMEMORY ? -ENOMEM
               : result == E_NOTIMPL     ? -95
                                         : -EIO;
    } catch (...) {
        // Match the upstream COM boundary without unwinding through C-owned
        // option buffers and decoder setup allocations.
        handler->_linuxReadResult = E_OUTOFMEMORY;
        return -ENOMEM;
    }
}

HRESULT CHandler::InitLinuxDecoder() {
    if (_h.IsOldVersion())
        return S_OK;
    _linuxReadResult = S_OK;
    const int result = sevenzip_squashfs_decoder_create(_h.Method, _h.BlockSize, _h.Flags, _h.Size,
                                                        ReadLinux, this, &_linuxDecoder);
    if (_linuxReadResult != S_OK)
        return _linuxReadResult;
    return result == 0 ? S_OK : result == -12 ? E_OUTOFMEMORY : result == -95 ? E_NOTIMPL : S_FALSE;
}

HRESULT CHandler::LinuxResult(int result) const {
    if (result == -ENOMEM)
        return E_OUTOFMEMORY;
    if (result == -ECANCELED)
        return E_ABORT;
    if (result == -95)
        return E_NOTIMPL;
    if (_linuxReadResult != S_OK)
        return _linuxReadResult;
    return result >= 0 ? S_OK : S_FALSE;
}

int CHandler::ReadLinuxMetadata(void *opaque, uint64_t block, void *data, unsigned *size,
                                uint64_t *next) {
    CHandler *handler = static_cast<CHandler *>(opaque);
    const UInt64 end = handler->_metadataReadEnd
                           ? MyMin(handler->_metadataReadEnd, handler->_h.Size)
                           : handler->_h.Size;
    if (block > end) {
        handler->_linuxReadResult = S_FALSE;
        return -5;
    }
    try {
        HRESULT result = handler->Seek2(block);
        UInt32 packed = (UInt32)MyMin(end - block, (UInt64)kMetadataBlockSize + 3);
        handler->_dynOutStream->Init();
        if (result == S_OK)
            result = handler->ReadMetadataBlock(packed);
        if (result == S_OK) {
            const size_t actual = handler->_dynOutStream->GetSize();
            if (actual > *size)
                result = S_FALSE;
            else {
                memcpy(data, handler->_dynOutStream->GetBuffer(), actual);
                *size = (unsigned)actual;
                *next = block + packed;
            }
        }
        handler->_linuxReadResult = result;
        return result == S_OK            ? 0
               : result == E_ABORT       ? -ECANCELED
               : result == E_OUTOFMEMORY ? -ENOMEM
               : result == E_NOTIMPL     ? -95
                                         : -EIO;
    } catch (...) {
        handler->_linuxReadResult = E_OUTOFMEMORY;
        return -12;
    }
}

HRESULT CHandler::LoadLinuxTables(UInt64 &directoryEnd) {
    _linuxReadResult = S_OK;
    uint64_t end;
    const int created = sevenzip_squashfs_metadata_create(_h.Size, ReadLinux, ReadLinuxMetadata,
                                                          this, &_linuxMetadata, &end);
    if (created != 0)
        return LinuxResult(created);
    // Ancillary I/O failures have been retained as unavailable tables by the
    // C adapter. A successful subset must not inherit its last tolerated read.
    _linuxReadResult = S_OK;
    const unsigned unavailable = sevenzip_squashfs_unavailable_tables(_linuxMetadata);
    _indexLimitations = unavailable != 0;
    directoryEnd = end;
    _uids.Alloc((size_t)_h.NumIDs * 4);
    return S_OK;
}

HRESULT CHandler::LoadId(UInt32 index) {
    if (index >= _h.NumIDs || _loadedIds.count(index))
        return S_OK;
    _linuxReadResult = S_OK;
    unsigned id = 0;
    const HRESULT result = LinuxResult(sevenzip_squashfs_id(_linuxMetadata, index, &id));
    if (result == E_ABORT || result == E_OUTOFMEMORY || result == E_NOTIMPL)
        return result;
    if (_openCallback) {
        _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
        RINOK(_directoryCallbackResult)
    }
    if (result != S_OK)
        _unavailableIds.insert(index);
    SetUi32(_uids + (size_t)index * 4, id);
    _loadedIds.insert(index);
    return S_OK;
}

HRESULT CHandler::LoadFragment(CNode &node) {
    if ((node.Type != kType_FILE && node.Type != kType_FILE + 7) || !node.ThereAreFrags())
        return S_OK;
    const auto found = _fragmentIndexes.find(node.Frag);
    if (found != _fragmentIndexes.end()) {
        node.Frag = found->second;
        return S_OK;
    }
    _linuxReadResult = S_OK;
    uint64_t start = 0;
    unsigned size = 0;
    const HRESULT result =
        LinuxResult(sevenzip_squashfs_fragment(_linuxMetadata, node.Frag, &start, &size));
    if (result == E_ABORT || result == E_OUTOFMEMORY || result == E_NOTIMPL)
        return result;
    if (_openCallback) {
        _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
        RINOK(_directoryCallbackResult)
    }
    CFrag frag;
    const UInt32 packed = GET_COMPRESSED_BLOCK_SIZE(size);
    frag.Available = result == S_OK && packed != 0 && packed <= _h.BlockSize && start <= _h.Size &&
                     packed <= _h.Size - start;
    frag.StartBlock = start;
    frag.Size = size;
    const UInt32 index = _frags.Size();
    _fragmentIndexes.emplace(node.Frag, index);
    _frags.Add(frag);
    node.Frag = index;
    return S_OK;
}

HRESULT CHandler::ReadNode(UInt32 pos, int &nodeIndex, UInt64 *length, UInt32 *needed) {
    nodeIndex = -1;
    if (needed)
        *needed = 0;
    const UInt32 totalSize = (UInt32)_inodesData.Size();
    if (pos >= totalSize)
        return S_FALSE;
    if (_badInodePositions.count(pos))
        return S_FALSE;
    const UInt32 *starts = &_inodesData.UnpackPos[0];
    const unsigned block =
        (unsigned)(std::upper_bound(starts, starts + _inodesData.UnpackPos.Size(), pos) - starts -
                   1);
    const auto cachedNode = [&](unsigned index, UInt64 size) {
        if (length && _nodes[index].Type == kType_DIR + 7) {
            CNode complete;
            size =
                complete.Parse4(_inodesData.Data + pos, _inodesData.ContiguousEnd(block) - pos, _h);
            if (!size) {
                _inodeTableIncomplete = true;
                return S_FALSE;
            }
        }
        nodeIndex = (int)index;
        if (length)
            *length = size;
        return S_OK;
    };
    auto next = _inodePositions.lower_bound(pos);
    if (next != _inodePositions.end() && next->first == pos) {
        return cachedNode(next->second.first, _nodeRecordSizes[next->second.first]);
    }
    if (next != _inodePositions.begin()) {
        auto previous = next;
        --previous;
        if (!_inodeTableIncomplete && pos < previous->second.second) {
            _inodeTableIncomplete = true;
        }
    }
    const UInt64 reference = ((UInt64)_inodesData.PackPos[block] << 16) + pos - starts[block];
    const auto existing = _inodeNodes.find(reference);
    if (existing != _inodeNodes.end()) {
        return cachedNode(existing->second.first, existing->second.second);
    }
    CNode n;
    UInt32 required = 0;
    const UInt32 parsed = n.Parse4(_inodesData.Data + pos, _inodesData.ContiguousEnd(block) - pos,
                                   _h, &required, needed != NULL);
    UInt64 size = parsed;
    if (parsed && needed && (n.Type == kType_FILE || n.Type == kType_FILE + 7))
        size += n.GetNumBlocks(_h) * 4;
    if (!size && required) {
        if (needed)
            *needed = required;
        return S_FALSE;
    }
    if (!size || _nodes.Size() >= kNumFilesMax) {
        _inodeTableIncomplete = true;
        // A full-inventory failure may concern an optional suffix rather
        // than the header used by a canonical reference.
        if (needed)
            _badInodePositions.insert(pos);
        return S_FALSE;
    }
    if (next != _inodePositions.end() && parsed > next->first - pos)
        _inodeTableIncomplete = true;
    if (_openCallback && (_nodes.Size() & 255) == 0) {
        _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
        RINOK(_directoryCallbackResult)
    }
    const UInt16 type = n.Type > kType_SOCK ? n.Type - 7 : n.Type;
    const auto inserted = _inodeReferences.emplace(n.Number, std::make_pair(reference, type));
    if (!inserted.second) {
        inserted.first->second.first = UINT64_MAX;
        _inodeMappingLimitations = true;
    }
    const unsigned index = _nodes.Size();
    _attributes.resize(index + 1);
    _loadedNodes.Add(false);
    _nodesPos.Add(pos);
    _nodeReferences.Add(reference);
    _nodeRecordSizes.Add(size);
    _nodes.Add(n);
    _activeDirs.Add(false);
    _inodePositions.emplace(pos, std::make_pair(index, (UInt64)pos + parsed));
    _inodeNodes.emplace(reference, std::make_pair(index, size));
    nodeIndex = (int)index;
    if (length)
        *length = size;
    return S_OK;
}

HRESULT CHandler::LoadNode(unsigned index) {
    if (_loadedNodes[index])
        return S_OK;
    CNode &n = _nodes[index];
    RINOK(LoadId(n.Uid))
    RINOK(LoadId(n.Gid))
    RINOK(LoadFragment(n))
    _ownershipLimitations |= OwnershipUnavailable(n);
    _fragmentLimitations |= FragmentUnavailable(n);
    _linuxReadResult = S_OK;
    auto found = _attributeSets.find(n.Xattr);
    if (found == _attributeSets.end()) {
        _attributes[index] = std::make_shared<std::vector<ArchiveAttribute>>();
        CAttributeContext context{this, index, S_OK};
        const int result =
            sevenzip_squashfs_visit_xattrs(_linuxMetadata, n.Xattr, EmitAttribute, &context);
        RINOK(context.Result)
        const HRESULT attributeResult = LinuxResult(result);
        if (attributeResult == E_ABORT || attributeResult == E_OUTOFMEMORY)
            return attributeResult;
        if (attributeResult != S_OK)
            _incompleteAttributes.insert(n.Xattr);
        _attributeSets.emplace(n.Xattr, _attributes[index]);
    } else
        _attributes[index] = found->second;
    _loadedNodes[index] = true;
    return S_OK;
}

HRESULT CHandler::CreateLinuxFile(unsigned index, sevenzip_squashfs_file **file) {
    const CNode &node = _nodes[index];
    _linuxReadResult = S_OK;
    const UInt64 savedEnd = _metadataReadEnd;
    _metadataReadEnd = _h.DirTable;
    const int result = sevenzip_squashfs_file_create(
        _h.Size, _h.InodeTable, _h.DirTable, _h.BlockSize, _nodeReferences[index],
        node.Type == kType_FILE ? 32 : 56, node.FileSize, node.GetNumBlocks(_h), node.StartBlock,
        node.Number, ReadLinuxMetadata, this, file);
    _metadataReadEnd = savedEnd;
    return LinuxResult(result);
}

HRESULT CHandler::ReadLinuxFileBlock(sevenzip_squashfs_file *file, UInt64 index, uint64_t *start,
                                     unsigned *size) {
    _linuxReadResult = S_OK;
    const UInt64 savedEnd = _metadataReadEnd;
    _metadataReadEnd = _h.DirTable;
    const int result = sevenzip_squashfs_file_block(file, index, start, size);
    _metadataReadEnd = savedEnd;
    return LinuxResult(result);
}

HRESULT CHandler::GetLinuxPackSize(unsigned index, UInt64 &total) {
    sevenzip_squashfs_file *raw = NULL;
    const HRESULT created = CreateLinuxFile(index, &raw);
    std::unique_ptr<sevenzip_squashfs_file, decltype(&sevenzip_squashfs_file_destroy)> file(
        raw, sevenzip_squashfs_file_destroy);
    if (created == S_FALSE)
        _incompleteBlocklists.insert(index);
    RINOK(created)
    const CNode &node = _nodes[index];
    total = 0;
    for (UInt64 i = 0; i < node.GetNumBlocks(_h); ++i) {
        uint64_t start;
        unsigned size;
        const HRESULT result = ReadLinuxFileBlock(file.get(), i, &start, &size);
        if (result == S_FALSE)
            _incompleteBlocklists.insert(index);
        RINOK(result)
        total += GET_COMPRESSED_BLOCK_SIZE(size);
        if (_openCallback && (i & 255) == 0) {
            _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
            RINOK(_directoryCallbackResult)
        }
    }
    if (node.ThereAreFrags()) {
        if (FragmentUnavailable(node))
            return S_FALSE;
        const CFrag &frag = _frags[node.Frag];
        const UInt32 size = GET_COMPRESSED_BLOCK_SIZE(frag.Size);
        if (size == 0 || size > _h.BlockSize || frag.StartBlock > _h.Size ||
            size > _h.Size - frag.StartBlock)
            return S_FALSE;
        if (node.Offset == 0)
            total += size;
    }
    return S_OK;
}

int CHandler::EmitAttribute(void *opaque, const char *name, const void *value,
                            size_t size) noexcept {
    auto &context = *static_cast<CAttributeContext *>(opaque);
    try {
        if (context.Owner->_openCallback) {
            context.Result = context.Owner->_openCallback->SetCompleted(NULL, NULL);
            context.Owner->_directoryCallbackResult = context.Result;
        }
        if (context.Result == S_OK)
            context.Result = AppendArchiveAttribute(*context.Owner->_attributes[context.Node], name,
                                                    value, size);
    } catch (...) {
        context.Result = E_FAIL;
    }
    return context.Result == S_OK ? 0 : -5;
}

HRESULT CHandler::AddAttributeItems(int root) {
    unsigned count = _items.Size();
    if (!_attributes[(unsigned)root]->empty() || _incompleteDirectories.count(root)) {
        if (_items.Size() >= kNumFilesMax)
            return S_FALSE;
        _rootAttributeName = "[ROOT]";
        for (;;) {
            bool collision = false;
            for (unsigned i = 0; i < count; ++i)
                if (_items[i].Parent < 0 &&
                    StringsAreEqualNoCase(UString(GetPath(i)), UString(_rootAttributeName)))
                    collision = true;
            if (!collision)
                break;
            _rootAttributeName += '_';
        }
        CItem item;
        item.Construct();
        item.Node = root;
        item.Attribute = -2;
        _items.Add(item);
        ++count;
    }
    struct CaseLess {
        bool operator()(const UString &a, const UString &b) const {
            return MyStringCompareNoCase(a, b) < 0;
        }
    };
    std::set<UString, CaseLess> paths;
    for (unsigned i = 0; i < count; ++i)
        paths.insert(MultiByteToUnicodeString(GetPath(i), _openCodePage));
    for (unsigned i = 0; i < count; ++i) {
        const int node = _items[i].Node;
        if (node < 0)
            continue;
        const auto &attributes = *_attributes[(unsigned)node];
        for (unsigned a = 0; a < attributes.size(); ++a) {
            if (_items.Size() >= kNumFilesMax)
                return S_FALSE;
            CItem item;
            item.Construct();
            item.Node = node;
            item.Parent = (int)i;
            item.Attribute = (int)a;
            item.RecoveredInode = _items[i].RecoveredInode;
            const UString prefix = MultiByteToUnicodeString(GetPath(i), _openCodePage);
            for (;;) {
                UString path = prefix;
                path += L':';
                path += attributes[a].name;
                if (item.AttributeSuffix) {
                    path += L'#';
                    path.Add_UInt32(item.AttributeSuffix);
                }
                if (paths.insert(path).second)
                    break;
                ++item.AttributeSuffix;
            }
            _items.Add(item);
        }
    }
    return S_OK;
}
#endif

static const Byte kProps[] = {
    kpidPath,
#ifdef SEVENZIP_LINUX_SQUASHFS
    kpidIsAltStream, kpidError,
#endif
    kpidIsDir,       kpidSize,  kpidPackSize, kpidMTime,  kpidPosixAttrib, kpidUserId,
    kpidGroupId,     kpidINode, kpidLinks,    kpidSymLink
    // kpidLinks,
    // kpidOffset
};

#ifdef SEVENZIP_LINUX_SQUASHFS
static const PROPID kpidReadLimitations = kpidUserDefined;
static const char kAttributeLimitations[] = "Some SquashFS extended attributes are unavailable. "
                                            "Readable file data and attributes remain accessible.";
static const char kOwnershipLimitations[] =
    "Some SquashFS ownership metadata is unavailable. User or group IDs may be unknown. Readable "
    "file data remains accessible.";
static const char kFragmentLimitations[] =
    "Some SquashFS file fragments are unavailable. Only readable prefixes of affected files are "
    "available; copying them reports an error.";
static const CStatProp kArcProps[] = {
    {NULL, kpidHeadersSize, VT_UI8}, {NULL, kpidFileSystem, VT_BSTR},
    {NULL, kpidMethod, VT_BSTR},     {NULL, kpidClusterSize, VT_UI4},
    {NULL, kpidBigEndian, VT_BOOL},  {NULL, kpidCTime, VT_FILETIME},
    {NULL, kpidCharacts, VT_BSTR},   {NULL, kpidCodePage, VT_BSTR},
    {NULL, kpidReadOnly, VT_BOOL},   {"Read-only limitations", kpidReadLimitations, VT_BSTR}};
IMP_IInArchive_Props IMP_IInArchive_ArcProps_WITH_NAME
#else
static const Byte kArcProps[] = {
    kpidHeadersSize, kpidFileSystem, kpidMethod,  kpidClusterSize, kpidBigEndian,
    kpidCTime,       kpidCharacts,   kpidCodePage
    // kpidNumBlocks
};

IMP_IInArchive_Props IMP_IInArchive_ArcProps
#endif

    static HRESULT LzoDecode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen) {
    SizeT destRem = *destLen;
    SizeT srcRem = *srcLen;
    *destLen = 0;
    *srcLen = 0;
    const Byte *destStart = dest;
    const Byte *srcStart = src;
    unsigned mode = 0;

    {
        if (srcRem == 0)
            return S_FALSE;
        UInt32 b = *src;
        if (b > 17) {
            src++;
            srcRem--;
            b -= 17;
            mode = (b < 4 ? 1 : 4);
            if (b > srcRem || b > destRem)
                return S_FALSE;
            srcRem -= b;
            destRem -= b;
            do
                *dest++ = *src++;
            while (--b);
        }
    }

    for (;;) {
        if (srcRem < 3)
            return S_FALSE;
        UInt32 b = *src++;
        srcRem--;
        UInt32 len, back;

        if (b >= 64) {
            srcRem--;
            back = ((b >> 2) & 7) + ((UInt32)*src++ << 3);
            len = (b >> 5) + 1;
        } else if (b < 16) {
            if (mode == 0) {
                if (b == 0) {
                    for (b = 15;; b += 255) {
                        if (srcRem == 0)
                            return S_FALSE;
                        UInt32 b2 = *src++;
                        srcRem--;
                        if (b2 != 0) {
                            b += b2;
                            break;
                        }
                    }
                }

                b += 3;
                if (b > srcRem || b > destRem)
                    return S_FALSE;
                srcRem -= b;
                destRem -= b;
                mode = 4;
                do
                    *dest++ = *src++;
                while (--b);
                continue;
            }

            srcRem--;
            back = (b >> 2) + ((UInt32)*src++ << 2);
            len = 2;
            if (mode == 4) {
                back += (1 << 11);
                len = 3;
            }
        } else {
            UInt32 bOld = b;
            b = (b < 32 ? 7 : 31);
            len = bOld & b;

            if (len == 0) {
                for (len = b;; len += 255) {
                    if (srcRem == 0)
                        return S_FALSE;
                    UInt32 b2 = *src++;
                    srcRem--;
                    if (b2 != 0) {
                        len += b2;
                        break;
                    }
                }
            }

            len += 2;
            if (srcRem < 2)
                return S_FALSE;
            b = *src;
            back = (b >> 2) + ((UInt32)src[1] << 6);
            src += 2;
            srcRem -= 2;
            if (bOld < 32) {
                back += ((bOld & 8) << 11);
                if (back == 0) {
                    *destLen = (size_t)(dest - destStart);
                    *srcLen = (size_t)(src - srcStart);
                    return S_OK;
                }
                back += (1 << 14) - 1;
            }
        }

        back++;
        if (len > destRem || (size_t)(dest - destStart) < back)
            return S_FALSE;
        destRem -= len;
        Byte *destTemp = dest - back;
        dest += len;

        do {
            *(destTemp + back) = *destTemp;
            destTemp++;
        } while (--len);

        b &= 3;
        mode = b;
        if (b == 0)
            continue;
        if (b > srcRem || b > destRem)
            return S_FALSE;
        srcRem -= b;
        destRem -= b;
        *dest++ = *src++;
        if (b > 1) {
            *dest++ = *src++;
            if (b > 2)
                *dest++ = *src++;
        }
    }
}

HRESULT CHandler::Decompress(ISequentialOutStream *outStream, Byte *outBuf, bool *outBufWasWritten,
                             UInt32 *outBufWasWrittenSize, UInt32 inSize, UInt32 outSizeMax) {
    if (outBuf) {
        *outBufWasWritten = false;
        *outBufWasWrittenSize = 0;
    }
    UInt32 method = _h.Method;
    if (_h.SeveralMethods) {
        Byte b;
        RINOK(ReadStream_FALSE(_stream, &b, 1))
        RINOK(_stream->Seek(-1, STREAM_SEEK_CUR, NULL))
        method = (b == 0x5D ? kMethod_LZMA : kMethod_ZLIB);
    }

    if (method == kMethod_ZLIB && _needCheckLzma) {
        Byte b;
        RINOK(ReadStream_FALSE(_stream, &b, 1))
        RINOK(_stream->Seek(-1, STREAM_SEEK_CUR, NULL))
        if (b == 0) {
            _noPropsLZMA = true;
            method = _h.Method = kMethod_LZMA;
        }
        _needCheckLzma = false;
    }

    if (method == kMethod_ZLIB) {
        _zlibDecoder.Create_if_Empty();
        const UInt64 outputLimit = outSizeMax;
        RINOK(_zlibDecoder.Interface()->Code(_limitedInStream, outStream, NULL, &outputLimit, NULL))
        if (inSize != _zlibDecoder->GetInputProcessedSize())
            return S_FALSE;
    }
    /*
    else if (method == kMethod_LZMA)
    {
      _lzmaDecoder.Create_if_Empty();
      // _lzmaDecoder->FinishStream = true;
      const UInt32 kPropsSize = LZMA_PROPS_SIZE + 8;
      Byte props[kPropsSize];
      UInt32 propsSize;
      UInt64 outSize;
      if (_noPropsLZMA)
      {
        props[0] = 0x5D;
        SetUi32(&props[1], _h.BlockSize);
        propsSize = 0;
        outSize = outSizeMax;
      }
      else
      {
        RINOK(ReadStream_FALSE(_limitedInStream, props, kPropsSize));
        propsSize = kPropsSize;
        outSize = GetUi64(&props[LZMA_PROPS_SIZE]);
        if (outSize > outSizeMax)
          return S_FALSE;
      }
      RINOK(_lzmaDecoderSpec->SetDecoderProperties2(props, LZMA_PROPS_SIZE));
      RINOK(_lzmaDecoder->Code(_limitedInStream, outStream, NULL, &outSize, NULL));
      if (inSize != propsSize + _lzmaDecoderSpec->GetInputProcessedSize())
        return S_FALSE;
    }
    */
    else {
        if (_inputBuffer.Size() < inSize)
            _inputBuffer.Alloc(inSize);
        RINOK(ReadStream_FALSE(_stream, _inputBuffer, inSize))

        Byte *dest = outBuf;
        if (!outBuf) {
            dest = _dynOutStream->GetBufPtrForWriting(outSizeMax);
            if (!dest)
                return E_OUTOFMEMORY;
        }

        SizeT destLen = outSizeMax, srcLen = inSize;

        if (method == kMethod_LZO
#ifdef SEVENZIP_LINUX_SQUASHFS
            && _h.IsOldVersion()
#endif
        ) {
            RINOK(LzoDecode(dest, &destLen, _inputBuffer, &srcLen))
#ifdef SEVENZIP_LINUX_SQUASHFS
        } else if (method == kMethod_LZO || method == kMethod_LZ4) {
            const int result =
                sevenzip_squashfs_decode(_linuxDecoder, _inputBuffer, inSize, dest, outSizeMax);
            if (result < 0)
                return result == -12 ? E_OUTOFMEMORY : S_FALSE;
            destLen = (SizeT)result;
#endif
        } else if (method == kMethod_LZMA) {
            Byte props[5];
            const Byte *src = _inputBuffer;

            if (_noPropsLZMA) {
                props[0] = 0x5D;
                SetUi32(&props[1], _h.BlockSize)
            } else {
                const UInt32 kPropsSize = LZMA_PROPS_SIZE + 8;
                if (inSize < kPropsSize)
                    return S_FALSE;
                memcpy(props, src, LZMA_PROPS_SIZE);
                UInt64 outSize = GetUi64(src + LZMA_PROPS_SIZE);
                if (outSize > outSizeMax)
                    return S_FALSE;
                destLen = (SizeT)outSize;
                src += kPropsSize;
                inSize -= kPropsSize;
                srcLen = inSize;
            }

            ELzmaStatus status;
            SRes res = LzmaDecode(dest, &destLen, src, &srcLen, props, LZMA_PROPS_SIZE,
                                  LZMA_FINISH_END, &status, &g_Alloc);
            if (res != 0)
                return SResToHRESULT(res);
            if (status != LZMA_STATUS_FINISHED_WITH_MARK &&
                status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK)
                return S_FALSE;
        } else if (method == kMethod_ZSTD) {
            const Byte *src = _inputBuffer;

            if (!_zstd) {
                _zstd = ZstdDec_Create(&g_AlignedAlloc, &g_AlignedAlloc);
                if (!_zstd)
                    return E_OUTOFMEMORY;
            }

            CZstdDecState state;
            ZstdDecState_Clear(&state);

            state.inBuf = src;
            state.inLim = srcLen; //  + 1; for debug
            // state.outStep = outSizeMax;

            state.outBuf_fromCaller = dest;
            state.outBufSize_fromCaller = outSizeMax;
            // state.mustBeFinished = True;

            ZstdDec_Init(_zstd);
            SRes sres;
            for (;;) {
                sres = ZstdDec_Decode(_zstd, &state);
                if (sres != SZ_OK)
                    break;
                if (state.inLim == state.inPos && (state.status == ZSTD_STATUS_NEEDS_MORE_INPUT ||
                                                   state.status == ZSTD_STATUS_FINISHED_FRAME))
                    break;
                // sres = sres;
                // break; // for debug
            }

            CZstdDecResInfo info;
            // ZstdDecInfo_Clear(&stat);
            // stat->InSize = state.inPos;
            ZstdDec_GetResInfo(_zstd, &state, sres, &info);
            sres = info.decode_SRes;
            if (sres == SZ_OK) {
                if (state.status != ZSTD_STATUS_FINISHED_FRAME
                    // ||stat.UnexpededEnd
                    || info.extraSize != 0 || state.inLim != state.inPos)
                    sres = SZ_ERROR_DATA;
            }
            if (sres != SZ_OK)
                return SResToHRESULT(sres);
            if (state.winPos > outSizeMax)
                return E_FAIL;
            // memcpy(dest, state.dic, state.dicPos);
            destLen = state.winPos;
        } else {
            ECoderStatus status;
            const SRes res = XzUnpacker_CodeFull(&_xz, dest, &destLen, _inputBuffer, &srcLen,
                                                 CODER_FINISH_END, &status);
            if (res != 0)
                return SResToHRESULT(res);
            if (status != CODER_STATUS_NEEDS_MORE_INPUT || !XzUnpacker_IsStreamWasFinished(&_xz))
                return S_FALSE;
        }

        if (inSize != srcLen)
            return S_FALSE;
        if (outBuf) {
            *outBufWasWritten = true;
            *outBufWasWrittenSize = (UInt32)destLen;
        } else
            _dynOutStream->UpdateSize(destLen);
    }
    return S_OK;
}

// in  : packSize : allowed packSize limit
// out : packSize : real packSize in block
// out(packSize) <= in(packSize)
HRESULT CHandler::ReadMetadataBlock(UInt32 &packSize, bool *boundaryKnown,
                                    CDynBufSeqOutStream *output) {
    if (boundaryKnown)
        *boundaryKnown = false;
    if (!output)
        output = _dynOutStream.ClsPtr();
    Byte temp[3];
    const unsigned offset = _h.NeedCheckData() ? 3 : 2;
    if (offset > packSize)
        return S_FALSE;
    RINOK(ReadStream_FALSE(_stream, temp, offset))
    // if (NeedCheckData && Major < 4) checkByte must be = 0xFF
    const bool be = _h.be;
    UInt32 size = Get16(temp);
    const bool isCompressed = ((size & kNotCompressedBit16) == 0);
    if (size != kNotCompressedBit16)
        size &= ~kNotCompressedBit16;
    if (size == 0 || size > kMetadataBlockSize)
        return S_FALSE;
    const UInt32 packSize2 = offset + size;
    if (packSize2 > packSize)
        return S_FALSE;
    packSize = packSize2;
    if (boundaryKnown)
        *boundaryKnown = true;
    if (isCompressed) {
        const size_t previousSize = output->GetSize();
        Byte *buffer = output->GetBufPtrForWriting(kMetadataBlockSize);
        if (!buffer)
            return E_OUTOFMEMORY;
        bool written = false;
        UInt32 writtenSize = 0;
        _limitedInStream->Init(size);
        RINOK(Decompress(output, buffer, &written, &writtenSize, size, kMetadataBlockSize))
        if (written)
            output->UpdateSize(writtenSize);
        const size_t unpackSize = output->GetSize() - previousSize;
        if (unpackSize == 0 || unpackSize > kMetadataBlockSize)
            return S_FALSE;
    } else {
        // size != 0 here
        Byte *buf = output->GetBufPtrForWriting(size);
        if (!buf)
            return E_OUTOFMEMORY;
        RINOK(ReadStream_FALSE(_stream, buf, size))
        output->UpdateSize(size);
    }
    return S_OK;
}

HRESULT CHandler::ReadMetadataBlock2() {
    _dynOutStream->Init();
    UInt64 position;
    RINOK(_stream->Seek(0, STREAM_SEEK_CUR, &position))
    if (position > _h.Size)
        return S_FALSE;
    UInt32 packSize = (UInt32)MyMin(_h.Size - position, (UInt64)kMetadataBlockSize + 3);
    return ReadMetadataBlock(packSize);
}

HRESULT CHandler::ReadData(CData &data, UInt64 start, UInt64 end) {
    if (end < start || end > _h.Size)
        return S_FALSE;
    const UInt64 size64 = end - start;
    if (size64 >= ((UInt64)1 << 32))
        return S_FALSE;
    const UInt32 size = (UInt32)size64;
    RINOK(Seek2(start))
    _dynOutStream->Init();
    UInt32 packPos = 0;
    while (packPos != size) {
        data.PackPos.Add(packPos);
        data.UnpackPos.Add((UInt32)_dynOutStream->GetSize());
        UInt32 packSize = size - packPos;
        RINOK(ReadMetadataBlock(packSize)) {
            const size_t tSize = _dynOutStream->GetSize();
            if (tSize != (UInt32)tSize)
                return S_FALSE;
        }
        packPos += packSize;
        data.PackEnd.Add(packPos);
    }
    _dynOutStream->CopyToBuffer(data.Data);
    return S_OK;
}

#ifdef SEVENZIP_LINUX_SQUASHFS
HRESULT CHandler::ReadDirectoryData(UInt64 end) {
    const UInt64 start = _h.DirTable;
    if (end < start || end > _h.Size)
        return S_FALSE;
    _directoryDataEnd = end;
    return S_OK;
}

HRESULT CHandler::ScanCachedInodes(bool forRecovery) {
    if (_inodeInventoryScanned)
        return S_OK;
    const auto first = _inodeBlockIndexes.find(0);
    if (first == _inodeBlockIndexes.end())
        return S_OK;
    const UInt32 end = _inodesData.ContiguousEnd(first->second);
    const UInt32 *starts = &_inodesData.UnpackPos[0];
    const unsigned last =
        (unsigned)(std::upper_bound(starts, starts + _inodesData.UnpackPos.Size(), end - 1) -
                   starts - 1);
    if (_inodesData.PackEnd[last] != _h.DirTable - _h.InodeTable)
        return S_OK;
    _inodeInventoryScanned = true;
    UInt64 pos = _inodesData.UnpackPos[first->second];
    for (UInt32 i = 0; i < _h.NumInodes; ++i) {
        // Optional directory indexes are not required by namespace enumeration.
        // Their variable extent matters only when proving a recovery inventory.
        if (!forRecovery && pos < end && end - pos >= 40 &&
            GetUi16(_inodesData.Data + pos) == kType_DIR + 7 &&
            GetUi16(_inodesData.Data + pos + 32) != 0) {
            _inodeInventoryScanned = false;
            return S_OK;
        }
        int node;
        UInt64 length = 0;
        const HRESULT result = pos < end ? ReadNode((UInt32)pos, node, &length) : S_FALSE;
        RINOK(_directoryCallbackResult)
        if (result == S_FALSE) {
            _inodeTableIncomplete = true;
            break;
        }
        RINOK(result)
        pos += length;
    }
    if (pos != end)
        _inodeTableIncomplete = true;
    return S_OK;
}

HRESULT CHandler::EnsureInodeInventory() {
    RINOK(ScanCachedInodes(true))
    if (_inodeInventoryScanned || _inodeTableIncomplete)
        return S_OK;
    UInt64 position = 0, next;
    const HRESULT result = ReadMetadataRun(_inodesData, _inodeBlockIndexes, _h.InodeTable,
                                           _h.DirTable, position, next, UINT64_MAX);
    RINOK(_directoryCallbackResult)
    if (result == E_ABORT || result == E_OUTOFMEMORY || result == E_NOTIMPL)
        return result;
    if (result == S_OK) {
        RINOK(ScanCachedInodes(true))
    }
    if (result != S_OK || !_inodeInventoryScanned) {
        _inodeInventoryScanned = true;
        _inodeTableIncomplete = true;
    }
    return S_OK;
}

HRESULT CHandler::ReadMetadataRun(CData &data, std::map<UInt64, unsigned> &indexes, UInt64 start,
                                  UInt64 end, UInt64 &position, UInt64 &next, UInt64 needed) {
    if (end < start || end > _h.Size || position > end - start)
        return S_FALSE;
    const UInt64 size = end - start;
    next = size;
    CMyComPtr2_Create<ISequentialOutStream, CDynBufSeqOutStream> block;
    CRecordVector<UInt64> packedPositions, packedEnds;
    CRecordVector<UInt32> unpackedPositions;
    _dynOutStream->Init();
    HRESULT result = S_OK;
    UInt32 blocks = 0;
    const size_t oldSize = data.Size();
    while (position < size && _dynOutStream->GetSize() < needed) {
        UInt32 packed = (UInt32)MyMin(size - position, (UInt64)kMetadataBlockSize + 3);
        bool boundaryKnown = false;
        block->Init();
        result = Seek2(start + position);
        if (result == S_OK)
            result = ReadMetadataBlock(packed, &boundaryKnown, block.ClsPtr());
        if (result == E_ABORT || result == E_OUTOFMEMORY || result == E_NOTIMPL)
            return result;
        if (_openCallback && ((blocks & 255) == 0 || result != S_OK)) {
            _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
            RINOK(_directoryCallbackResult)
        }
        ++blocks;
        next = boundaryKnown ? position + packed : size;
        if (result != S_OK)
            break;
        if (result == S_OK) {
            const size_t before = _dynOutStream->GetSize();
            const size_t length = block->GetSize();
            if (oldSize >= UINT32_MAX || before >= UINT32_MAX - oldSize ||
                length >= UINT32_MAX - oldSize - before)
                return S_FALSE;
            UInt32 processed = 0;
            RINOK(_dynOutStream.Interface()->Write(block->GetBuffer(), (UInt32)length, &processed))
            if (processed != length)
                return E_FAIL;
            packedPositions.Add(position);
            packedEnds.Add(next);
            unpackedPositions.Add((UInt32)before);
        }
        position = next;
    }
    if (_openCallback) {
        _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
        RINOK(_directoryCallbackResult)
    }
    const size_t length = _dynOutStream->GetSize();
    if (length) {
        const UInt32 end = (UInt32)(oldSize + length);
        RINOK(data.Append(_dynOutStream->GetBuffer(), length))
        FOR_VECTOR(i, packedPositions) {
            indexes[packedPositions[i]] = data.PackPos.Size();
            data.PackPos.Add(packedPositions[i]);
            data.PackEnd.Add(packedEnds[i]);
            data.UnpackPos.Add((UInt32)oldSize + unpackedPositions[i]);
            data.UnpackEnd.Add(end);
        }
    }
    return result;
}
#endif

struct CTempItem {
    UInt32 StartBlock;
    UInt32 Number;
    // UInt32 iNodeNumber1;
    UInt32 Offset;
    // UInt16 iNodeNumber2;
    UInt16 Type;
};

HRESULT CHandler::FindNode(UInt32 startBlock, UInt32 offset, int &nodeIndex) {
    nodeIndex = -1;

#ifdef SEVENZIP_LINUX_SQUASHFS
    if (!_h.IsOldVersion()) {
        if (startBlock >= _h.DirTable - _h.InodeTable)
            return S_FALSE;
        UInt64 wanted = 1;
        UInt32 previous = 0;
        for (;;) {
            auto found = _inodeBlockIndexes.find(startBlock);
            if (found == _inodeBlockIndexes.end() || wanted > 1) {
                if (_unavailableInodeBlocks.count(startBlock))
                    return S_FALSE;
                UInt64 position = startBlock, next;
                const HRESULT result =
                    ReadMetadataRun(_inodesData, _inodeBlockIndexes, _h.InodeTable, _h.DirTable,
                                    position, next, wanted);
                RINOK(_directoryCallbackResult)
                if (result == E_ABORT || result == E_OUTOFMEMORY || result == E_NOTIMPL)
                    return result;
                if (result != S_OK) {
                    _inodeTableIncomplete = true;
                    if (position <= UINT32_MAX)
                        _unavailableInodeBlocks.insert((UInt32)position);
                }
                found = _inodeBlockIndexes.find(startBlock);
            }
            if (found == _inodeBlockIndexes.end())
                return S_FALSE;
            const unsigned block = found->second;
            const UInt32 begin = _inodesData.UnpackPos[block];
            const UInt32 firstEnd = block + 1 < _inodesData.UnpackPos.Size()
                                        ? _inodesData.UnpackPos[block + 1]
                                        : (UInt32)_inodesData.Size();
            if (offset >= firstEnd - begin)
                return S_FALSE;
            const UInt32 available = _inodesData.ContiguousEnd(block) - begin - offset;
            if (wanted > 1 && available <= previous) {
                _inodeTableIncomplete = true;
                return S_FALSE;
            }
            UInt32 required = 0;
            const HRESULT result = ReadNode(begin + offset, nodeIndex, NULL, &required);
            RINOK(_directoryCallbackResult)
            if (result == S_OK)
                return LoadNode((unsigned)nodeIndex);
            if (!required)
                return result;
            previous = available;
            wanted = (UInt64)offset + required;
        }
    }
#endif
    const int blockIndex = _inodesData.PackPos.FindInSorted(startBlock);
    if (blockIndex < 0)
        return S_FALSE;
    const UInt32 inodeBlockEnd = (unsigned)blockIndex + 1 < _inodesData.UnpackPos.Size()
                                     ? _inodesData.UnpackPos[blockIndex + 1]
                                     : (UInt32)_inodesData.Size();
    if (offset >= inodeBlockEnd - _inodesData.UnpackPos[blockIndex])
        return S_FALSE;
    UInt32 unpackPos = _inodesData.UnpackPos[blockIndex] + offset;
    if (unpackPos < offset)
        return S_FALSE;

    nodeIndex = _nodesPos.FindInSorted(unpackPos);
    return nodeIndex < 0 ? S_FALSE : S_OK;
}

HRESULT CHandler::OpenDir(int parent, UInt32 startBlock, UInt32 offset, unsigned level,
                          int &nodeIndex) {
    nodeIndex = -1;
    if (level > kNumDirLevelsMax)
        return S_FALSE;
    const HRESULT nodeResult = FindNode(startBlock, offset, nodeIndex);
    RINOK(_directoryCallbackResult)
    RINOK(nodeResult)

    const CNode &n = _nodes[nodeIndex];
    if (!n.IsDir())
        return parent < 0 ? S_FALSE : S_OK;
    if (_activeDirs[nodeIndex])
        return S_FALSE;
    if (_h.Major >= 3 && n.FileSize == 3)
        return S_OK;
    _activeDirs[nodeIndex] = true;
    CRecordVector<CTempItem> tempItems;
    const HRESULT readResult = ReadDirItems(parent, n, tempItems, _directoryCallbackResult);
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (!_h.IsOldVersion() && _openCallback && _directoryCallbackResult == S_OK &&
        (readResult == S_OK || readResult == S_FALSE))
        _directoryCallbackResult = _openCallback->SetCompleted(NULL, NULL);
#endif
    RINOK(_directoryCallbackResult)
    if (readResult != S_OK) {
#ifdef SEVENZIP_LINUX_SQUASHFS
        if (readResult != S_FALSE || _h.IsOldVersion())
            return readResult;
        _incompleteDirectories.insert(nodeIndex);
#else
        return readResult;
#endif
    }
    const unsigned startItemIndex = _items.Size() - tempItems.Size();
    FOR_VECTOR(i, tempItems) {
        const CTempItem &tempItem = tempItems[i];
        const unsigned index = startItemIndex + i;
        int childNode;
        HRESULT result =
            OpenDir((int)index, tempItem.StartBlock, tempItem.Offset, level + 1, childNode);
        RINOK(_directoryCallbackResult)
#ifdef SEVENZIP_LINUX_SQUASHFS
        if (result == S_FALSE && childNode < 0 && !_h.IsOldVersion() && !_inodeTableIncomplete) {
            RINOK(EnsureInodeInventory())
            RINOK(_directoryCallbackResult)
            const auto found = _inodeReferences.find(tempItem.Number);
            if (_inodeInventoryScanned && !_inodeTableIncomplete &&
                found != _inodeReferences.end() && found->second.first != UINT64_MAX &&
                found->second.second == tempItem.Type) {
                const UInt64 reference = found->second.first;
                _items[index].RecoveredInode = true;
                _inodeMappingLimitations = true;
                _incompleteDirectories.insert(nodeIndex);
                result = OpenDir((int)index, (UInt32)(reference >> 16), (UInt32)reference & 0xFFFF,
                                 level + 1, childNode);
                RINOK(_directoryCallbackResult)
            }
        }
#endif
        if (result != S_OK) {
#ifdef SEVENZIP_LINUX_SQUASHFS
            if (result != S_FALSE || _h.IsOldVersion())
                return result;
            _incompleteDirectories.insert(nodeIndex);
            if (childNode >= 0)
                _incompleteDirectories.insert(childNode);
            _items[index].UnavailableType = tempItem.Type;
#else
            return result;
#endif
        }
        _items[index].Node = childNode;
    }
    _activeDirs[nodeIndex] = false;
    return S_OK;
}

HRESULT CHandler::ReadDirItems(int parent, const CNode &n, CRecordVector<CTempItem> &tempItems,
                               HRESULT &callbackResult) {
    UInt32 rem = 0;
    const Byte *p = NULL;
    if ((UInt32)n.StartBlock != n.StartBlock)
        return S_FALSE;
#ifdef SEVENZIP_LINUX_SQUASHFS
    const bool streamed = !_h.IsOldVersion();
    uint64_t directoryBlock = 0;
    unsigned directoryOffset = n.Offset;
    UInt32 reads = 0;
    Byte field[256];
    if (streamed) {
        if (n.FileSize < 3 || n.StartBlock >= _directoryDataEnd - _h.DirTable)
            return S_FALSE;
        directoryBlock = _h.DirTable + n.StartBlock;
        rem = (UInt32)n.FileSize - 3;
    } else
#endif
    {
        const int blockIndex = _dirs.PackPos.FindInSorted((UInt32)n.StartBlock);
        if (blockIndex < 0)
            return S_FALSE;
        const UInt32 dirBlockEnd = (unsigned)blockIndex + 1 < _dirs.UnpackPos.Size()
                                       ? _dirs.UnpackPos[blockIndex + 1]
                                       : (UInt32)_dirs.Size();
        if (n.Offset >= dirBlockEnd - _dirs.UnpackPos[blockIndex])
            return S_FALSE;
        const UInt32 unpackPos = _dirs.UnpackPos[blockIndex] + n.Offset;
        if (unpackPos < n.Offset || unpackPos > _dirs.Size())
            return S_FALSE;
        rem = _dirs.ContiguousEnd((unsigned)blockIndex) - unpackPos;
        p = _dirs.Data + unpackPos;
        UInt32 fileSize = (UInt32)n.FileSize;
        // for some squashfs files: fileSize = rem + 3  !!!
        if (_h.Major >= 3) {
            if (fileSize < 3)
                return S_FALSE;
            fileSize -= 3;
        }
        if (fileSize > rem)
            return S_FALSE;
        rem = fileSize;
    }
    const auto readField = [&](UInt32 size) -> HRESULT {
#ifdef SEVENZIP_LINUX_SQUASHFS
        if (streamed) {
            if (size > rem || size > sizeof(field))
                return S_FALSE;
            _linuxReadResult = S_OK;
            const UInt64 savedEnd = _metadataReadEnd;
            _metadataReadEnd = _directoryDataEnd;
            const int read = sevenzip_squashfs_directory_read(_linuxMetadata, &directoryBlock,
                                                              &directoryOffset, field, size);
            _metadataReadEnd = savedEnd;
            const HRESULT result = LinuxResult(read);
            if (result == E_ABORT || result == E_OUTOFMEMORY || result == E_NOTIMPL)
                return result;
            if (_openCallback && ((reads++ & 255) == 0 || result != S_OK)) {
                callbackResult = _openCallback->SetCompleted(NULL, NULL);
                RINOK(callbackResult)
            }
            if (result != S_OK)
                return S_FALSE;
            p = field;
        }
#else
        (void)size;
#endif
        return S_OK;
    };

    AString tempString;

    while (rem != 0) {
        RINOK(readField(_h.Major <= 2 ? 4 : _h.Major == 3 ? 9 : 12))
        const bool be = _h.be;
        UInt32 count;
        CTempItem tempItem;
        UInt32 inodeNumber = 0;
        tempItem.Number = 0;
        if (_h.Major <= 2) {
            if (rem < 4)
                return S_FALSE;
            count = p[0];
            tempItem.StartBlock = Get32(p);
            if (be)
                tempItem.StartBlock &= 0xFFFFFF;
            else
                tempItem.StartBlock >>= 8;
            p += 4;
            rem -= 4;
        } else {
            if (_h.Major == 3) {
                if (rem < 9)
                    return S_FALSE;
                count = p[0];
                p += 1;
                rem -= 1;
            } else {
                if (rem < 12)
                    return S_FALSE;
                count = GetUi32(p);
                p += 4;
                rem -= 4;
            }
            GET_32(0, tempItem.StartBlock);
            if (_h.Major == 4)
                inodeNumber = GetUi32(p + 4);
            // GET_32 (4, tempItem.iNodeNumber1);
            p += 8;
            rem -= 8;
        }
        if (_h.Major == 4 && count >= 256)
            return S_FALSE;
        count++;

        for (UInt32 i = 0; i < count; i++) {
            if (rem == 0)
                return S_FALSE;

            const UInt32 nameOffset = _h.GetFileNameOffset();
            if (rem < nameOffset)
                return S_FALSE;
            RINOK(readField(nameOffset))

            if (_items.Size() >= kNumFilesMax)
                return S_FALSE;
            if (_openCallback) {
                const UInt64 numFiles = _items.Size();
                if ((numFiles & 0xFFFF) == 0) {
                    callbackResult = _openCallback->SetCompleted(&numFiles, NULL);
                    RINOK(callbackResult)
                }
            }

            CItem item;
            item.Construct();
#ifdef SEVENZIP_LINUX_SQUASHFS
            item.RecoveredInode = parent >= 0 && _items[parent].RecoveredInode;
#endif
#ifdef SEVENZIP_LINUX_SQUASHFS
            if (!streamed)
#endif
                item.Ptr = (UInt32)(p - (const Byte *)_dirs.Data);

            UInt32 size;
            if (_h.IsOldVersion()) {
                UInt32 t = Get16(p);
                if (be) {
                    tempItem.Offset = t >> 3;
                    tempItem.Type = (UInt16)(t & 0x7);
                } else {
                    tempItem.Offset = t & 0x1FFF;
                    tempItem.Type = (UInt16)(t >> 13);
                }
                size = (UInt32)p[2];
                /*
                if (_h.Major > 2)
                  tempItem.iNodeNumber2 = Get16(p + 3);
                */
            } else {
                GET_16(0, tempItem.Offset);
                if (_h.Major == 4) {
                    const Int64 number = (Int64)inodeNumber + (Int16)GetUi16(p + 2);
                    tempItem.Number =
                        number > 0 && (UInt64)number <= _h.NumInodes ? (UInt32)number : 0;
                }
                // GET_16 (2, tempItem.iNodeNumber2);
                GET_16(4, tempItem.Type);
                GET_16(6, size);
            }
            p += nameOffset;
            rem -= nameOffset;
            size++;
            if (rem < size)
                return S_FALSE;
            RINOK(readField(size))
            if (_h.Major == 4) {
                if (size > 256 || tempItem.Type > kType_SOCK || (size == 1 && p[0] == '.') ||
                    (size == 2 && p[0] == '.' && p[1] == '.'))
                    return S_FALSE;
                for (UInt32 j = 0; j < size; j++)
                    if (p[j] == 0 || p[j] == '/')
                        return S_FALSE;
            }

            if (_openCodePage == CP_UTF8) {
                tempString.SetFrom_CalcLen((const char *)p, size);
                if (!CheckUTF8_AString(tempString))
                    _openCodePage = CP_OEMCP;
            }
#ifdef SEVENZIP_LINUX_SQUASHFS
            if (streamed) {
                AString name;
                name.SetFrom_CalcLen((const char *)p, size);
                _itemNames.Add(name);
            }
#endif

            p += size;
            rem -= size;
            item.Parent = parent;
            _items.Add(item);
            tempItems.Add(tempItem);
        }
    }

    return S_OK;
}

HRESULT CHandler::ReadUids(UInt64 start, UInt32 num, CByteBuffer &ids) {
    const size_t size = (size_t)num * 4;
    ids.Alloc(size);
    if (num == 0)
        return S_OK;
    RINOK(Seek2(start))
    return ReadStream_FALSE(_stream, ids, size);
}

HRESULT CHandler::Open2(IInStream *inStream) {
    {
        Byte buf[kHeaderSize3];
        RINOK(ReadStream_FALSE(inStream, buf, kHeaderSize3))
        if (!_h.Parse(buf))
            return S_FALSE;
        if (!_h.IsSupported())
            return E_NOTIMPL;

        _noPropsLZMA = false;
        _needCheckLzma = false;
        switch (_h.Method) {
        case kMethod_ZLIB:
            _needCheckLzma = _h.IsOldVersion();
            break;
        case kMethod_LZMA:
        case kMethod_LZO:
#ifdef SEVENZIP_LINUX_SQUASHFS
        case kMethod_LZ4:
#endif
        case kMethod_XZ:
        case kMethod_ZSTD:
            break;
        default:
            return E_NOTIMPL;
        }
    }

    _stream = inStream;
    UInt64 inputSize;
    RINOK(inStream->Seek(0, STREAM_SEEK_END, &inputSize))
    if (_h.Size > inputSize || (Int64)_h.Size < 0 || _h.Size < (_h.Major == 4 ? 96 : kHeaderSize3))
        return S_FALSE;
#ifdef SEVENZIP_LINUX_SQUASHFS
    RINOK(InitLinuxDecoder())
#endif

    UInt64 dirEnd = _h.FragTable;
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (!_h.IsOldVersion()) {
        RINOK(LoadLinuxTables(dirEnd))
    } else
#endif
    {
        if (_h.Major < 4) {
            RINOK(ReadUids(_h.UidTable, _h.NumUids, _uids))
            RINOK(ReadUids(_h.GidTable, _h.NumGids, _gids))
        } else {
            if (_h.NumIDs == 0)
                return S_FALSE;
            const UInt32 size = (UInt32)_h.NumIDs * 4;
            _uids.Alloc(size);

            const UInt32 numBlocks = (size + kMetadataBlockSize - 1) / kMetadataBlockSize;
            const UInt32 numBlocksBytes = numBlocks << 3;
            if (_h.UidTable > _h.Size || numBlocksBytes > _h.Size - _h.UidTable)
                return S_FALSE;
            CByteBuffer data(numBlocksBytes);
            RINOK(Seek2(_h.UidTable))
            RINOK(ReadStream_FALSE(inStream, data, numBlocksBytes))

            for (UInt32 i = 0; i < numBlocks; i++) {
                const UInt64 offset = GetUi64(data + i * 8);
                if (offset >= _h.UidTable || (i != 0 && offset <= GetUi64(data + (i - 1) * 8)))
                    return S_FALSE;
                if (i == 0)
                    dirEnd = offset;
                RINOK(Seek2(offset))
                // RINOK(ReadMetadataBlock(NULL, _uids + kMetadataBlockSize * i, packSize,
                // unpackSize));
                RINOK(ReadMetadataBlock2())
                const size_t unpackSize = _dynOutStream->GetSize();
                UInt32 remSize = size & (kMetadataBlockSize - 1);
                if (remSize == 0 || i != numBlocks - 1)
                    remSize = kMetadataBlockSize;
                if (unpackSize != remSize)
                    return S_FALSE;
                memcpy(_uids + kMetadataBlockSize * i, _dynOutStream->GetBuffer(), remSize);
            }
        }

        if (_h.Major == 4 && _h.LookupTable != (UInt64)(Int64)-1) {
            const UInt64 indexBytes =
                (((UInt64)_h.NumInodes * 8 + kMetadataBlockSize - 1) / kMetadataBlockSize) * 8;
            if (_h.NumInodes == 0 || _h.LookupTable > dirEnd ||
                indexBytes != dirEnd - _h.LookupTable)
                return S_FALSE;
            Byte first[8];
            RINOK(Seek2(_h.LookupTable))
            RINOK(ReadStream_FALSE(inStream, first, sizeof(first)))
            dirEnd = GetUi64(first);
            if (dirEnd >= _h.LookupTable)
                return S_FALSE;
        }

        if (_h.NumFrags != 0) {
            if (_h.NumFrags > kNumFilesMax)
                return S_FALSE;
            const unsigned bigFrag = (_h.Major > 2);

            const unsigned fragPtrsInBlockLog = kMetadataBlockSizeLog - (3 + bigFrag);
            const UInt32 numBlocks =
                (_h.NumFrags + (1 << fragPtrsInBlockLog) - 1) >> fragPtrsInBlockLog;
            const size_t numBlocksBytes = (size_t)numBlocks << (2 + bigFrag);
            const UInt64 fragEnd = _h.Major == 4 ? dirEnd : _h.Size;
            if (_h.FragTable > fragEnd || numBlocksBytes > fragEnd - _h.FragTable)
                return S_FALSE;
            CByteBuffer data(numBlocksBytes);
            RINOK(Seek2(_h.FragTable))
            RINOK(ReadStream_FALSE(inStream, data, numBlocksBytes))
            _frags.ClearAndReserve(_h.NumFrags);
            const bool be = _h.be;

            for (UInt32 i = 0; i < numBlocks; i++) {
                const UInt64 offset = bigFrag ? Get64(data + i * 8) : Get32(data + i * 4);
                if (_h.Major == 4) {
                    if (offset >= _h.FragTable)
                        return S_FALSE;
                    if (i == 0)
                        dirEnd = offset;
                }
                RINOK(Seek2(offset))
                RINOK(ReadMetadataBlock2())
                const UInt32 unpackSize = (UInt32)_dynOutStream->GetSize();
                UInt32 remSize = (_h.NumFrags << (3 + bigFrag)) & (kMetadataBlockSize - 1);
                if (remSize == 0 || i != numBlocks - 1)
                    remSize = kMetadataBlockSize;
                if (unpackSize != remSize)
                    return S_FALSE;
                const Byte *buf = _dynOutStream->GetBuffer();
                for (UInt32 j = 0; j < unpackSize;) {
                    CFrag frag;
                    if (bigFrag) {
                        frag.StartBlock = Get64(buf + j);
                        frag.Size = Get32(buf + j + 8);
                        // some archives contain nonzero in unused (buf + j + 12)
                        j += 16;
                    } else {
                        frag.StartBlock = Get32(buf + j);
                        frag.Size = Get32(buf + j + 4);
                        j += 8;
                    }
                    _frags.Add(frag);
                }
            }
            if ((UInt32)_frags.Size() != _h.NumFrags)
                return S_FALSE;
        }
    }

#ifdef SEVENZIP_LINUX_SQUASHFS
    if (!_h.IsOldVersion()) {
        if (_h.InodeTable >= _h.DirTable || _h.DirTable > _h.Size)
            return S_FALSE;
    } else
#endif
    {
        RINOK(ReadData(_inodesData, _h.InodeTable, _h.DirTable))
    }

    const UInt64 absOffset = _h.RootInode >> 16;
    if (absOffset >= ((UInt64)1 << 32))
        return S_FALSE;
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (!_h.IsOldVersion()) {
        int root;
        const HRESULT result = FindNode((UInt32)absOffset, (UInt32)_h.RootInode & 0xFFFF, root);
        RINOK(_directoryCallbackResult)
        RINOK(result)
        if (!_nodes[root].IsDir())
            return S_FALSE;
    } else
#endif
    {
        const UInt32 totalSize = (UInt32)_inodesData.Data.Size();
        const unsigned kMinNodeParseSize = 4;
        if (_h.NumInodes > totalSize / kMinNodeParseSize)
            return S_FALSE;
        _nodesPos.ClearAndReserve(_h.NumInodes);
        _nodes.ClearAndReserve(_h.NumInodes);
        UInt32 pos = 0;
        for (UInt32 i = 0; i < _h.NumInodes; i++) {
            CNode n;
            const Byte *p = _inodesData.Data + pos;
            UInt32 size = totalSize - pos;

            switch (_h.Major) {
            case 1:
                size = n.Parse1(p, size, _h);
                break;
            case 2:
                size = n.Parse2(p, size, _h);
                break;
            case 3:
                size = n.Parse3(p, size, _h);
                break;
            default:
                size = n.Parse4(p, size, _h);
                break;
            }
            if (size == 0)
                return S_FALSE;
            _nodesPos.AddInReserved(pos);
            _nodes.AddInReserved(n);
            pos += size;
        }
        if (pos != totalSize)
            return S_FALSE;
    }
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (!_h.IsOldVersion()) {
        RINOK(ReadDirectoryData(dirEnd))
    } else
#endif
    {
        RINOK(ReadData(_dirs, _h.DirTable, dirEnd))
    }
    _activeDirs.ClearAndReserve(_nodes.Size());
    FOR_VECTOR(i, _nodes)
    _activeDirs.AddInReserved(false);
    int rootNodeIndex;
    RINOK(OpenDir(-1, (UInt32)absOffset, (UInt32)_h.RootInode & 0xFFFF, 0, rootNodeIndex))
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (!_h.IsOldVersion()) {
        RINOK(ScanCachedInodes())
        RINOK(AddAttributeItems(rootNodeIndex))
    }
#endif

    {
        const UInt32 alignSize = 1 << 12;
        Byte buf[alignSize];
        RINOK(Seek2(_h.Size))
        UInt32 rem = (UInt32)(0 - _h.Size) & (alignSize - 1);
        _sizeCalculated = _h.Size;
        if (rem != 0) {
            const HRESULT readResult = ReadStream_FALSE(_stream, buf, rem);
            if (FAILED(readResult))
                return readResult;
            if (readResult == S_OK) {
                size_t i;
                for (i = 0; i < rem && buf[i] == 0; i++)
                    ;
                if (i == rem)
                    _sizeCalculated = _h.Size + rem;
            }
        }
    }
    return S_OK;
}

const Byte *CHandler::GetItemName(unsigned index, unsigned &size) const {
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (!_h.IsOldVersion()) {
        const AString &name = _itemNames[index];
        size = name.Len();
        return (const Byte *)name.Ptr();
    }
#endif
    const bool be = _h.be;
    const Byte *p = _dirs.Data + _items[index].Ptr;
    const unsigned limit = (_h.IsOldVersion() ? (unsigned)p[2] : (unsigned)Get16(p + 6)) + 1;
    p += _h.GetFileNameOffset();
    for (size = 0; size < limit && p[size]; ++size)
        ;
    return p;
}

AString CHandler::GetPath(unsigned index) const {
#ifdef SEVENZIP_LINUX_SQUASHFS
    const CItem &entry = _items[index];
    if (entry.Attribute == -2)
        return _rootAttributeName;
    if (entry.Attribute >= 0) {
        AString path = GetPath((unsigned)entry.Parent);
        path += ':';
        path += UnicodeStringToMultiByte(
            (*_attributes[(unsigned)entry.Node])[(unsigned)entry.Attribute].name, CP_UTF8);
        if (entry.AttributeSuffix) {
            path += '#';
            path.Add_UInt32(entry.AttributeSuffix);
        }
        return path;
    }
#endif
    unsigned len = 0;
    const unsigned indexMem = index;
    for (;;) {
        const CItem &item = _items[index];
        unsigned i;
        GetItemName(index, i);
        len += i + 1;
        index = (unsigned)item.Parent;
        if (item.Parent < 0)
            break;
    }
    len--;

    AString path;
    char *dest = path.GetBuf_SetEnd(len) + len;
    index = indexMem;
    for (;;) {
        const CItem &item = _items[index];
        unsigned i;
        const Byte *p = GetItemName(index, i);
        dest -= i;
        memcpy(dest, p, i);
        index = (unsigned)item.Parent;
        if (item.Parent < 0)
            break;
        *(--dest) = CHAR_PATH_SEPARATOR;
    }
    return path;
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback)) {
    COM_TRY_BEGIN {
        Close();
        _limitedInStream->SetStream(stream);
        HRESULT res;
        try {
            _openCallback = callback;
            res = Open2(stream);
        } catch (...) {
            Close();
            throw;
        }
        if (res != S_OK) {
            Close();
            return res;
        }
        _stream = stream;
        _openCallback = NULL;
    }
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::Close()) {
#ifdef SEVENZIP_LINUX_SQUASHFS
    sevenzip_squashfs_decoder_destroy(_linuxDecoder);
    _linuxDecoder = NULL;
    sevenzip_squashfs_metadata_destroy(_linuxMetadata);
    _linuxMetadata = NULL;
    sevenzip_squashfs_file_destroy(_linuxFile);
    _linuxFile = NULL;
    _nodeReferences.Clear();
    _nodeRecordSizes.Clear();
    _incompleteBlocklists.clear();
    _attributes.clear();
    _attributeSets.clear();
    _incompleteAttributes.clear();
    _unavailableIds.clear();
    _incompleteDirectories.clear();
    _inodeReferences.clear();
    _inodePositions.clear();
    _inodeNodes.clear();
    _inodeBlockIndexes.clear();
    _unavailableInodeBlocks.clear();
    _badInodePositions.clear();
    _itemNames.Clear();
    _directoryDataEnd = 0;
    _metadataReadEnd = 0;
    _inodeTableIncomplete = false;
    _inodeInventoryScanned = false;
    _inodeMappingLimitations = false;
    _ownershipLimitations = false;
    _fragmentLimitations = false;
    _indexLimitations = false;
    _fragmentIndexes.clear();
    _loadedIds.clear();
    _loadedNodes.Clear();
    _rootAttributeName.Empty();
#endif
    _openCodePage = CP_UTF8;
    _sizeCalculated = 0;
    _openCallback = NULL;
    _directoryCallbackResult = S_OK;

    _limitedInStream->ReleaseStream();
    _stream.Release();

    _items.Clear();
    _nodes.Clear();
    _nodesPos.Clear();
    _activeDirs.Clear();
    _frags.Clear();
    _inodesData.Clear();
    _dirs.Clear();

    _uids.Free();
    _gids.Free();

    _cachedBlock.Free();
    ClearCache();

    return S_OK;
}

UInt64 CHandler::GetItemSize(unsigned index) const {
    const CItem &item = _items[index];
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (item.Node < 0)
        return 0;
    if (item.Attribute >= 0)
        return (*_attributes[(unsigned)item.Node])[(unsigned)item.Attribute].value.size();
#endif
    return _nodes[item.Node].GetSize();
}
bool CHandler::ItemIsDir(unsigned index) const {
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (_items[index].Node < 0)
        return _items[index].UnavailableType == kType_DIR;
    if (_items[index].Attribute >= 0)
        return false;
#endif
    return _nodes[_items[index].Node].IsDir();
}

#ifdef SEVENZIP_LINUX_SQUASHFS
Z7_COM7F_IMF(CHandler::GetParent(UInt32 index, UInt32 *parent, UInt32 *type)) {
    if (index >= _items.Size())
        return E_INVALIDARG;
    *parent = (UInt32)_items[index].Parent;
    *type = _items[index].Attribute >= 0 ? NParentType::kAltStream : NParentType::kDir;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetNumRawProps(UInt32 *count)) {
    *count = 0;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetRawPropInfo(UInt32, BSTR *name, PROPID *id)) {
    *name = NULL;
    *id = 0;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetRawProp(UInt32, PROPID, const void **data, UInt32 *size, UInt32 *type)) {
    *data = NULL;
    *size = 0;
    *type = 0;
    return S_OK;
}
#endif

bool CHandler::GetPackSize(unsigned index, UInt64 &totalPack, bool fillOffsets) {
    totalPack = 0;
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (_items[index].Node < 0)
        return false;
    if (_items[index].Attribute >= 0) {
        totalPack = GetItemSize(index);
        return true;
    }
#endif
    const CItem &item = _items[index];
    const CNode &node = _nodes[item.Node];
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (!_h.IsOldVersion() && (node.Type == kType_FILE || node.Type == kType_FILE + 7) &&
        node.FileSize != 0) {
        const HRESULT result = GetLinuxPackSize((unsigned)item.Node, totalPack);
        if (FAILED(result))
            _linuxReadResult = result;
        return result == S_OK;
    }
#endif
    const UInt32 ptr = _nodesPos[item.Node];
    const Byte *p = _inodesData.Data + ptr;
    const bool be = _h.be;

    const UInt32 type = node.Type;
    UInt32 offset;
    if (node.IsLink() || node.FileSize == 0) {
        totalPack = node.FileSize;
        return true;
    }

    const UInt32 numBlocks = (UInt32)node.GetNumBlocks(_h);

    if (fillOffsets) {
        _blockOffsets.Clear();
        _blockCompressed.Clear();
        _blockOffsets.Add(totalPack);
    }

    if (_h.Major <= 1) {
        offset = 15;
        p += offset;

        for (UInt32 i = 0; i < numBlocks; i++) {
            UInt32 t = Get16(p + i * 2);
            if (fillOffsets)
                _blockCompressed.Add((t & kNotCompressedBit16) == 0);
            if (t != kNotCompressedBit16)
                t &= ~kNotCompressedBit16;
            totalPack += t;
            if (fillOffsets)
                _blockOffsets.Add(totalPack);
        }
    } else {
        if (_h.Major <= 2)
            offset = 24;
        else if (type == kType_FILE)
            offset = 32;
        else if (type == kType_FILE + 7)
            offset = (_h.Major <= 3 ? 40 : 56);
        else
            return false;

        p += offset;

        for (UInt64 i = 0; i < numBlocks; i++) {
            UInt32 t = Get32(p + i * 4);
            if (fillOffsets)
                _blockCompressed.Add(IS_COMPRESSED_BLOCK(t));
            UInt32 size = GET_COMPRESSED_BLOCK_SIZE(t);
            if (size > _h.BlockSize || (_h.Major == 4 && size == 0 && t != 0))
                return false;
            totalPack += size;
            if (fillOffsets)
                _blockOffsets.Add(totalPack);
        }

        if (node.ThereAreFrags()) {
#ifdef SEVENZIP_LINUX_SQUASHFS
            if (FragmentUnavailable(node)) {
                if (!fillOffsets)
                    return false;
            } else
#endif
            {
                if (node.Frag >= (UInt32)_frags.Size())
                    return false;
                const CFrag &frag = _frags[node.Frag];
                const UInt32 size = GET_COMPRESSED_BLOCK_SIZE(frag.Size);
                if (size > _h.BlockSize || (_h.Major == 4 && size == 0) ||
                    frag.StartBlock > _h.Size || size > _h.Size - frag.StartBlock)
                    return false;
                if (node.Offset == 0) {
                    totalPack += size;
                }
            }
        }
    }
    if (node.StartBlock > _h.Size || (_blockOffsets.Size() != 0 && fillOffsets &&
                                      _blockOffsets.Back() > _h.Size - node.StartBlock))
        return false;
    return true;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems)) {
    *numItems = _items.Size();
    return S_OK;
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    NWindows::NCOM::CPropVariant prop;
    switch (propID) {
#ifdef SEVENZIP_LINUX_SQUASHFS
    case kpidReadOnly:
        prop = true;
        break;
    case kpidReadLimitations: {
        AString reasons;
        if (_indexLimitations)
            reasons = "Some SquashFS metadata indexes are unavailable. File data, ownership or "
                      "extended attributes may be incomplete.";
        if (!_incompleteAttributes.empty()) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += kAttributeLimitations;
        }
        if (_ownershipLimitations) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += kOwnershipLimitations;
        }
        if (_fragmentLimitations) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += kFragmentLimitations;
        }
        if (!_incompleteDirectories.empty()) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "Some SquashFS directories could not be read completely. Files or names may "
                       "be missing. Readable files remain accessible.";
        }
        if (!_incompleteBlocklists.empty()) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "Some SquashFS file block lists are unavailable. Only readable portions "
                       "of affected files can be copied; incomplete copies report an error.";
        }
        if (_inodeMappingLimitations) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "Some SquashFS directory references are unavailable. Files recovered by "
                       "inode number may be misidentified or outdated.";
        }
        if (_inodeTableIncomplete) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "Some SquashFS inode records are unavailable. Files or metadata may be "
                       "missing. Inode identities are unverified.";
        }
        if (!reasons.IsEmpty())
            prop = reasons;
        break;
    }
    case kpidIsTree:
        prop = true;
        break;
    case kpidIsAltStream: {
        bool found = false;
        FOR_VECTOR(i, _items) if (_items[i].Attribute >= 0) {
            found = true;
            break;
        }
        prop = found;
        break;
    }
#endif
    case kpidINode:
        prop = (_h.Major == 4);
        break;
    case kpidMethod: {
        char sz[16];
        const char *s;
        if (_noPropsLZMA)
            s = "LZMA Spec";
        else if (_h.SeveralMethods)
            s = "LZMA ZLIB";
        else {
            s = NULL;
            if (_h.Method < Z7_ARRAY_SIZE(k_Methods))
                s = k_Methods[_h.Method];
            if (!s) {
                ConvertUInt32ToString(_h.Method, sz);
                s = sz;
            }
        }
        prop = s;
        break;
    }
    case kpidFileSystem: {
        AString res("SquashFS");
        if (_h.SeveralMethods)
            res += "-LZMA";
        res.Add_Space();
        res.Add_UInt32(_h.Major);
        res.Add_Dot();
        res.Add_UInt32(_h.Minor);
        prop = res;
        break;
    }
    case kpidClusterSize:
        prop = _h.BlockSize;
        break;
    case kpidBigEndian:
        prop = _h.be;
        break;
    case kpidCTime:
        if (_h.CTime != 0)
            PropVariant_SetFrom_UnixTime(prop, _h.CTime);
        break;
    case kpidCharacts:
        FLAGS_TO_PROP(k_Flags, _h.Flags, prop);
        break;
    // case kpidNumBlocks: prop = _h.NumFrags; break;
    case kpidPhySize:
        prop = _sizeCalculated;
        break;
    case kpidHeadersSize:
        if (_sizeCalculated >= _h.InodeTable)
            prop = _sizeCalculated - _h.InodeTable;
        break;

    case kpidCodePage: {
        char sz[16];
        const char *name = NULL;
        switch (_openCodePage) {
        case CP_OEMCP:
            name = "OEM";
            break;
        case CP_UTF8:
            name = "UTF-8";
            break;
        }
        if (!name) {
            ConvertUInt32ToString(_openCodePage, sz);
            name = sz;
        }
        prop = name;
        break;
    }
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    if (index >= _items.Size())
        return E_INVALIDARG;
    NWindows::NCOM::CPropVariant prop;
    const CItem &item = _items[index];
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (item.Node < 0) {
        switch (propID) {
        case kpidPath:
        case kpidName: {
            AString path = GetPath(index);
            if (propID == kpidName)
                path = AString(path.Ptr(path.ReverseFind_PathSepar() + 1));
            prop = MultiByteToUnicodeString(path, _openCodePage);
            break;
        }
        case kpidIsDir:
            prop = ItemIsDir(index);
            break;
        case kpidIsAltStream:
            prop = false;
            break;
        case kpidError:
            prop = "The SquashFS inode is unavailable. File data and metadata could not be read.";
            break;
        }
        prop.Detach(value);
        return S_OK;
    }
#endif
    const CNode &node = _nodes[item.Node];
    const bool isDir = ItemIsDir(index);
    const bool be = _h.be;

#ifdef SEVENZIP_LINUX_SQUASHFS
    if (item.Attribute >= 0) {
        switch (propID) {
        case kpidINode:
        case kpidSymLink:
            prop.Detach(value);
            return S_OK;
        case kpidStreamId:
            prop = ((UInt64)1 << 63) | index;
            prop.Detach(value);
            return S_OK;
        case kpidLinks:
            prop = (UInt32)1;
            prop.Detach(value);
            return S_OK;
        case kpidPosixAttrib:
            prop = (UInt32)(MY_LIN_S_IFREG | 0444);
            prop.Detach(value);
            return S_OK;
        }
    }
#endif

    switch (propID) {
#ifdef SEVENZIP_LINUX_SQUASHFS
    case kpidError: {
        AString reasons;
        if (_incompleteDirectories.count(item.Node))
            reasons = "The SquashFS directory could not be read completely.";
        if (InodeIdentityUnavailable(item)) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "The SquashFS inode-number mapping is unverified.";
        }
        if (item.Attribute < 0 && _incompleteAttributes.count(node.Xattr)) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "Some SquashFS extended attributes are unavailable.";
        }
        if (OwnershipUnavailable(node)) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "SquashFS user or group IDs are unavailable.";
        }
        if (item.Attribute < 0 && FragmentUnavailable(node)) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons +=
                "The SquashFS file fragment is unavailable. Only the readable prefix is available.";
        }
        if (item.Attribute < 0 && _incompleteBlocklists.count((unsigned)item.Node)) {
            if (!reasons.IsEmpty())
                reasons += '\n';
            reasons += "Some SquashFS file block-list metadata is unavailable.";
        }
        if (!reasons.IsEmpty())
            prop = reasons;
        break;
    }
    case kpidIsAltStream:
        prop = item.Attribute >= 0;
        break;
    case kpidName: {
        AString path = GetPath(index);
        UString name;
        if (item.Attribute >= 0) {
            name = (*_attributes[(unsigned)item.Node])[(unsigned)item.Attribute].name;
            if (item.AttributeSuffix) {
                name += L'#';
                name.Add_UInt32(item.AttributeSuffix);
            }
        } else
            MultiByteToUnicodeString2(name, AString(path.Ptr(path.ReverseFind_PathSepar() + 1)),
                                      _openCodePage);
        prop = name;
        break;
    }
#endif
    case kpidStreamId:
    case kpidINode:
#ifdef SEVENZIP_LINUX_SQUASHFS
        if (InodeIdentityUnavailable(item) || _inodeTableIncomplete)
            break;
#endif
        if (_h.Major == 4)
            prop = node.Number;
        break;
    case kpidLinks:
        if (_h.Major == 4)
            prop = node.NumLinks;
        break;
    case kpidSymLink:
        if (node.IsLink() && node.FileSize != 0) {
            AString target;
            target.SetFrom_CalcLen((const char *)(const Byte *)_inodesData.Data +
                                       _nodesPos[item.Node] + _h.GetSymLinkOffset(),
                                   (unsigned)node.FileSize);
            UString text;
            if (_openCodePage != CP_UTF8 || !ConvertUTF8ToUnicode(target, text))
                MultiByteToUnicodeString2(text, target, _openCodePage);
            prop = text;
        }
        break;
    case kpidPath: {
        AString path(GetPath(index));
        UString s;
        if (_openCodePage == CP_UTF8)
            ConvertUTF8ToUnicode(path, s);
        else
            MultiByteToUnicodeString2(s, path, _openCodePage);
        prop = s;
        break;
    }
    case kpidIsDir:
        prop = isDir;
        break;
    // case kpidOffset: if (!node.IsLink()) prop = (UInt64)node.StartBlock; break;
    case kpidSize:
        if (!isDir)
            prop = GetItemSize(index);
        break;
    case kpidPackSize:
        if (!isDir) {
            UInt64 size;
#ifdef SEVENZIP_LINUX_SQUASHFS
            _linuxReadResult = S_OK;
#endif
            if (GetPackSize(index, size, false))
                prop = size;
#ifdef SEVENZIP_LINUX_SQUASHFS
            if (FAILED(_linuxReadResult))
                return _linuxReadResult;
#endif
        }
        break;
    case kpidMTime: {
        UInt32 offset = 0;
        switch (_h.Major) {
        case 1:
            if (node.Type == kType_FILE)
                offset = 3;
            else if (node.Type == kType_DIR)
                offset = 7;
            break;
        case 2:
            if (node.Type == kType_FILE)
                offset = 4;
            else if (node.Type == kType_DIR)
                offset = 8;
            else if (node.Type == kType_DIR + 7)
                offset = 9;
            break;
        case 3:
            offset = 4;
            break;
        case 4:
            offset = 8;
            break;
        }
        if (offset != 0) {
            const Byte *p = _inodesData.Data + _nodesPos[item.Node] + offset;
            PropVariant_SetFrom_UnixTime(prop, Get32(p));
        }
        break;
    }
    case kpidPosixAttrib: {
        if (node.Type != 0 && node.Type < Z7_ARRAY_SIZE(k_TypeToMode))
            prop = (UInt32)(node.Mode & 0xFFF) | k_TypeToMode[node.Type];
        break;
    }
    case kpidUserId:
    case kpidGroupId: {
        UInt32 id = node.Uid;
        const CByteBuffer *ids = &_uids;
        if (propID == kpidGroupId) {
            id = node.Gid;
            if (_h.Major < 4) {
                if (id == _h.GetSpecGuidIndex())
                    id = node.Uid;
                else
                    ids = &_gids;
            }
        }
        const UInt32 offset = (UInt32)id * 4;
#ifdef SEVENZIP_LINUX_SQUASHFS
        if (!_h.IsOldVersion() && _unavailableIds.count(id))
            break;
#endif
        if (offset < ids->Size())
            prop = (UInt32)Get32(*ids + offset);
        break;
    }
        /*
        case kpidLinks:
          if (_h.Major >= 3 && node.Type != kType_FILE)
            prop = node.NumLinks;
          break;
        */
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

class CSquashfsInStream : public CCachedInStream {
    HRESULT ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize) Z7_override;

  public:
    CHandler *Handler;
    CMyComPtr<IInArchive> Owner;
};

HRESULT CSquashfsInStream::ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize) {
    return Handler->ReadBlock(blockIndex, dest, blockSize);
}

HRESULT CHandler::ReadBlock(UInt64 blockIndex, Byte *dest, size_t blockSize) {
    const CNode &node = _nodes[_nodeIndex];
    UInt64 blockOffset;
    UInt32 packBlockSize;
    UInt32 offsetInBlock = 0;
    bool compressed;
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (_linuxFile && blockIndex < node.GetNumBlocks(_h)) {
        uint64_t start;
        unsigned size;
        RINOK(ReadLinuxFileBlock(_linuxFile, blockIndex, &start, &size))
        blockOffset = start;
        packBlockSize = GET_COMPRESSED_BLOCK_SIZE(size);
        compressed = IS_COMPRESSED_BLOCK(size);
    } else
#endif
        if (blockIndex < _blockCompressed.Size()) {
        compressed = _blockCompressed[(unsigned)blockIndex];
        blockOffset = _blockOffsets[(unsigned)blockIndex];
        packBlockSize = (UInt32)(_blockOffsets[(unsigned)blockIndex + 1] - blockOffset);
        blockOffset += node.StartBlock;
    } else {
        if (!node.ThereAreFrags())
            return S_FALSE;
        if (node.Frag >= _frags.Size())
            return S_FALSE;
        const CFrag &frag = _frags[node.Frag];
#ifdef SEVENZIP_LINUX_SQUASHFS
        if (!frag.Available)
            return S_FALSE;
#endif
        offsetInBlock = node.Offset;
        if (offsetInBlock > _h.BlockSize)
            return S_FALSE;
        blockOffset = frag.StartBlock;
        packBlockSize = GET_COMPRESSED_BLOCK_SIZE(frag.Size);
        compressed = IS_COMPRESSED_BLOCK(frag.Size);
    }

    if (packBlockSize > _h.BlockSize || blockOffset > _h.Size ||
        packBlockSize > _h.Size - blockOffset)
        return S_FALSE;

    if (packBlockSize == 0) {
        // sparse file ???
        memset(dest, 0, blockSize);
        return S_OK;
    }

    if (blockOffset != _cachedBlockStartPos || packBlockSize != _cachedPackBlockSize) {
        ClearCache();
        RINOK(Seek2(blockOffset))
        _limitedInStream->Init(packBlockSize);

        if (compressed) {
            _outStream->Init((Byte *)_cachedBlock, _h.BlockSize);
            bool outBufWasWritten = false;
            UInt32 outBufWasWrittenSize = 0;
            HRESULT res = Decompress(_outStream, _cachedBlock, &outBufWasWritten,
                                     &outBufWasWrittenSize, packBlockSize, _h.BlockSize);
            RINOK(res)
            if (outBufWasWritten)
                _cachedUnpackBlockSize = outBufWasWrittenSize;
            else
                _cachedUnpackBlockSize = (UInt32)_outStream->GetPos();
        } else {
            if (packBlockSize > _h.BlockSize)
                return S_FALSE;
            RINOK(ReadStream_FALSE(_limitedInStream, _cachedBlock, packBlockSize))
            _cachedUnpackBlockSize = packBlockSize;
        }
        _cachedBlockStartPos = blockOffset;
        _cachedPackBlockSize = packBlockSize;
        _processedPackSize += packBlockSize;
    }

    if (_cachedUnpackBlockSize < offsetInBlock ||
        _cachedUnpackBlockSize - offsetInBlock < blockSize)
        return S_FALSE;
    if (blockSize != 0)
        memcpy(dest, _cachedBlock + offsetInBlock, blockSize);
    return S_OK;
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems, Int32 testMode,
                               IArchiveExtractCallback *extractCallback)) {
    COM_TRY_BEGIN
    const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
    if (allFilesMode)
        numItems = _items.Size();
    if (numItems == 0)
        return S_OK;
    UInt64 totalSize = 0;
    UInt32 i;
    for (i = 0; i < numItems; i++) {
        const unsigned index = allFilesMode ? i : indices[i];
        if (index >= _items.Size() || UINT64_MAX - totalSize < GetItemSize(index))
            return E_INVALIDARG;
        totalSize += GetItemSize(index);
    }
    RINOK(extractCallback->SetTotal(totalSize))

    UInt64 totalPackSize;
    totalSize = totalPackSize = 0;

    CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
    lps->Init(extractCallback, false);
    CMyComPtr2_Create<ICompressCoder, NCompress::CCopyCoder> copyCoder;

    for (i = 0;; i++) {
        lps->InSize = totalPackSize;
        lps->OutSize = totalSize;
        RINOK(lps->SetCur())
        if (i >= numItems)
            break;

        int res;
        {
            CMyComPtr<ISequentialOutStream> outStream;
            const Int32 askMode =
                testMode ? NExtract::NAskMode::kTest : NExtract::NAskMode::kExtract;
            const UInt32 index = allFilesMode ? i : indices[i];
            RINOK(extractCallback->GetStream(index, &outStream, askMode))
            // const Byte *p = _data + item.Offset;

            if (ItemIsDir(index)) {
                RINOK(extractCallback->PrepareOperation(askMode))
#ifdef SEVENZIP_LINUX_SQUASHFS
                const Int32 result = _items[index].Node < 0 ||
                                             InodeIdentityUnavailable(_items[index]) ||
                                             _incompleteDirectories.count(_items[index].Node)
                                         ? NExtract::NOperationResult::kDataError
                                         : NExtract::NOperationResult::kOK;
                RINOK(extractCallback->SetOperationResult(result))
#else
                RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
#endif
                continue;
            }
            const UInt64 unpackSize = GetItemSize(index);
            totalSize += unpackSize;
            UInt64 packSize;
#ifdef SEVENZIP_LINUX_SQUASHFS
            if (_h.IsOldVersion())
#endif
                if (GetPackSize(index, packSize, false))
                    totalPackSize += packSize;

            if (!testMode && !outStream)
                continue;
            RINOK(extractCallback->PrepareOperation(askMode))

            res = NExtract::NOperationResult::kDataError;
            {
                CMyComPtr<ISequentialInStream> inSeqStream;
                CHandler *reader = NULL;
                HRESULT hres = CreateReadStream(index, &inSeqStream, &reader);
                if (hres == E_NOTIMPL) {
                    res = NExtract::NOperationResult::kUnsupportedMethod;
                } else if (hres != S_FALSE) {
                    RINOK(hres)
                    if (!inSeqStream)
                        return E_FAIL;
                    {
                        hres = copyCoder.Interface()->Code(inSeqStream, outStream, NULL, NULL, lps);
#ifdef SEVENZIP_LINUX_SQUASHFS
                        if (!_h.IsOldVersion())
                            totalPackSize +=
                                reader ? reader->_processedPackSize : copyCoder->TotalSize;
#endif
                        if (hres == S_OK) {
                            if (copyCoder->TotalSize == unpackSize)
                                res = NExtract::NOperationResult::kOK;
                        } else if (hres == E_NOTIMPL) {
                            res = NExtract::NOperationResult::kUnsupportedMethod;
                        } else if (hres != S_FALSE) {
                            RINOK(hres)
                        }
                    }
                }
            }
        }
#ifdef SEVENZIP_LINUX_SQUASHFS
        if (InodeIdentityUnavailable(_items[allFilesMode ? i : indices[i]]) &&
            res == NExtract::NOperationResult::kOK)
            res = NExtract::NOperationResult::kDataError;
#endif
        RINOK(extractCallback->SetOperationResult(res))
    }

    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream)) {
    return CreateReadStream(index, stream);
}

HRESULT CHandler::CreateReadStream(UInt32 index, ISequentialInStream **stream,
                                   CHandler **readerResult) {
    COM_TRY_BEGIN
    *stream = NULL;
    if (readerResult)
        *readerResult = NULL;
    if (index >= _items.Size())
        return E_INVALIDARG;

    const CItem &item = _items[index];
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (item.Node < 0)
        return S_FALSE;
    if (item.Attribute >= 0) {
        const auto &value = (*_attributes[(unsigned)item.Node])[(unsigned)item.Attribute].value;
        Create_BufInStream_WithNewBuffer(value.data(), value.size(), stream);
        return S_OK;
    }
#endif
    const CNode &node = _nodes[item.Node];

    if (node.IsDir())
        return E_FAIL;

    const Byte *p = _inodesData.Data + _nodesPos[item.Node];

    if (node.FileSize == 0 || node.IsLink()) {
        if (node.IsLink())
            Create_BufInStream_WithNewBuffer(p + _h.GetSymLinkOffset(), (size_t)node.FileSize,
                                             stream);
        else
            Create_BufInStream_WithNewBuffer(NULL, 0, stream);
        return S_OK;
    }

    UInt64 packSize;
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (_h.IsOldVersion())
#endif
        if (!GetPackSize(index, packSize, true))
            return S_FALSE;

    CSquashfsInStream *streamSpec = new CSquashfsInStream;
    CMyComPtr<IInStream> streamTemp = streamSpec;
    // A returned stream owns only its inode, mapping and decoder state.  Other
    // GetStream calls and Close/reopen must not change a reader already in use.
    CHandler *reader = new CHandler;
    streamSpec->Owner = reader;
    streamSpec->Handler = reader;
    reader->_h = _h;
    reader->_noPropsLZMA = _noPropsLZMA;
    reader->_needCheckLzma = _needCheckLzma;
    reader->_nodes.Add(node);
    reader->_nodeIndex = 0;
#ifdef SEVENZIP_LINUX_SQUASHFS
    if (_h.IsOldVersion())
#endif
    {
        reader->_blockOffsets = _blockOffsets;
        reader->_blockCompressed = _blockCompressed;
    }
    if (node.ThereAreFrags()) {
        if (node.Frag < _frags.Size())
            reader->_frags.Add(_frags[node.Frag]);
#ifdef SEVENZIP_LINUX_SQUASHFS
        else {
            CFrag missing = {};
            missing.Available = false;
            reader->_frags.Add(missing);
        }
#endif
        reader->_nodes[0].Frag = 0;
    }
    reader->_stream = _stream;
    reader->_limitedInStream->SetStream(_stream);
#ifdef SEVENZIP_LINUX_SQUASHFS
    RINOK(reader->InitLinuxDecoder())
    if (!_h.IsOldVersion()) {
        reader->_nodeReferences.Add(_nodeReferences[(unsigned)item.Node]);
        RINOK(reader->CreateLinuxFile(0, &reader->_linuxFile))
    }
#endif
    reader->_cachedBlock.Alloc(_h.BlockSize);
    reader->ClearCache();
    unsigned cacheSizeLog = 22;
    if (cacheSizeLog <= _h.BlockSizeLog)
        cacheSizeLog = _h.BlockSizeLog + 1;
    if (!streamSpec->Alloc(_h.BlockSizeLog, cacheSizeLog - _h.BlockSizeLog))
        return E_OUTOFMEMORY;
    streamSpec->Init(node.FileSize);
    // The stream owns this reader; extraction may inspect its counters only
    // while keeping the returned stream alive.
    if (readerResult)
        *readerResult = reader;
    *stream = streamTemp.Detach();

    return S_OK;

    COM_TRY_END
}

#if defined(SEVENZIP_LINUX_SQUASHFS) || !defined(Z7_EXTERNAL_CODECS)
static const Byte k_Signature[] = {4, 'h', 's', 'q', 's', 4, 's', 'q', 's', 'h',
                                   4, 's', 'h', 's', 'q', 4, 'q', 's', 'h', 's'};

REGISTER_ARC_I("SquashFS", "squashfs", NULL, 0xD2, k_Signature, 0, NArcInfoFlags::kMultiSignature,
               NULL)
#endif

}
}

// Extracted from the corresponding 7-Zip handler; retained LGPL notices apply.
#include "StdAfx.h"

#include "../../../C/CpuArch.h"
#include "ExtHeader.h"
#define EXT_NODE_SIZE_MIN 128
#define EXT4_GOOD_OLD_REV 0
#define EXT4_FEATURE_INCOMPAT_FILETYPE (1 << 1)
#define EXT4_FEATURE_INCOMPAT_64BIT (1 << 7)
#define EXT4_GOOD_OLD_INODE_SIZE 128
#define k_INODE_GOOD_OLD_FIRST 11
#define LE_16(offs, dest) dest = GetUi16(p + (offs));
#define LE_32(offs, dest) dest = GetUi32(p + (offs));
#define LE_64(offs, dest) dest = GetUi64(p + (offs));
#define HI_32(offs, dest) dest |= (((UInt64)GetUi32(p + (offs))) << 32);
#define Get32(p) GetUi32(p)

namespace NArchive {
namespace NExt {
bool CHeader::Parse(const Byte *p) {
    if (GetUi16(p + 0x38) != 0xEF53)
        return false;

    LE_32(0x18, BlockBits)
    LE_32(0x1C, ClusterBits)

    const bool bigalloc = (Get32(p + 0x64) & 0x200) != 0;
    if (bigalloc ? ClusterBits < BlockBits || ClusterBits > 28 - 10
                 : ClusterBits != 0 && BlockBits != ClusterBits)
        return false;

    if (BlockBits > 16 - 10)
        return false;
    BlockBits += 10;

    LE_32(0x00, NumInodes)
    LE_32(0x04, NumBlocks)
    // LE_32 (0x08, NumBlocksSuper);
    LE_32(0x0C, NumFreeBlocks)
    LE_32(0x10, NumFreeInodes)

    if (NumInodes < 2)
        return false;

    LE_32(0x14, FirstDataBlock)
    if (FirstDataBlock != (unsigned)(BlockBits == 10 ? 1 : 0) &&
        !(bigalloc && BlockBits == 10 && FirstDataBlock == 0))
        return false;

    LE_32(0x20, BlocksPerGroup)
    LE_32(0x24, ClustersPerGroup)

    if (bigalloc ? (UInt64)ClustersPerGroup << (ClusterBits + 10 - BlockBits) != BlocksPerGroup
                 : BlocksPerGroup != ClustersPerGroup)
        return false;
    if (BlocksPerGroup == 0)
        return false;
    if (BlocksPerGroup != ((UInt32)1 << (BlockBits + 3))) {
        // it's allowed in ext2
        // return false;
    }

    LE_32(0x28, InodesPerGroup)

    if (InodesPerGroup < 1 || InodesPerGroup > NumInodes)
        return false;

    LE_32(0x2C, MountTime)
    LE_32(0x30, WriteTime)

    // LE_16 (0x34, NumMounts);
    // LE_16 (0x36, NumMountsMax);

    LE_16(0x3A, State)
    LE_32(0xE8, LastOrphan)
    // LE_16 (0x3C, Errors);
    // LE_16 (0x3E, MinorRevLevel);

    LE_32(0x40, LastCheckTime)
    // LE_32 (0x44, CheckInterval);
    LE_32(0x48, CreatorOs)
    LE_32(0x4C, RevLevel)

    // LE_16 (0x50, DefResUid);
    // LE_16 (0x52, DefResGid);

    FirstInode = k_INODE_GOOD_OLD_FIRST;
    InodeSize = EXT4_GOOD_OLD_INODE_SIZE;

    if (!IsOldRev()) {
        LE_32(0x54, FirstInode)
        LE_16(0x58, InodeSize)
        if (FirstInode < k_INODE_GOOD_OLD_FIRST)
            return false;
        if (InodeSize > ((UInt32)1 << BlockBits) || InodeSize < EXT_NODE_SIZE_MIN ||
            GetLog(InodeSize) < 0)
            return false;
    }

    LE_16(0x5A, BlockGroupNr)
    LE_32(0x5C, FeatureCompat)
    LE_32(0x60, FeatureIncompat)
    LE_32(0x64, FeatureRoCompat)

    memcpy(Uuid, p + 0x68, sizeof(Uuid));
    memcpy(VolName, p + 0x78, sizeof(VolName));
    memcpy(LastMount, p + 0x88, sizeof(LastMount));

    // LE_32 (0xC8, BitmapAlgo);

    LE_32(0xE0, JournalInode)

    LE_16(0xFE, GdSize)

    LE_32(0x108, CTime)

    if (Is64Bit()) {
        HI_32(0x150, NumBlocks)
        // HI_32(0x154, NumBlocksSuper);
        HI_32(0x158, NumFreeBlocks)
    }

    if (NumBlocks >= (UInt64)1 << (63 - BlockBits))
        return false;

    LE_16(0x15C, MinExtraISize)
    // LE_16(0x15E, WantExtraISize);
    // LE_32(0x160, Flags);
    // LE_16(0x164, RaidStride);
    // LE_16(0x166, MmpInterval);
    // LE_64(0x168, MmpBlock);

    // LogGroupsPerFlex = p[0x174];
    // ChecksumType = p[0x175];

    LE_64(0x178, WrittenKB)

    // LE_32(0x194, ErrorCount);
    // LE_32(0x198, ErrorTime);
    // LE_32(0x19C, ErrorINode);
    // LE_32(0x1A0, ErrorBlock);

    if (NumBlocks < 1)
        return false;

    if (GetNumGroups() != GetNumGroups2())
        return false;

    return true;
}

API_FUNC_IsArc IsArc_Ext_PhySize(const Byte *p, size_t size, UInt64 *phySize);
API_FUNC_IsArc IsArc_Ext_PhySize(const Byte *p, size_t size, UInt64 *phySize) {
    if (phySize)
        *phySize = 0;
    if (size < kHeaderSize)
        return k_IsArc_Res_NEED_MORE;
    CHeader h;
    if (!h.Parse(p + kHeaderDataOffset))
        return k_IsArc_Res_NO;
    if (phySize)
        *phySize = h.GetPhySize();
    return k_IsArc_Res_YES;
}

API_FUNC_IsArc IsArc_Ext(const Byte *p, size_t size);
API_FUNC_IsArc IsArc_Ext(const Byte *p, size_t size) { return IsArc_Ext_PhySize(p, size, NULL); }

} // namespace NExt
} // namespace NArchive

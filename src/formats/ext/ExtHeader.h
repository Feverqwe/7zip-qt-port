#ifndef ZIP7_INC_EXT_HEADER_H
#define ZIP7_INC_EXT_HEADER_H
#include "7zip/Common/RegisterArc.h"
#define EXT4_GOOD_OLD_REV 0
#define EXT4_FEATURE_INCOMPAT_FILETYPE (1 << 1)
#define EXT4_FEATURE_INCOMPAT_64BIT (1 << 7)
#define EXT4_GOOD_OLD_INODE_SIZE 128
#define k_INODE_GOOD_OLD_FIRST 11
namespace NArchive {
namespace NExt {
static const UInt32 RO_COMPAT_GDT_CSUM = 1 << 4;
static const UInt32 RO_COMPAT_METADATA_CSUM = 1 << 10;
static const unsigned kHeaderSize = 2 * 1024;
static const unsigned kHeaderDataOffset = 1024;
struct CHeader {
    unsigned BlockBits;
    unsigned ClusterBits;

    UInt32 NumInodes;
    UInt64 NumBlocks;
    // UInt64 NumBlocksSuper;
    UInt64 NumFreeBlocks;
    UInt32 NumFreeInodes;
    UInt32 FirstDataBlock;

    UInt32 BlocksPerGroup;
    UInt32 ClustersPerGroup;
    UInt32 InodesPerGroup;

    UInt32 MountTime;
    UInt32 WriteTime;

    // UInt16 NumMounts;
    // UInt16 NumMountsMax;

    UInt16 State;
    UInt32 LastOrphan;
    // UInt16 Errors;
    // UInt16 MinorRevLevel;

    UInt32 LastCheckTime;
    // UInt32 CheckInterval;
    UInt32 CreatorOs;
    UInt32 RevLevel;

    // UInt16 DefResUid;
    // UInt16 DefResGid;

    UInt32 FirstInode;
    UInt16 InodeSize;
    UInt16 BlockGroupNr;
    UInt32 FeatureCompat;
    UInt32 FeatureIncompat;
    UInt32 FeatureRoCompat;
    Byte Uuid[16];
    char VolName[16];
    char LastMount[64];
    // UInt32 BitmapAlgo;

    UInt32 JournalInode;
    UInt16 GdSize; // = 64 if 64-bit();
    UInt32 CTime;
    UInt16 MinExtraISize;
    // UInt16 WantExtraISize;
    // UInt32 Flags;
    // Byte LogGroupsPerFlex;
    // Byte ChecksumType;

    UInt64 WrittenKB;

    bool IsOldRev() const { return RevLevel == EXT4_GOOD_OLD_REV; }

    UInt64 GetNumGroups() const {
        return (NumBlocks - FirstDataBlock + BlocksPerGroup - 1) / BlocksPerGroup;
    }
    UInt64 GetNumGroups2() const {
        return ((UInt64)NumInodes + InodesPerGroup - 1) / InodesPerGroup;
    }

    bool IsThereFileType() const { return (FeatureIncompat & EXT4_FEATURE_INCOMPAT_FILETYPE) != 0; }
    bool Is64Bit() const { return (FeatureIncompat & EXT4_FEATURE_INCOMPAT_64BIT) != 0; }
    bool UseGdtChecksum() const { return (FeatureRoCompat & RO_COMPAT_GDT_CSUM) != 0; }
    bool UseMetadataChecksum() const { return (FeatureRoCompat & RO_COMPAT_METADATA_CSUM) != 0; }

    UInt64 GetPhySize() const { return NumBlocks << BlockBits; }

    bool Parse(const Byte *p);
};

static int inline GetLog(UInt32 num) {
    for (unsigned i = 0; i < 32; i++)
        if (((UInt32)1 << i) == num)
            return (int)i;
    return -1;
}

API_FUNC_IsArc IsArc_Ext_PhySize(const Byte *p, size_t size, UInt64 *phySize);
API_FUNC_IsArc IsArc_Ext(const Byte *p, size_t size);
} // namespace NExt
} // namespace NArchive
#endif

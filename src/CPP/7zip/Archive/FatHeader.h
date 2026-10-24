#ifndef ZIP7_INC_FAT_HEADER_H
#define ZIP7_INC_FAT_HEADER_H
#include "../Common/RegisterArc.h"
namespace NArchive {
namespace NFat {
static const UInt32 kFatItemUsedByDirMask = (UInt32)1 << 31;
static const UInt32 kFatItemActiveFileMask = (UInt32)1 << 29;
static const UInt32 kFatItemUnknownMask = (UInt32)1 << 28;
static const UInt32 kFatItemActiveDirMask = (UInt32)1 << 30;

struct CHeader {
    Byte NumFatBits;
    Byte SectorSizeLog;
    Byte SectorsPerClusterLog;
    Byte ClusterSizeLog;
    Byte NumFats;
    Byte MediaType;

    bool VolFieldsDefined;
    bool HeadersWarning;
    bool Dirty = false;

    UInt32 FatSize;
    UInt32 BadCluster;

    UInt16 NumReservedSectors;
    UInt32 NumSectors;
    UInt32 NumFatSectors;
    UInt32 RootDirSector;
    UInt32 NumRootDirSectors;
    UInt32 DataSector;

    UInt16 SectorsPerTrack;
    UInt16 NumHeads;
    UInt32 NumHiddenSectors;

    UInt32 VolId;
    // Byte VolName[11];
    // Byte FileSys[8];
    // Byte OemName[5];

    // 32-bit FAT
    UInt16 Flags;
    UInt16 FsInfoSector;
    UInt32 RootCluster;

    bool IsFat32() const { return NumFatBits == 32; }
    UInt64 GetPhySize() const { return (UInt64)NumSectors << SectorSizeLog; }
    UInt32 SectorSize() const { return (UInt32)1 << SectorSizeLog; }
    UInt32 ClusterSize() const { return (UInt32)1 << ClusterSizeLog; }
    UInt32 ClusterToSector(UInt32 c) const {
        return DataSector + ((c - 2) << SectorsPerClusterLog);
    }
    UInt32 IsEoc(UInt32 c) const { return c > BadCluster; }
    UInt32 IsEocAndUnused(UInt32 c) const {
        return c > BadCluster && (c & kFatItemUsedByDirMask) == 0;
    }
    UInt32 IsValidCluster(UInt32 c) const { return c >= 2 && c < FatSize; }
    UInt32 SizeToSectors(UInt32 size) const { return (size + SectorSize() - 1) >> SectorSizeLog; }
    UInt32 CalcFatSizeInSectors() const {
        return SizeToSectors((FatSize * (NumFatBits / 4) + 1) / 2);
    }

    UInt32 GetFatSector() const {
        UInt32 index = (IsFat32() && (Flags & 0x80) != 0) ? (Flags & 0xF) : 0;
        if (index >= NumFats)
            index = 0;
        return NumReservedSectors + index * NumFatSectors;
    }

    UInt64 GetFilePackSize(UInt32 unpackSize) const {
        UInt64 mask = ClusterSize() - 1;
        return (unpackSize + mask) & ~mask;
    }

    UInt32 GetNumClusters(UInt32 size) const {
        return (UInt32)(((UInt64)size + ClusterSize() - 1) >> ClusterSizeLog);
    }

    bool Parse(const Byte *p);
};

API_FUNC_IsArc IsArc_Fat(const Byte *p, size_t size);
} // namespace NFat
} // namespace NArchive
#endif

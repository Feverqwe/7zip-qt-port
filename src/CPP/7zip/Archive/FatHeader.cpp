// Extracted from the corresponding 7-Zip handler; retained LGPL notices apply.
#include "StdAfx.h"

#include "../../../C/CpuArch.h"
#include "FatHeader.h"
#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get16a(p) GetUi16a(p)
#define Get32a(p) GetUi32a(p)

namespace NArchive {
namespace NFat {
bool CHeader::Parse(const Byte *p) {

    HeadersWarning = false;

    {
        {
            const unsigned num = Get16(p + 11);
            unsigned i = 9;
            unsigned m = 1 << i;
            for (;;) {
                if (m == num)
                    break;
                m <<= 1;
                if (++i > 12)
                    return false;
            }
            SectorSizeLog = (Byte)i;
        }
        {
            const unsigned num = p[13];
            unsigned i = 0;
            unsigned m = 1 << i;
            for (;;) {
                if (m == num)
                    break;
                m <<= 1;
                if (++i > 7)
                    return false;
            }
            SectorsPerClusterLog = (Byte)i;
            i += SectorSizeLog;
            ClusterSizeLog = (Byte)i;
            // (2^15 = 32 KB is safe cluster size that is suported by all system.
            // (2^16 = 64 KB is supported by some systems
            // (128 KB / 256 KB) can be created by some tools, but it is not supported by many
            // tools.
            if (i > 19) // Linux accepts 128 sectors of 4096 bytes.
                return false;
        }
    }

    NumReservedSectors = Get16(p + 14);
    if (NumReservedSectors == 0)
        return false;

    NumFats = p[16];
    if (NumFats < 1)
        return false;

    const unsigned numRootDirEntries = Get16(p + 17);
    const unsigned rootMask = (1u << (SectorSizeLog - 5)) - 1;
    if (numRootDirEntries & rootMask)
        return false;
    NumRootDirSectors = numRootDirEntries >> (SectorSizeLog - 5);
    if (Get16(p + 22) == 0 && Get32(p + 36) != 0) {
        NumFatBits = 32;
    } else {
        NumFatBits = 0;
    }

    NumSectors = Get16(p + 19);
    if (NumSectors == 0)
        NumSectors = Get32(p + 32);
    /*
    // mkfs.fat could create fat32 image with 16-bit number of sectors.
    // v23: we allow 16-bit value for number of sectors in fat32.
    else if (IsFat32())
      return false;
    */
    MediaType = p[21];
    if (MediaType != 0xF0 && MediaType < 0xF8)
        return false;
    NumFatSectors = Get16(p + 22);
    SectorsPerTrack = Get16(p + 24);
    NumHeads = Get16(p + 26);
    NumHiddenSectors = Get32(p + 28);
    RootCluster = 0;

    // memcpy(OemName, p + 3, 5);

    p += 36;
    if (IsFat32()) {
        if (NumFatSectors != 0)
            return false;
        NumFatSectors = Get32(p);
        Flags = Get16(p + 4);
        RootCluster = Get32(p + 8);
        FsInfoSector = Get16(p + 12);
        if (FsInfoSector == 0)
            FsInfoSector = 1;
        p += 28;
    }

    // DriveNumber = p[0];
    VolFieldsDefined = (p[2] == 0x29); // ExtendedBootSig
    // Linux reads the BPB volume ID independently of bootstrap code/signature.
    VolId = Get32(p + 3);
    // memcpy(VolName, p + 7, 11);
    // memcpy(FileSys, p + 18, 8);

    if (NumFatSectors == 0)
        return false;
    const UInt64 rootDirSector = NumReservedSectors + (UInt64)NumFatSectors * NumFats;
    const UInt64 dataSector = rootDirSector + NumRootDirSectors;
    if (dataSector > NumSectors)
        return false;
    RootDirSector = (UInt32)rootDirSector;
    DataSector = (UInt32)dataSector;
    const UInt32 numDataSectors = NumSectors - DataSector;
    const UInt32 numClusters = numDataSectors >> SectorsPerClusterLog;

    BadCluster = 0x0FFFFFF7;
    // v23: we support unusual (< 0xFFF5) numClusters values in fat32 systems
    if (NumFatBits != 32) {
        NumFatBits = (Byte)(numClusters < 0xFF5 ? 12 : 16);
        BadCluster &= (((UInt32)1 << NumFatBits) - 1);
    }

    const UInt64 fatCapacity = ((UInt64)NumFatSectors << (SectorSizeLog + 3)) / NumFatBits;
    UInt64 fatSize = (UInt64)numClusters + 2;
    if (fatSize > fatCapacity) {
        /* some third-party program can create such FAT image, where
           size of FAT table (NumFatSectors from headers) is smaller than
           required value that is calculated from calculated (FatSize) value.
           Another FAT unpackers probably ignore that error.
           v23.02: we also ignore that error, and
           we recalculate (FatSize) value from (NumFatSectors).
           New (FatSize) will be smaller than original "full" (FatSize) value.
           So we will have some unused clusters at the end of archive.
        */
        fatSize = fatCapacity;
        HeadersWarning = true;
    }
    const UInt64 maxEntries = IsFat32() ? (UInt64)BadCluster + 1 : (UInt64)BadCluster - 1;
    if (fatSize < 2 || fatSize > maxEntries)
        return false;
    FatSize = (UInt32)fatSize;
    return true;
}

API_FUNC_IsArc IsArc_Fat(const Byte *p, size_t size) {
    if (size < 512)
        return k_IsArc_Res_NEED_MORE;
    if (GetUi16(p + 0x1FE) != 0xAA55)
        return k_IsArc_Res_NO;
    CHeader h;
    return h.Parse(p) ? k_IsArc_Res_YES : k_IsArc_Res_NO;
}

} // namespace NFat
} // namespace NArchive

// Archive/UdfIn.cpp

#include "7zip/Archive/Udf/StdAfx.h"

// #define SHOW_DEBUG_INFO

#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#endif

#include "platform/cpu/CpuArch.h"

#include "Common/UTFConvert.h"
#include "Windows/PropVariantUtils.h"

#include "7zip/Common/LimitedStreams.h"
#include "7zip/Common/RegisterArc.h"
#include "7zip/Common/StreamObjects.h"
#include "7zip/Common/StreamUtils.h"

#include "formats/udf/UdfIn.h"

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

#define Get16(p) GetUi16(p)
#define Get32(p) GetUi32(p)
#define Get64(p) GetUi64(p)

#define G16(_offs_, dest) dest = Get16(p + (_offs_))
#define G32(_offs_, dest) dest = Get32(p + (_offs_))
#define G64(_offs_, dest) dest = Get64(p + (_offs_))

namespace NArchive {
namespace NUdf {

static const unsigned kNumPartitionsMax = 64;
static const unsigned kNumLogVolumesMax = 64;
static const unsigned kNumRecursionLevelsMax = 1 << 10;
static const unsigned kNumItemsMax = 1 << 27;
static const unsigned kNumFilesMax = 1 << 28;
static const unsigned kNumRefsMax = 1 << 28;
static const UInt32 kNumExtentsMax = (UInt32)1 << 30;
static const UInt64 kFileNameLengthTotalMax = (UInt64)1 << 33;
static const UInt64 kInlineExtentsSizeMax = (UInt64)1 << 33;

#define CRC16_INIT_VAL 0
#define CRC16_UPDATE_BYTE(crc, b)                                                                  \
    ((UInt16)(g_Crc16Table[(((crc) >> 8) ^ (b)) & 0xFF] ^ ((crc) << 8)))

#define kCrc16Poly 0x1021
static UInt16 g_Crc16Table[256];

static void Z7_FASTCALL Crc16GenerateTable(void) {
    UInt32 i;
    for (i = 0; i < 256; i++) {
        UInt32 r = (i << 8);
        for (unsigned j = 0; j < 8; j++)
            r = ((r << 1) ^ (kCrc16Poly & ((UInt32)0 - (r >> 15)))) & 0xFFFF;
        g_Crc16Table[i] = (UInt16)r;
    }
}

static UInt32 Z7_FASTCALL Crc16Calc(const void *data, size_t size) {
    UInt32 v = CRC16_INIT_VAL;
    const Byte *p = (const Byte *)data;
    const Byte *pEnd = p + size;
    for (; p != pEnd; p++)
        v = CRC16_UPDATE_BYTE(v, *p);
    return v;
}

static struct CCrc16TableInit {
    CCrc16TableInit() { Crc16GenerateTable(); }
} g_Crc16TableInit;

// ---------- ECMA Part 1 ----------

void CDString::Parse(const Byte *p, unsigned size) { Data.CopyFrom(p, size); }

static UString ParseLinuxDString(const Byte *data, unsigned size) {
    Byte output[127];
    const int length = sevenzip_udf_dstring(data, (int)size, output, (int)size - 1);
    UString result;
    if (length > 0) {
        AString text;
        text.SetFrom((const char *)output, (unsigned)length);
        ConvertUTF8ToUnicode(text, result);
    }
    return result;
}

bool CDString::GetFilename(UString &name) const {
    Byte output[255];
    const int length = sevenzip_udf_filename(Data, (int)Data.Size(), output, sizeof(output));
    if (length < 0)
        return false;
    AString text;
    text.SetFrom((const char *)output, (unsigned)length);
    return ConvertUTF8ToUnicode(text, name);
}

UString CDString32::GetString() const { return ParseLinuxDString(Data, sizeof(Data)); }

UString CDString128::GetString() const { return ParseLinuxDString(Data, sizeof(Data)); }

UString CDString::GetString() const {
    UString name;
    GetFilename(name);
    return name;
}

void CTime::Parse(const Byte *p) { memcpy(Data, p, sizeof(Data)); }

static void AddCommentChars(UString &dest, const char *s, size_t size) {
    for (size_t i = 0; i < size; i++) {
        char c = s[i];
        if (c == 0)
            break;
        if (c < 0x20)
            c = '_';
        dest += (wchar_t)c;
    }
}

void CRegId::Parse(const Byte *p) {
    Flags = p[0];
    memcpy(Id, p + 1, sizeof(Id));
    memcpy(Suffix, p + 24, sizeof(Suffix));
}

void CRegId::AddCommentTo(UString &s) const { AddCommentChars(s, Id, sizeof(Id)); }

void CRegId::AddUdfVersionTo(UString &s) const {
    // use it only for "Domain Identifier Suffix" and "UDF Identifier Suffix"
    // UDF 2.1.5.3
    // Revision in hex (3 digits)
    const Byte minor = Suffix[0];
    const Byte major = Suffix[1];
    if (major != 0 || minor != 0) {
        char temp[16];
        ConvertUInt32ToHex(major, temp);
        s += temp;
        s.Add_Dot();
        ConvertUInt32ToHex8Digits(minor, temp);
        s += &temp[8 - 2];
    }
}

// ---------- ECMA Part 3: Volume Structure ----------

void CExtent::Parse(const Byte *p) {
    /* Len shall be less than < 2^30.
       Unless otherwise specified, the length shall be an integral multiple of the logical sector
       size. If (Len == 0), no extent is specified and (Pos) shall contain 0 */
    G32(0, Len);
    G32(4, Pos);
}

// ECMA 3/7.2

struct CTag {
    UInt16 Id;
    // UInt16 Version;
    // Byte Checksum;
    // UInt16 SerialNumber;
    // UInt16 Crc;
    UInt16 CrcLen;
    // UInt32 TagLocation; // the number of the logical sector

    HRESULT Parse(const Byte *p, size_t size);
};

HRESULT CTag::Parse(const Byte *p, size_t size) {
    if (size < 16)
        return S_FALSE;
    {
        unsigned sum = 0;
        for (unsigned i = 0; i < 16; i++)
            sum = sum + p[i];
        if ((Byte)(sum - p[4]) != p[4] || p[5] != 0)
            return S_FALSE;
    }
    Id = Get16(p);
    const UInt16 Version = Get16(p + 2);
    if (Version != 2 && Version != 3)
        return S_FALSE;
    // SerialNumber = Get16(p + 6);
    const UInt32 crc = Get16(p + 8);
    CrcLen = Get16(p + 10);
    // TagLocation = Get32(p + 12);

    if (size >= 16 + (size_t)CrcLen)
        if (crc == Crc16Calc(p + 16, (size_t)CrcLen))
            return S_OK;
    return S_FALSE;
}

// ECMA 3/7.2.1

enum EDescriptorType {
    DESC_TYPE_SpoaringTable = 0, // UDF
    DESC_TYPE_PrimVol = 1,
    DESC_TYPE_AnchorVolPtr = 2,
    DESC_TYPE_VolPtr = 3,
    DESC_TYPE_ImplUseVol = 4,
    DESC_TYPE_Partition = 5,
    DESC_TYPE_LogicalVol = 6,
    DESC_TYPE_UnallocSpace = 7,
    DESC_TYPE_Terminating = 8,
    DESC_TYPE_LogicalVolIntegrity = 9,
    DESC_TYPE_FileSet = 256,
    DESC_TYPE_FileId = 257,
    DESC_TYPE_AllocationExtent = 258,
    DESC_TYPE_Indirect = 259,
    DESC_TYPE_Terminal = 260,
    DESC_TYPE_File = 261,
    DESC_TYPE_ExtendedAttrHeader = 262,
    DESC_TYPE_UnallocatedSpaceEntry = 263,
    DESC_TYPE_SpaceBitmap = 264,
    DESC_TYPE_PartitionIntegrity = 265,
    DESC_TYPE_ExtendedFile = 266
};

void CLogBlockAddr::Parse(const Byte *p) {
    G32(0, Pos);
    G16(4, PartitionRef);
}

void CShortAllocDesc::Parse(const Byte *p) {
    G32(0, Len);
    G32(4, Pos);
}

/*
void CADImpUse::Parse(const Byte *p)
{
  G16 (0, Flags);
  G32 (2, UdfUniqueId);
}
*/

void CLongAllocDesc::Parse(const Byte *p) {
    G32(0, Len);
    Location.Parse(p + 4);
    // memcpy(ImplUse, p + 10, sizeof(ImplUse));
    // adImpUse.Parse(ImplUse);
}

void CPrimeVol::Parse(const Byte *p) {
    // G32 (16, VolumeDescriptorSequenceNumber);
    G32(20, PrimaryVolumeDescriptorNumber);
    VolumeId.Parse(p + 24);
    G16(56, VolumeSequenceNumber);
    G16(58, MaximumVolumeSequenceNumber);
    // G16 (60, InterchangeLevel);
    // G16 (62, MaximumInterchangeLevel);
    // G32 (64, CharacterSetList)
    // G32 (68, MaximumCharacterSetList)
    VolumeSetId.Parse(p + 72);
    // 200 64 Descriptor Character Set charspec (1/7.2.1)
    // 264 64 Explanatory Character Set charspec (1/7.2.1)
    // VolumeAbstract.Parse(p + 328);
    // VolumeCopyrightNotice.Parse(p + 336);
    ApplicationId.Parse(p + 344);
    RecordingTime.Parse(p + 376);
    ImplId.Parse(p + 388);
    // 420 64 Implementation Use bytes
    // G32 (484, PredecessorVolumeDescriptorSequenceLocation);
    // G16 (488, Flags);
}

bool CInArchive::CheckExtent(unsigned volIndex, unsigned partitionRef, UInt32 blockPos,
                             UInt32 len) const {
    const CLogVol &vol = LogVols[volIndex];
    if (partitionRef >= vol.PartitionMaps.Size())
        return false;
    const CPartitionMap &map = vol.PartitionMaps[partitionRef];
    if (map.PartitionIndex >= Partitions.Size())
        return !map.IsVirtual && !map.IsMetadata() &&
               ((UInt64)blockPos * vol.BlockSize + len) <= FileSize;
    const CPartition &partition = Partitions[map.PartitionIndex];
    return ((UInt64)blockPos * vol.BlockSize + len) <= ((UInt64)partition.Len << SecLogSize);
}

bool CInArchive::CheckItemExtents(unsigned volIndex, const CItem &item) const {
    UInt64 remaining = item.Size;
    FOR_VECTOR(i, item.Extents) {
        if (!remaining)
            break;
        const CMyExtent &e = item.Extents[i];
        const UInt32 len = (UInt32)MyMin(remaining, (UInt64)e.GetLen());
        if (e.PartitionRef >= LogVols[volIndex].PartitionMaps.Size() ||
            e.GetType() == SHORT_ALLOC_DESC_TYPE_NextExtent ||
            (e.IsRecAndAlloc() && !CheckExtent(volIndex, e.PartitionRef, e.Pos, len)))
            return false;
        remaining -= len;
    }
    return true;
}

HRESULT CInArchive::GetPhysicalSpan(unsigned volIndex, unsigned partitionRef, UInt32 block,
                                    UInt32 length, UInt64 &offset, UInt32 &size, UInt32 blockOffset,
                                    bool checkRange) const {
    if ((UInt64)block + blockOffset > UINT32_MAX)
        return S_FALSE;
    if (partitionRef >= LogVols[volIndex].PartitionMaps.Size() ||
        (checkRange && !CheckExtent(volIndex, partitionRef, block + blockOffset, length)))
        return S_FALSE;
    const CLogVol &vol = LogVols[volIndex];
    const CPartitionMap &map = vol.PartitionMaps[partitionRef];
    UInt32 root = 0;
    if (map.PartitionIndex < Partitions.Size())
        root = Partitions[map.PartitionIndex].Pos;
    offset = ((UInt64)root << SecLogSize) + ((UInt64)block + blockOffset) * vol.BlockSize;
    size = length;
    if (map.IsVirtual) {
        uint16_t backing;
        uint32_t mapped;
        if (!map.Virtual)
            return S_FALSE;
        const int result =
            sevenzip_udf_virtual_map(map.Virtual.get(), block, blockOffset, &backing, &mapped);
        if (result)
            return result < 0 ? (HRESULT)result : S_FALSE;
        return GetPhysicalSpan(volIndex, backing, mapped, length, offset, size, 0, checkRange);
    }
    if (map.IsMetadata()) {
        if (!map.Metadata)
            return S_FALSE;
        uint16_t backing;
        uint32_t mapped;
        const int result =
            sevenzip_udf_metadata_map(map.Metadata.get(), block, blockOffset, &backing, &mapped);
        if (result)
            return result < -4095 ? (HRESULT)result : result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
        return GetPhysicalSpan(volIndex, backing, mapped, length, offset, size, 0, checkRange);
    }
    if (map.IsSparable()) {
        uint32_t physical, blocks;
        if (!map.Sparing ||
            sevenzip_udf_sparing_span(map.Sparing.get(), block + blockOffset, &physical, &blocks))
            return S_FALSE;
        size = (UInt32)MyMin((UInt64)length, (UInt64)blocks * vol.BlockSize);
        if ((UInt64)physical + ((UInt64)size + vol.BlockSize - 1) / vol.BlockSize >
            (UInt64)UINT32_MAX)
            return S_FALSE;
        offset = (UInt64)physical * vol.BlockSize;
        if (offset > FileSize || size > FileSize - offset)
            return S_FALSE;
    }
    return S_OK;
}

HRESULT CInArchive::Read(unsigned volIndex, unsigned partitionRef, UInt32 blockPos, UInt32 len,
                         Byte *buf) {
    if (!CheckExtent(volIndex, partitionRef, blockPos, len))
        return S_FALSE;
    const CLogVol &vol = LogVols[volIndex];
    UInt32 blockOffset = 0;
    while (len) {
        UInt64 offset;
        UInt32 size;
        RINOK(GetPhysicalSpan(volIndex, partitionRef, blockPos, len, offset, size, blockOffset))
        RINOK(InStream_SeekSet(_stream, offset))
        UpdatePhySize(offset + size);
        const HRESULT res = ReadStream_FALSE(_stream, buf, size);
        if (res == S_FALSE && offset + size > FileSize)
            UnexpectedEnd = true;
        RINOK(res)
        len -= size;
        buf += size;
        blockOffset += size / vol.BlockSize;
    }
    return S_OK;
}

HRESULT CInArchive::GetItemStream(unsigned volIndex, const CItem &item,
                                  ISequentialInStream **stream) {
    *stream = NULL;
    const CLogVol &vol = LogVols[volIndex];
    UInt64 size = item.Size;
    if (!item.LinuxAttributes.hidden && !(item.LinuxAttributes.mode & 0170000))
        return E_NOTIMPL;

    if (!item.CheckChunkSizes() || !CheckItemExtents(volIndex, item))
        return E_NOTIMPL;

    if (item.IsInline) {
        Create_BufInStream_WithNewBuffer(item.InlineData, stream);
        return S_OK;
    }

    CExtentsStream *extentStreamSpec = new CExtentsStream();
    CMyComPtr<ISequentialInStream> extentStream = extentStreamSpec;

    extentStreamSpec->Stream = _stream;

    UInt64 virtOffset = 0;
    FOR_VECTOR(extentIndex, item.Extents) {
        if (size == 0)
            break;
        const CMyExtent &extent = item.Extents[extentIndex];
        UInt32 len = extent.GetLen();
        if (len == 0)
            continue;
        if (size < len)
            len = (UInt32)size;

        UInt32 remaining = len, blockOffset = 0;
        while (remaining) {
            CSeekExtent se;
            UInt32 chunk = remaining;
            if (extent.IsRecAndAlloc()) {
                RINOK(GetPhysicalSpan(volIndex, extent.PartitionRef, extent.Pos, remaining, se.Phy,
                                      chunk, blockOffset))
            } else {
                se.SetAs_ZeroFill();
            }
            se.Virt = virtOffset;
            if (extentStreamSpec->Extents.IsEmpty() || se.Is_ZeroFill() ||
                extentStreamSpec->Extents.Back().Is_ZeroFill() ||
                extentStreamSpec->Extents.Back().Phy + virtOffset -
                        extentStreamSpec->Extents.Back().Virt !=
                    se.Phy)
                extentStreamSpec->Extents.Add(se);
            virtOffset += chunk;
            remaining -= chunk;
            blockOffset += chunk / vol.BlockSize;
        }

        size -= len;
    }
    if (size != 0) {
        CSeekExtent hole;
        hole.Virt = virtOffset;
        hole.SetAs_ZeroFill();
        extentStreamSpec->Extents.Add(hole);
        virtOffset += size;
    }
    CSeekExtent se;
    se.Phy = 0;
    se.Virt = virtOffset;
    extentStreamSpec->Extents.Add(se);
    extentStreamSpec->Init();
    *stream = extentStream.Detach();
    return S_OK;
}

HRESULT CInArchive::ReadLad(unsigned volIndex, const CLongAllocDesc &lad, Byte *buf) {
    return Read(volIndex, lad.Location.PartitionRef, lad.Location.Pos, lad.GetLen(), (Byte *)buf);
}

HRESULT CInArchive::ReadFromFile(unsigned volIndex, const CItem &item, CByteBuffer &buf) {
    if (item.Size >= (UInt32)1 << 30)
        return S_FALSE;
    if (item.IsInline) {
        buf = item.InlineData;
        return S_OK;
    }
    buf.Alloc((size_t)item.Size);
    size_t pos = 0;
    FOR_VECTOR(i, item.Extents) {
        if (pos == item.Size)
            break;
        const CMyExtent &e = item.Extents[i];
        const UInt32 len = (UInt32)MyMin(item.Size - pos, (UInt64)e.GetLen());
        if (!e.IsRecAndAlloc())
            return S_FALSE;
        RINOK(Read(volIndex, e.PartitionRef, e.Pos, len, (Byte *)buf + pos))
        pos += len;
    }
    return pos == item.Size ? S_OK : S_FALSE;
}

void CIcbTag::Parse(const Byte *p) {
    // G32 (0, PriorDirectNum);
    // G16 (4, StrategyType);
    // G16 (6, StrategyParam);
    // G16 (8, MaxNumOfEntries);
    FileType = p[11];
    // ParentIcb.Parse(p + 12);
    G16(18, Flags);
}

// ECMA 4/14.9 File Entry
// UDF FileEntry 2.3.6

// ECMA 4/14.17 Extended File Entry

void CItem::Parse(const Byte *p) {
    // (-1) can be stored in Uid/Gid.
    // G32 (36, Uid);
    // G32 (40, Gid);
    // G32 (44, Permissions);
    G16(48, FileLinkCount);
    // RecordFormat = p[50];
    // RecordDisplayAttr = p[51];
    // G32 (52, RecordLen);
    G64(56, Size);
    if (IsExtended) {
        // The sum of all Information Length fields for all streams of a file (including the default
        // stream). If this file has no streams, the Object Size shall be equal to the Information
        // Length. G64 (64, ObjectSize);
        p += 8;
    }
    G64(64, NumLogBlockRecorded);
    ATime.Parse(p + 72);
    MTime.Parse(p + 84);
    if (IsExtended) {
        CreateTime.Parse(p + 96);
        p += 12;
    }
    AttribTime.Parse(p + 96);
    // G32 (108, CheckPoint);
    /*
    if (IsExtended)
    {
      // Get32(p + 112); // reserved
      p += 4;
    }
    // ExtendedAttrIcb.Parse(p + 112);
    if (IsExtended)
    {
      StreamDirectoryIcb.Parse(p + 128);
      p += 16;
    }
    */

    // ImplId.Parse(p + 128);
    // G64 (160, UniqueId);
}

// ECMA 4/14.4
// UDF 2.3.4

/*
File Characteristics:
Deleted bit:
  ECMA: If set to ONE, shall mean this File Identifier Descriptor
        identifies a file that has been deleted;
  UDF:  If the space for the file or directory is deallocated,
        the implementation shall set the ICB field to zero.
    ECMA 167 4/8.6 requires that the File Identifiers of all FIDs in a directory shall be unique.
    The implementations shall follow these rules when a Deleted bit is set:
    rewrire the compression ID of the File Identifier: 8 -> 254, 16 -> 255.
*/

HRESULT CInArchive::ReadFileItem(unsigned volIndex, unsigned fsIndex, const CLongAllocDesc &lad,
                                 bool isDir, int numRecurseAllowed) {
    if (Files.Size() % 100 == 0)
        RINOK(_progress->SetCompleted(Files.Size(), _processedProgressBytes))
    if (numRecurseAllowed-- == 0)
        return S_FALSE;
    CFile &file = Files.Back();
    const CLogVol &vol = LogVols[volIndex];
    const unsigned partitionRef = lad.Location.PartitionRef;
    if (partitionRef >= vol.PartitionMaps.Size())
        return S_FALSE;
    UInt32 key;
    RINOK(GetLinuxInodeKey(volIndex, lad, key))
    CMap32 &inodeMap = LogVols[volIndex].PublicItems;
    UInt32 value;
    const UInt32 kRecursedErrorValue = (UInt32)(Int32)-1;
    if (inodeMap.Find(key, value)) {
        if (value == kRecursedErrorValue)
            return S_FALSE;
        file.ItemIndex = (int)(Int32)value;
    } else {
        value = Items.Size();
        file.ItemIndex = (int)(Int32)value;
        if (inodeMap.Set(key, kRecursedErrorValue))
            return S_FALSE;
        RINOK(ReadItem(volIndex, (int)fsIndex, lad, isDir, numRecurseAllowed, nullptr, &key))
        CItem &item = Items[value];
        if (item.IcbTag.IsDir()) {
            RINOK(ReadLinuxDirectory(volIndex, fsIndex, item, item.LinuxAllocData,
                                     (UInt32)item.LinuxAllocData.Size(), numRecurseAllowed))
            item.Size = 0;
            item.Extents.ClearAndFree();
            item.InlineData.Free();
            item.LinuxAllocData.Free();
        }
        if (!inodeMap.Set(key, value))
            return S_FALSE;
    }
    return S_OK;
}

// Negative fsIndex roles: -1 legacy metadata, -2 hidden VAT stream,
// -3 raw hidden metadata allocation list for the Linux mapper.

HRESULT CInArchive::ReadItem(unsigned volIndex, int fsIndex, const CLongAllocDesc &lad, bool isDir,
                             int numRecurseAllowed, HRESULT *progressResult,
                             const UInt32 *inodeKey) {
    (void)progressResult;
    (void)inodeKey;
    if (Items.Size() >= kNumItemsMax)
        return S_FALSE;
    CItem &item = Items.AddNew();

    const CLogVol &vol = LogVols[volIndex];

    const size_t size = vol.BlockSize;

    CByteBuffer buf(size);
    CLongAllocDesc resolved = lad;
    RINOK(ReadLinuxItem(volIndex, resolved, buf, item, fsIndex < 0, progressResult, inodeKey))
    item.Location = resolved.Location;

    CTag tag;
    const Byte *p = buf;
    tag.Id = Get16(p);
    tag.CrcLen = Get16(p + 10);

    item.IsExtended = (tag.Id == DESC_TYPE_ExtendedFile);
    if (tag.Id == DESC_TYPE_UnallocatedSpaceEntry) {
        if (isDir)
            return S_FALSE;
        item.IsUnallocated = true;
        item.IcbTag.Parse(p + 16);
        item.IsInline = item.IcbTag.GetDescriptorType() == ICB_DESC_TYPE_INLINE;
        item.Size = item.NumLogBlockRecorded = 0;
        item.FileLinkCount = (UInt16)item.LinuxAttributes.links;
        const UInt32 allocationSize = Get32(p + 36);
        if (fsIndex < 0)
            item.LinuxAllocData.CopyFrom(p + 40, allocationSize);
        return S_OK;
    }
    const size_t kExtendOffset = item.IsExtended ? 40 : 0;

    if (size < kExtendOffset + 176)
        return S_FALSE;
    if (tag.Id != DESC_TYPE_File && tag.Id != DESC_TYPE_ExtendedFile)
        return S_FALSE;

    item.IcbTag.Parse(p + 16);
    // The archive root is a directory; FID type hints are not inode type authority.
    if (isDir && !item.IcbTag.IsDir())
        return S_FALSE;

    item.Parse(p);

    _processedProgressBytes += (UInt64)item.NumLogBlockRecorded * vol.BlockSize + size;

    const UInt32 extendedAttrLen = Get32(p + 168 + kExtendOffset);
    const UInt32 allocDescriptorsLen = Get32(p + 172 + kExtendOffset);

    size_t pos = 176 + kExtendOffset;
    if (extendedAttrLen > size - pos)
        return S_FALSE;
    /*
    if (extendedAttrLen != 16)
    {
      if (extendedAttrLen < 24)
        return S_FALSE;
      CTag attrTag;
      RINOK(attrTag.Parse(p + pos, size));
      if (attrTag.Id != DESC_TYPE_ExtendedAttrHeader)
        return S_FALSE;
      // UInt32 implAttrLocation = Get32(p + pos + 16);
      // UInt32 applicationlAttrLocation = Get32(p + pos + 20);
    }
    */
    pos += extendedAttrLen;

    const int descType = item.IcbTag.GetDescriptorType();
    if (allocDescriptorsLen > size - pos)
        return S_FALSE;
    if (fsIndex < -1)
        item.LinuxAllocData.CopyFrom(p + pos, allocDescriptorsLen);
    if (fsIndex == -3) {
        item.IsInline = descType == ICB_DESC_TYPE_INLINE;
        if (item.IsInline)
            item.InlineData.CopyFrom(p + pos, allocDescriptorsLen);
        return S_OK;
    }
    const UInt32 modeType = item.LinuxAttributes.mode & 0170000;
    if (fsIndex >= 0 && (modeType == 0020000 || modeType == 0060000 || modeType == 0010000 ||
                         modeType == 0140000)) {
        // Special inode data is not a regular-file AD stream. Never open a host device.
        item.IsInline = true;
        return S_OK;
    }
    if (descType == ICB_DESC_TYPE_INLINE) {
        item.IsInline = true;
        item.InlineData.CopyFrom(p + pos, allocDescriptorsLen);
    } else {
        item.IsInline = false;
        if (descType != ICB_DESC_TYPE_SHORT && descType != ICB_DESC_TYPE_LONG)
            return S_FALSE;
        if (!item.IcbTag.IsDir()) {
            RINOK(ReadLinuxExtents(volIndex, resolved.Location.PartitionRef, item, p + pos,
                                   allocDescriptorsLen, progressResult))
        }
    }

    if (fsIndex == -2)
        return S_OK;

    if (item.IcbTag.IsDir()) {
        if (fsIndex < 0)
            return S_FALSE;

        // ReadFileItem walks children after this parser frame has returned,
        // while keeping the inode's in-progress cycle marker until completion.
        item.LinuxAllocData.CopyFrom(p + pos, allocDescriptorsLen);
    } else {
        if (item.IcbTag.FileType == ICB_FILE_TYPE_SYMLINK)
            RINOK(ReadLinuxSymlink(volIndex, item))
        if ((UInt32)item.Extents.Size() > kNumExtentsMax - _numExtents)
            return S_FALSE;
        _numExtents += item.Extents.Size();

        if (item.InlineData.Size() > kInlineExtentsSizeMax - _inlineExtentsSize)
            return S_FALSE;
        _inlineExtentsSize += item.InlineData.Size();
    }

    return S_OK;
}

HRESULT CInArchive::AddLinuxDirectoryFile(CItem &item, const Byte *name, unsigned size,
                                          bool &added) {
    added = false;
    CFile file;
    file.Id.Parse(name, size);
    UString decodedName;
    if (!file.Id.GetFilename(decodedName))
        return S_OK;
    _fileNameLengthTotal += file.Id.Data.Size();
    if (_fileNameLengthTotal > kFileNameLengthTotalMax)
        return S_FALSE;
    item.SubFiles.Add(Files.Size());
    if (Files.Size() >= kNumFilesMax)
        return S_FALSE;
    Files.Add(file);
    added = true;
    return S_OK;
}

HRESULT CInArchive::FillRefs(CFileSet &fs, unsigned fileIndex, int parent, int numRecurseAllowed) {
    if ((_numRefs & 0xFFF) == 0) {
        RINOK(_progress->SetCompleted())
    }
    if (numRecurseAllowed-- == 0)
        return S_FALSE;
    if (_numRefs >= kNumRefsMax)
        return S_FALSE;
    _numRefs++;
    CRef ref;
    ref.FileIndex = fileIndex;
    ref.Parent = parent;
    parent = (int)fs.Refs.Size();
    fs.Refs.Add(ref);
    const CItem &item = Items[Files[fileIndex].ItemIndex];
    FOR_VECTOR(i, item.SubFiles) {
        RINOK(FillRefs(fs, item.SubFiles[i], parent, numRecurseAllowed))
    }
    return S_OK;
}

API_FUNC_IsArc IsArc_Udf(const Byte *p, size_t size) {
    const int result = sevenzip_udf_probe(p, size);
    return result < 0 ? k_IsArc_Res_NEED_MORE : result ? k_IsArc_Res_YES : k_IsArc_Res_NO;
}

HRESULT CInArchive::ParseVolumeDescriptor(const Byte *buf, size_t bufSize, UInt16 id,
                                          UInt32 *linuxFlags) {
    (void)linuxFlags;
    if (id == DESC_TYPE_PrimVol) {
        CPrimeVol &pm = PrimeVols.AddNew();
        pm.Parse(buf);
        return S_OK;
    }

    if (id == DESC_TYPE_Partition) {
        if (linuxFlags) {
            FOR_VECTOR(v, LogVols)
            LogVols[v].LinuxFlags = *linuxFlags;
        }
        bool used = false;
        FOR_VECTOR(v, LogVols) {
            FOR_VECTOR(p, LogVols[v].PartitionMaps) {
                const CPartitionMap &map = LogVols[v].PartitionMaps[p];
                if ((map.Native.kind == UDF_TYPE1_MAP15 || map.IsSparable()) &&
                    map.PartitionNumber == Get16(buf + 22))
                    used = true;
            }
        }
        if (!used)
            return S_OK;
        // Partition Descriptor
        // ECMA 3/10.5
        // UDF 2.2.14
        CPartition partition;
        partition.RawDescriptor.CopyFrom(buf, 512);
        // const UInt32 volDescSeqNumer = Get32(buf + 16);
        partition.Flags = Get16(buf + 20);
        partition.Number = Get16(buf + 22);
        partition.ContentsId.Parse(buf + 24);

        // memcpy(partition.ContentsUse, buf + 56, sizeof(partition.ContentsUse));
        // ContentsUse contains Partition Header Description.
        // ECMA 4/14.3
        // UDF PartitionHeaderDescriptor 2.3.3

        partition.AccessType = Get32(buf + 184);
        partition.Pos = Get32(buf + 188);
        partition.Len = Get32(buf + 192);
        partition.ImplId.Parse(buf + 196);
        // memcpy(partition.ImplUse, buf + 228, sizeof(partition.ImplUse));

        PRF(printf("\nPartition number = %2d   pos = %d   len = %d", partition.Number,
                   partition.Pos, partition.Len));
        const HRESULT result = LoadLinuxPartitionDescriptor(Partitions.Add(partition));
        if (linuxFlags && !LogVols.IsEmpty())
            *linuxFlags = LogVols[0].LinuxFlags;
        return result;
    }

    if (id == DESC_TYPE_LogicalVol) {
        /* Logical Volume Descriptor
            ECMA 3/10.6
            UDF 2.60 2.2.4 */

        if (LogVols.Size() >= kNumLogVolumesMax)
            return S_FALSE;
        CLogVol &vol = LogVols.AddNew();

        vol.Id.Parse(buf + 84);
        vol.BlockSize = Get32(buf + 212);
        // Linux uses the block size chosen during anchor discovery.
        vol.BlockSize = (UInt32)1 << SecLogSize;
        /*
        if (vol.BlockSize < 512 || vol.BlockSize > ((UInt32)1 << 30))
          return S_FALSE;
        */

        vol.DomainId.Parse(buf + 216);

        // ECMA 4/3.1
        // UDF 2.2.4.4 LogicalVolumeContentsUse
        /* the extent in which the first File Set Descriptor Sequence
           of the logical volume is recorded */
        vol.FileSetLocation.Parse(buf + 248);
        vol.FileSetLocation.Len = vol.BlockSize;
        // memcpy(vol.ContentsUse, buf + 248, sizeof(vol.ContentsUse));

        vol.ImplId.Parse(buf + 272);
        // memcpy(vol.ImplUse, buf + 304, sizeof(vol.ImplUse));
        // vol.IntegritySequenceExtent.Parse(buf + 432);

        if (linuxFlags)
            vol.LinuxFlags = *linuxFlags;
        const HRESULT result = LoadLinuxMaps(vol, buf, (UInt32)bufSize);
        if (linuxFlags)
            *linuxFlags = vol.LinuxFlags;
        return result;
    }

    return S_OK;
}

HRESULT CInArchive::Open2() {
    Clear();
    UInt64 fileSize;
    RINOK(InStream_GetSize_SeekToEnd(_stream, fileSize))
    FileSize = fileSize;

    // Some UDFs contain additional pad zeros (2 KB).
    // Seek to STREAM_SEEK_END for direct DVD reading can return 8 KB more, so we check last 16 KB.
    // And when we read last block, result read size can be smaller than required size.

    /*
    const size_t kBufSize = 1 << 14;
    Byte buf[kBufSize];
    size_t readSize = (fileSize < kBufSize) ? (size_t)fileSize : kBufSize;
    RINOK(InStream_SeekSet(_stream, fileSize - readSize))
    RINOK(ReadStream(_stream, buf, &readSize));
    size_t i = readSize;
    for (;;)
    {
      const size_t kSecSizeMin = 1 << 8;
      if (i < kSecSizeMin)
        return S_FALSE;
      i -= kSecSizeMin;
      SecLogSize = (readSize - i < ((size_t)1 << 11)) ? 8 : 11;
      CTag tag;
      if (tag.Parse(buf + i, (1 << SecLogSize)) == S_OK)
        if (tag.Id == DESC_TYPE_AnchorVolPtr)
          break;
    }
    PhySize = fileSize;
    CExtent extentVDS;
    extentVDS.Parse(buf + i + 16);
    */

    /*
    An Anchor Volume Descriptor Pointer structure shall be recorded in at
    least 2 of the following 3 locations on the media:
        Logical Sector 256.
        Logical Sector (N - 256).
        N
    */

    Byte buf[1 << 12];
    RINOK(LoadLinuxDiscovery())

    UInt64 totalSize = 0;

    unsigned volIndex;
    for (volIndex = 0; volIndex < LogVols.Size(); volIndex++) {
        CLogVol &vol = LogVols[volIndex];
        FOR_VECTOR(p, vol.PartitionMaps) {
            const CPartitionMap &map = vol.PartitionMaps[p];
            if ((map.Native.kind == UDF_TYPE1_MAP15 || map.IsSparable()) &&
                map.PartitionIndex < Partitions.Size())
                totalSize += (UInt64)Partitions[map.PartitionIndex].Len << SecLogSize;
        }
    }

    RINOK(_progress->SetTotal(totalSize))

    PRF(printf("\n Read files"));

    for (volIndex = 0; volIndex < LogVols.Size(); volIndex++) {
        CLogVol &vol = LogVols[volIndex];

        PRF(printf("\nLogVol %2d", volIndex));

        CLongAllocDesc nextExtent = vol.FileSetLocation;
        // while (nextExtent.ExtentLen != 0)
        // for (int i = 0; i < 1; i++)
        {
            CByteBuffer buf2(vol.BlockSize);
            RINOK(ReadLinuxFileset(volIndex, nextExtent, buf2))
            const Byte *p = buf2;

            CTag tag;
            tag.Id = Get16(p);

            /*
            // commented in 22.01
            if (tag.Id == DESC_TYPE_ExtendedFile)
            {
              // ECMA 4 / 14.17
              // 2.60 ??
              return S_FALSE;
            }
            */

            if (tag.Id != DESC_TYPE_FileSet)
                return S_FALSE;

            PRF(printf("\n FileSet"));
            CFileSet fs;
            fs.RecordingTime.Parse(p + 16);
            // fs.InterchangeLevel = Get16(p + 18);
            // fs.MaxInterchangeLevel = Get16(p + 20);
            fs.FileSetNumber = Get32(p + 40);
            fs.FileSetDescNumber = Get32(p + 44);

            fs.LogicalVolumeId.Parse(p + 112);
            fs.Id.Parse(p + 304);
            fs.CopyrightId.Parse(p + 336);
            fs.AbstractId.Parse(p + 368);

            fs.RootDirICB.Parse(p + 400);
            fs.DomainId.Parse(p + 416);

            // fs.SystemStreamDirICB.Parse(p + 464);

            vol.FileSets.Add(fs);

            // nextExtent.Parse(p + 448);
        }

        FOR_VECTOR(fsIndex, vol.FileSets) {
            CFileSet &fs = vol.FileSets[fsIndex];
            const unsigned fileIndex = Files.Size();
            Files.AddNew();
            RINOK(ReadFileItem(volIndex, fsIndex, fs.RootDirICB,
                               true, // isDir
                               kNumRecursionLevelsMax))
            RINOK(FillRefs(fs, fileIndex, -1, kNumRecursionLevelsMax))
        }
    }

    for (volIndex = 0; volIndex < LogVols.Size(); volIndex++) {
        const CLogVol &vol = LogVols[volIndex];
        // bool showFileSetName = (vol.FileSets.Size() > 1);
        FOR_VECTOR(fsIndex, vol.FileSets) {
            const CFileSet &fs = vol.FileSets[fsIndex];
            for (unsigned i =
                     // ((showVolName || showFileSetName) ? 0 : 1)
                 0;
                 i < fs.Refs.Size(); i++) {
                const CRef &ref = vol.FileSets[fsIndex].Refs[i];
                const CFile &file = Files[ref.FileIndex];
                const CItem &item = Items[file.ItemIndex];
                UInt64 size = item.Size;

                if (!item.CheckChunkSizes() || !CheckItemExtents(volIndex, item))
                    continue;

                FOR_VECTOR(extentIndex, item.Extents) {
                    if (size == 0)
                        break;
                    const CMyExtent &extent = item.Extents[extentIndex];
                    const UInt32 len = (UInt32)MyMin(size, (UInt64)extent.GetLen());
                    size -= len;
                    if (len == 0)
                        continue;
                    if (!extent.IsRecAndAlloc())
                        continue;

                    UInt32 remaining = len, blockOffset = 0;
                    while (remaining) {
                        UInt64 offset;
                        UInt32 chunk;
                        RINOK(GetPhysicalSpan(volIndex, extent.PartitionRef, extent.Pos, remaining,
                                              offset, chunk, blockOffset))
                        UpdatePhySize(offset + chunk);
                        remaining -= chunk;
                        blockOffset += chunk / vol.BlockSize;
                    }
                }
            }
        }
    }

    {
        const UInt32 secMask = ((UInt32)1 << SecLogSize) - 1;
        PhySize = (PhySize + secMask) & ~(UInt64)secMask;
    }

    // Linux accepts the already validated primary anchor, including open media
    // and previous inode revisions followed by unreferenced rewrite records.
    NoEndAnchor = false;

    if (PhySize < fileSize) {
        UInt64 rem = fileSize - PhySize;
        const size_t secSize = (size_t)1 << SecLogSize;

        const HRESULT tailSeek = InStream_SeekSet(_stream, PhySize);
        if (tailSeek != S_OK)
            return tailSeek == E_ABORT ? E_ABORT : S_OK;

        // some UDF images contain ZEROs before "Anchor Volume Descriptor Pointer" at the end

        for (unsigned sec = 0; sec < 1024; sec++) {
            if (rem == 0)
                break;

            size_t readSize = secSize;
            if (readSize > rem)
                readSize = (size_t)rem;

            const HRESULT tailRead = ReadStream(_stream, buf, &readSize);
            if (tailRead == E_ABORT)
                return E_ABORT;
            if (tailRead != S_OK)
                break;

            if (readSize == 0)
                break;

            // some udf contain many EndAnchors
            if (readSize == secSize /* && NoEndAnchor */) {
                CTag tag;
                if (tag.Parse(buf, readSize) == S_OK && tag.Id == DESC_TYPE_AnchorVolPtr &&
                    Get32(buf + 12) == (UInt32)((fileSize - rem) >> SecLogSize)) {
                    NoEndAnchor = false;
                    rem -= readSize;
                    PhySize = fileSize - rem;
                    continue;
                }
            }

            size_t i;
            for (i = 0; i < readSize && buf[i] == 0; i++)
                ;
            if (i != readSize)
                break;
            rem -= readSize;
        }

        if (rem == 0)
            PhySize = fileSize;
    }

    return S_OK;
}

HRESULT CInArchive::Open(IInStream *inStream, CProgressVirt *progress) {
    _progress = progress;
    struct CResetProgress {
        CProgressVirt **Value;
        ~CResetProgress() { *Value = nullptr; }
    } resetProgress{&_progress};
    _stream = inStream;
    HRESULT res = Open2();
    if (res == S_FALSE && IsArc && !UnexpectedEnd)
        Unsupported = true;
    return res;

    /*
    HRESULT res;
    try
    {
      res = Open2();
    }
    catch(...)
    {
      // Clear();
      // res = S_FALSE;
      _stream.Release();
      throw;
    }
    _stream.Release();
    return res;
    */
}

void CInArchive::Clear() {
    IsArc = false;
    Unsupported = false;
    UnexpectedEnd = false;
    NoEndAnchor = false;

    PhySize = 0;
    FileSize = 0;

    Partitions.Clear();
    LogVols.Clear();
    PrimeVols.Clear();
    Items.Clear();
    Files.Clear();
    _fileNameLengthTotal = 0;
    _numRefs = 0;
    _numExtents = 0;
    _inlineExtentsSize = 0;
    _processedProgressBytes = 0;
    _lastRecordedBlock = 0;
}

static const char *const g_PartitionTypes[] = {"Pseudo-Overwritable" // UDF
                                               ,
                                               "Read-Only", "Write-Once", "Rewritable",
                                               "Overwritable"};

static void AddComment_Align(UString &s) { s += "  "; }

static void AddComment_PropName(UString &s, const char *name) {
    AddComment_Align(s);
    s += name;
    s += ": ";
}

static void AddComment_UInt32(UString &s, const char *name, UInt32 val) {
    AddComment_PropName(s, name);
    s.Add_UInt32(val);
    s.Add_LF();
}

static void AddComment_UInt32_2(UString &s, const char *name, UInt32 val) {
    AddComment_Align(s);
    AddComment_UInt32(s, name, val);
}

static void AddComment_UInt64(UString &s, const char *name, UInt64 val) {
    AddComment_PropName(s, name);
    s.Add_UInt64(val);
    s.Add_LF();
}

static void AddComment_RegId(UString &s, const char *name, const CRegId &ri) {
    AddComment_PropName(s, name);
    ri.AddCommentTo(s);
    s.Add_LF();
}

static void AddComment_RegId_Domain(UString &s, const char *name, const CRegId &ri) {
    AddComment_PropName(s, name);
    ri.AddCommentTo(s);
    {
        UString s2;
        ri.AddUdfVersionTo(s2);
        if (!s2.IsEmpty()) {
            s += "::";
            s += s2;
        }
    }
    s.Add_LF();
}

// UDF 6.3.1 OS Class

static const char *const g_OsClasses[] = {NULL,   "DOS",        "OS/2",       "Macintosh OS",
                                          "UNIX", "Windows 9x", "Windows NT", "OS/400",
                                          "BeOS", "Windows CE"};

// UDF 6.3.2 OS Identifier

static const char *const g_OsIds_Unix[] = {NULL // "Generic"
                                           ,
                                           "AIX",
                                           "SUN OS / Solaris",
                                           "HP/UX",
                                           "Silicon Graphics Irix",
                                           "Linux",
                                           "MKLinux",
                                           "FreeBSD",
                                           "NetBSD"};

static void AddOs_Class_Id(UString &s, const Byte *p) {
    // UDF 2.1.5.3 Implementation Identifier Suffix
    // Appendix 6.3 Operating System Identifiers.
    const Byte osClass = p[0];
    if (osClass != 0) {
        s += "::";
        s += TypeToString(g_OsClasses, Z7_ARRAY_SIZE(g_OsClasses), osClass);
    }
    const Byte osId = p[1];
    if (osId != 0) {
        s += "::";
        if (osClass == 4) // unix
        {
            s += TypeToString(g_OsIds_Unix, Z7_ARRAY_SIZE(g_OsIds_Unix), osId);
        } else
            s.Add_UInt32(osId);
    }
}

static void AddComment_RegId_Impl(UString &s, const char *name, const CRegId &ri) {
    AddComment_PropName(s, name);
    ri.AddCommentTo(s);
    {
        AddOs_Class_Id(s, ri.Suffix);
    }
    s.Add_LF();
}

static void AddComment_RegId_UdfId(UString &s, const char *name, const CRegId &ri) {
    AddComment_PropName(s, name);
    ri.AddCommentTo(s);
    {
        // UDF 2.1.5.3
        // UDF Identifier Suffix format
        UString s2;
        ri.AddUdfVersionTo(s2);
        if (!s2.IsEmpty()) {
            s += "::";
            s += s2;
        }
        AddOs_Class_Id(s, &ri.Suffix[2]);
    }
    s.Add_LF();
}

static void AddComment_DString32(UString &s, const char *name, const CDString32 &d) {
    AddComment_Align(s);
    AddComment_PropName(s, name);
    s += d.GetString();
    s.Add_LF();
}

UString CInArchive::GetComment() const {
    UString s;
    {
        s += "Primary Volumes:";
        s.Add_LF();
        FOR_VECTOR(i, PrimeVols) {
            if (i != 0)
                s.Add_LF();
            const CPrimeVol &pv = PrimeVols[i];
            // AddComment_UInt32(s, "VolumeDescriptorSequenceNumber",
            // pv.VolumeDescriptorSequenceNumber); if (PrimeVols.Size() != 1 ||
            // pv.PrimaryVolumeDescriptorNumber != 0)
            AddComment_UInt32(s, "PrimaryVolumeDescriptorNumber", pv.PrimaryVolumeDescriptorNumber);
            // if (pv.MaximumVolumeSequenceNumber != 1 || pv.VolumeSequenceNumber != 1)
            AddComment_UInt32(s, "VolumeSequenceNumber", pv.VolumeSequenceNumber);
            if (pv.MaximumVolumeSequenceNumber != 1)
                AddComment_UInt32(s, "MaximumVolumeSequenceNumber", pv.MaximumVolumeSequenceNumber);
            AddComment_PropName(s, "VolumeId");
            s += pv.VolumeId.GetString();
            s.Add_LF();
            AddComment_PropName(s, "VolumeSetId");
            s += pv.VolumeSetId.GetString();
            s.Add_LF();
            // AddComment_UInt32(s, "InterchangeLevel", pv.InterchangeLevel);
            // AddComment_UInt32(s, "MaximumInterchangeLevel", pv.MaximumInterchangeLevel);
            AddComment_RegId(s, "ApplicationId", pv.ApplicationId);
            AddComment_RegId_Impl(s, "ImplementationId", pv.ImplId);
        }
    }
    {
        s += "Partitions:";
        s.Add_LF();
        FOR_VECTOR(i, Partitions) {
            if (i != 0)
                s.Add_LF();
            const CPartition &part = Partitions[i];
            AddComment_UInt32(s, "PartitionIndex", i);
            AddComment_UInt32(s, "PartitionNumber", part.Number);
            if (part.IsMetadata)
                AddComment_UInt32(s, "IsMetadata", 1);
            else {
                AddComment_RegId(s, "ContentsId", part.ContentsId);
                AddComment_RegId_Impl(s, "ImplementationId", part.ImplId);
                AddComment_PropName(s, "AccessType");
                s += TypeToString(g_PartitionTypes, Z7_ARRAY_SIZE(g_PartitionTypes),
                                  part.AccessType);
                s.Add_LF();
            }
            AddComment_UInt64(s, "Size", (UInt64)part.Len << SecLogSize);
            AddComment_UInt64(s, "Pos", (UInt64)part.Pos << SecLogSize);
        }
    }
    s += "Logical Volumes:";
    s.Add_LF();
    {
        FOR_VECTOR(i, LogVols) {
            if (i != 0)
                s.Add_LF();
            const CLogVol &vol = LogVols[i];
            if (LogVols.Size() != 1)
                AddComment_UInt32(s, "Number", i);
            AddComment_PropName(s, "Id");
            s += vol.Id.GetString();
            s.Add_LF();
            AddComment_UInt32(s, "BlockSize", vol.BlockSize);
            AddComment_RegId_Domain(s, "DomainId", vol.DomainId);
            AddComment_RegId_Impl(s, "ImplementationId", vol.ImplId);
            // AddComment_UInt64(s, "IntegritySequenceExtent_Len", vol.IntegritySequenceExtent.Len);
            // AddComment_UInt64(s, "IntegritySequenceExtent_Pos",
            // (UInt64)vol.IntegritySequenceExtent.Pos << SecLogSize);

            s += "  Partition Maps:";
            s.Add_LF();
            {
                FOR_VECTOR(j, vol.PartitionMaps) {
                    if (j != 0)
                        s.Add_LF();
                    const CPartitionMap &pm = vol.PartitionMaps[j];
                    AddComment_UInt32_2(s, "PartitionMap", j);
                    AddComment_UInt32_2(s, "Type", pm.Type);
                    AddComment_UInt32_2(s, "VolumeSequenceNumber", pm.VolumeSequenceNumber);
                    AddComment_UInt32_2(s, "PartitionNumber", pm.PartitionNumber);
                    if (pm.Type == 2) {
                        AddComment_UInt32_2(s, "MetadataFileLocation", pm.MetadataFileLocation);
                        // AddComment_UInt32_2(s, "MetadataMirrorFileLocation",
                        // pm.MetadataMirrorFileLocation); AddComment_UInt32_2(s,
                        // "MetadataBitmapFileLocation", pm.MetadataBitmapFileLocation);
                        // AddComment_UInt32_2(s, "AllocationUnitSize", pm.AllocationUnitSize);
                        // AddComment_UInt32_2(s, "AlignmentUnitSize", pm.AlignmentUnitSize);
                        // AddComment_UInt32_2(s, "Flags", pm.Flags);
                        AddComment_Align(s);
                        AddComment_RegId_UdfId(s, "PartitionTypeId", pm.PartitionTypeId);
                    }
                }
            }
            s += "  File Sets:";
            s.Add_LF();
            {
                FOR_VECTOR(j, vol.FileSets) {
                    if (j != 0)
                        s.Add_LF();
                    const CFileSet &fs = vol.FileSets[j];
                    AddComment_Align(s);
                    AddComment_UInt32(s, "FileSetNumber", fs.FileSetNumber);
                    AddComment_Align(s);
                    AddComment_UInt32(s, "FileSetDescNumber", fs.FileSetDescNumber);

                    AddComment_Align(s);
                    AddComment_PropName(s, "LogicalVolumeId");
                    s += fs.LogicalVolumeId.GetString();
                    s.Add_LF();

                    AddComment_DString32(s, "Id", fs.Id);
                    AddComment_DString32(s, "CopyrightId", fs.CopyrightId);
                    AddComment_DString32(s, "AbstractId", fs.AbstractId);

                    AddComment_Align(s);
                    AddComment_RegId_Domain(s, "DomainId", fs.DomainId);
                }
            }
        }
    }
    return s;
}

static UString GetSpecName(const UString &name) {
    UString name2 = name;
    if (name2.IsEmpty())
        return UString("[]");
    return name;
}

static void UpdateWithName(UString &res, const UString &addString) {
    if (res.IsEmpty())
        res = addString;
    else
        res.Insert(0, addString + WCHAR_PATH_SEPARATOR);
}

UString CInArchive::GetItemPath(unsigned volIndex, unsigned fsIndex, unsigned refIndex,
                                bool showVolName, bool showFsName) const {
    // showVolName = true;
    const CLogVol &vol = LogVols[volIndex];
    const CFileSet &fs = vol.FileSets[fsIndex];

    UString name;

    for (;;) {
        const CRef &ref = fs.Refs[refIndex];
        // we break on root file (that probably has empty name)
        if (ref.Parent < 0)
            break;
        refIndex = (unsigned)ref.Parent;
        UpdateWithName(name, GetSpecName(Files[ref.FileIndex].GetName()));
    }

    if (showFsName) {
        UString newName("File Set ");
        newName.Add_UInt32(fsIndex);
        UpdateWithName(name, newName);
    }

    if (showVolName) {
        UString newName;
        newName.Add_UInt32(volIndex);
        UString newName2 = vol.GetName();
        if (newName2.IsEmpty())
            newName2 = "Volume";
        newName.Add_Minus();
        newName += newName2;
        UpdateWithName(name, newName);
    }
    return name;
}

} // namespace NUdf
} // namespace NArchive

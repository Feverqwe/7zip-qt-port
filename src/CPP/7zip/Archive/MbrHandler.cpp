// MbrHandler.cpp

#include "StdAfx.h"

// #define SHOW_DEBUG_INFO

#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#endif

#include "../../../C/CpuArch.h"

#include "../../Common/ComTry.h"
#include "../../Common/IntToString.h"
#include "../../Common/MyBuffer.h"

#include "../../Windows/PropVariant.h"

#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "HandlerCont.h"
#ifdef SEVENZIP_LINUX_MBR
#include "../../../msdosreader.h"
#include <errno.h>
#endif

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

using namespace NWindows;

namespace NArchive {

namespace NFat {
API_FUNC_IsArc IsArc_Fat(const Byte *p, size_t size);
}

namespace NMbr {

struct CChs {
    Byte Head;
    Byte SectCyl;
    Byte Cyl8;

    UInt32 GetSector() const { return SectCyl & 0x3F; }
    UInt32 GetCyl() const { return ((UInt32)SectCyl >> 6 << 8) | Cyl8; }
    void ToString(NCOM::CPropVariant &prop) const;

    void Parse(const Byte *p) {
        Head = p[0];
        SectCyl = p[1];
        Cyl8 = p[2];
    }
    bool Check() const {
        if ((Head | SectCyl | Cyl8) == 0)
            return true;
        return GetSector() > 0;
    }
};

// Chs in some MBRs contains only low bits of "Cyl number". So we disable check.
/*
#define RINOZ(x) { int _t_ = (x); if (_t_ != 0) return _t_; }
static int CompareChs(const CChs &c1, const CChs &c2)
{
  RINOZ(MyCompare(c1.GetCyl(), c2.GetCyl()));
  RINOZ(MyCompare(c1.Head, c2.Head));
  return MyCompare(c1.GetSector(), c2.GetSector());
}
*/

void CChs::ToString(NCOM::CPropVariant &prop) const {
    AString s;
    s.Add_UInt32(GetCyl());
    s.Add_Minus();
    s.Add_UInt32(Head);
    s.Add_Minus();
    s.Add_UInt32(GetSector());
    prop = s;
}

struct CPartition {
    Byte Status;
    CChs BeginChs;
    Byte Type;
    CChs EndChs;
    UInt64 Lba;
    UInt64 NumBlocks;

    CPartition() { memset(this, 0, sizeof(*this)); }

    bool IsEmpty() const { return NumBlocks == 0; }
    bool IsExtended() const { return Type == 5 || Type == 0xF || Type == 0x85; }
    UInt64 GetLimit() const { return Lba + NumBlocks; }
    // bool IsActive() const { return Status == 0x80; }
    UInt64 GetPos(unsigned sectorSizeLog) const { return (UInt64)Lba << sectorSizeLog; }
    UInt64 GetSize(unsigned sectorSizeLog) const { return (UInt64)NumBlocks << sectorSizeLog; }

    bool Parse(const Byte *p) {
        Status = p[0];
        BeginChs.Parse(p + 1);
        Type = p[4];
        EndChs.Parse(p + 5);
        Lba = GetUi32(p + 8);
        NumBlocks = GetUi32(p + 12);
        return true; // LBA tables do not require valid CHS; EBR boot flags are unused.
    }

#ifdef SHOW_DEBUG_INFO
    void Print() const {
        NCOM::CPropVariant prop, prop2;
        BeginChs.ToString(prop);
        EndChs.ToString(prop2);
        printf("   %2x %2x %8llX %8llX %12S %12S", (int)Status, (int)Type, (unsigned long long)Lba,
               (unsigned long long)NumBlocks, prop.bstrVal, prop2.bstrVal);
    }
#endif
};

struct CPartType {
    UInt32 Id;
    const char *Ext;
    const char *Name;
};

#define kFat "fat"

/*
if we use "format" command in Windows 10 for existing partition:
  - if we format to ExFAT, it sets type=7
  - if we format to UDF, it doesn't change type from previous value.
*/

static const unsigned kType_Windows_NTFS = 7;

static const CPartType kPartTypes[] = {{0x01, kFat, "FAT12"},
                                       {0x04, kFat, "FAT16 DOS 3.0+"},
                                       {0x05, NULL, "Extended"},
                                       {0x06, kFat, "FAT16 DOS 3.31+"},
                                       {0x07, "ntfs", "NTFS"},
                                       {0x0B, kFat, "FAT32"},
                                       {0x0C, kFat, "FAT32-LBA"},
                                       {0x0E, kFat, "FAT16-LBA"},
                                       {0x0F, NULL, "Extended-LBA"},
                                       {0x11, kFat, "FAT12-Hidden"},
                                       {0x14, kFat, "FAT16-Hidden < 32 MB"},
                                       {0x16, kFat, "FAT16-Hidden >= 32 MB"},
                                       {0x1B, kFat, "FAT32-Hidden"},
                                       {0x1C, kFat, "FAT32-LBA-Hidden"},
                                       {0x1E, kFat, "FAT16-LBA-WIN95-Hidden"},
                                       {0x27, "ntfs", "NTFS-WinRE"},
                                       {0x82, NULL, "Solaris x86 / Linux swap"},
                                       {0x83, NULL, "Linux"},
                                       {0x8E, "lvm", "Linux LVM"},
                                       {0xA5, NULL, "BSD slice"},
                                       {0xBE, NULL, "Solaris 8 boot"},
                                       {0xBF, NULL, "New Solaris x86"},
                                       {0xC2, NULL, "Linux-Hidden"},
                                       {0xC3, NULL, "Linux swap-Hidden"},
                                       {0xEE, NULL, "GPT"},
                                       {0xEE, NULL, "EFI"}};

static int FindPartType(UInt32 type) {
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(kPartTypes); i++)
        if (kPartTypes[i].Id == type)
            return (int)i;
    return -1;
}

struct CItem {
    bool IsReal;
    bool IsPrim;
    bool WasParsed;
    const char *FileSystem;
    UInt64 Size;
    UInt64 Offset;
    CPartition Part;

    CItem() : WasParsed(false), FileSystem(NULL) {}
};

Z7_class_CHandler_final : public CHandlerCont {
    Z7_IFACE_COM7_IMP(IInArchive_Cont)

    CObjectVector<CItem> _items;
    UInt64 _totalSize;
    CByteBuffer _buffer;

    UInt32 _signature;
    unsigned _sectorSizeLog;
    unsigned _numLogical;
    bool _sectorSizeKnown;
#ifdef SEVENZIP_LINUX_MBR
    CPartition _primary[4];
    HRESULT ReadSubtypes(IInStream * stream);
#endif

    virtual int GetItem_ExtractInfo(UInt32 index, UInt64 & pos, UInt64 & size)
        const Z7_override Z7_final {
        const CItem &item = _items[index];
        pos = item.Offset;
        size = item.Size;
        return NExtract::NOperationResult::kOK;
    }

    HRESULT ReadTables(IInStream * stream, UInt64 baseLba, UInt64 lba, unsigned level,
                       UInt64 extendedSize = 0);
    HRESULT Open2(IInStream * stream);
};

/*
static bool IsEmptyBuffer(const Byte *data, size_t size)
{
  for (unsigned i = 0; i < size; i++)
    if (data[i] != 0)
      return false;
  return true;
}
*/

const char *GetFileSystem(ISequentialInStream *stream, UInt64 partitionSize);

HRESULT CHandler::ReadTables(IInStream *stream, UInt64 baseLba, UInt64 lba, unsigned level,
                             UInt64 extendedSize) {
    const unsigned kNumHeaderParts = 4;
    CPartition parts[kNumHeaderParts];
    CRecordVector<UInt64> visited;
    unsigned emptyLinks = 0;
    UInt64 thisSize = extendedSize;

    for (;;) {
        if (level != 0) {
            if (++emptyLinks > 100 || _numLogical >= 251 || visited.FindInSorted(lba) >= 0)
                return S_OK;
            visited.AddToUniqueSorted(lba);
        }
        const UInt32 kSectorSize = (UInt32)1 << _sectorSizeLog;
        UInt32 bufSize = kSectorSize;
        if (level == 0 && _totalSize >= (1 << 12))
            bufSize = (1 << 12);
        _buffer.Alloc(bufSize);
        {
            Byte *buf = _buffer;
            const UInt64 newPos = (UInt64)lba << _sectorSizeLog;
            if (newPos > _totalSize || bufSize > _totalSize - newPos)
                return level == 0 ? S_FALSE : S_OK;
            RINOK(InStream_SeekSet(stream, newPos))
            RINOK(ReadStream_FALSE(stream, buf, bufSize))
            if (buf[0x1FE] != 0x55 || buf[0x1FF] != 0xAA)
                return level == 0 ? S_FALSE : S_OK;
            if (level == 0)
                _signature = GetUi32(buf + 0x1B8);
            for (unsigned i = 0; i < kNumHeaderParts; i++) {
                parts[i].Parse(buf + 0x1BE + 16 * i);
#ifdef SEVENZIP_LINUX_MBR
                if (level == 0)
                    _primary[i] = parts[i];
#endif
                if (level == 0 && parts[i].Status != 0 && parts[i].Status != 0x80)
                    return S_FALSE;
            }
        }

        // 23.02: now we try to detect 4kn format (4 KB sectors case)
        if (level == 0 && !_sectorSizeKnown)
            // if (_totalSize >= (1 << 28)) // we don't expect small images with 4 KB sectors
            if (bufSize >= (1 << 12)) {
                UInt64 lastLim = 0;
                UInt64 firstLba = 0;
                UInt64 numBlocks = 0; // in first partition
                for (unsigned i = 0; i < kNumHeaderParts; i++) {
                    const CPartition &part = parts[i];
                    if (part.IsEmpty())
                        continue;
                    if (firstLba == 0 && part.NumBlocks != 0) {
                        firstLba = part.Lba;
                        numBlocks = part.NumBlocks;
                    }
                    const UInt64 newLim = part.GetLimit();
                    if (lastLim < newLim)
                        lastLim = newLim;
                }
                if (lastLim != 0) {
                    const UInt64 lim12 = (UInt64)lastLim << 12;
                    if (lim12 <= _totalSize
                        // && _totalSize - lim12 < (1 << 28) // we can try to exclude false positive
                        // cases
                    )
                        // if (IsEmptyBuffer(&_buffer[(1 << 9)], (1 << 12) - (1 << 9)))
                        if (InStream_SeekSet(stream, (UInt64)firstLba << 12) == S_OK)
                            if (GetFileSystem(stream, (UInt64)numBlocks << 12))
                                _sectorSizeLog = 12;
                }
            }

        PRF(printf("\n# %8llX", (unsigned long long)lba));

        for (unsigned i = 0; i < kNumHeaderParts; i++) {
            CPartition &part = parts[i];

            if (part.IsEmpty())
                continue;
            PRF(printf("\n   %2d ", (unsigned)level));
#ifdef SHOW_DEBUG_INFO
            part.Print();
#endif

            if (level != 0 && part.IsExtended())
                continue; // Process every data slot before following the first EBR link.
            const UInt64 relative = part.Lba;
            UInt64 newLba = lba + relative;
            if (level != 0 && i >= 2) {
                if (relative + part.NumBlocks > thisSize || newLba < baseLba ||
                    newLba + part.NumBlocks > baseLba + extendedSize)
                    continue;
            }
            if (level != 0) {
                emptyLinks = 0;
                ++_numLogical; // Count accepted table entries before block-layer EOD filtering.
            }
            const UInt64 diskBlocks = _totalSize >> _sectorSizeLog;
            if (newLba >= diskBlocks) {
                if (level != 0 && _numLogical >= 251)
                    return S_OK;
                continue;
            }
            part.Lba = newLba;
            const UInt64 declaredBlocks = part.NumBlocks;
            if (part.NumBlocks > diskBlocks - newLba)
                part.NumBlocks = diskBlocks - newLba; // Linux block layer clamps at EOD.

            const unsigned numItems = _items.Size();
            if (part.IsExtended()) {
                RINOK(ReadTables(stream, newLba, newLba, 1, declaredBlocks))
            }

            CItem n;
            n.Part = part;
            bool addItem = false;
            if (numItems == _items.Size()) {
                n.IsPrim = (level == 0);
                n.IsReal = true;
                addItem = true;
            } else {
                UInt64 backLimit = part.Lba;
                for (unsigned j = numItems; j < _items.Size(); ++j)
                    if (backLimit < _items[j].Part.GetLimit())
                        backLimit = _items[j].Part.GetLimit();
                const UInt64 partLimit = part.GetLimit();
                if (backLimit < partLimit) {
                    n.IsReal = false;
                    n.Part.Lba = backLimit;
                    n.Part.NumBlocks = partLimit - backLimit;
                    addItem = true;
                }
            }
            if (addItem) {
                n.Offset = n.Part.GetPos(_sectorSizeLog);
                n.Size = n.Part.GetSize(_sectorSizeLog);
                _items.Add(n);
                if (level != 0) {
                    if (_numLogical >= 251)
                        return S_OK;
                }
            }
        }
        if (level == 0)
            return S_OK;
        unsigned next;
        for (next = 0; next < kNumHeaderParts; ++next)
            if (!parts[next].IsEmpty() && parts[next].IsExtended())
                break;
        if (next == kNumHeaderParts)
            return S_OK;
        lba = baseLba + parts[next].Lba;
        thisSize = parts[next].NumBlocks;
    }
}

static const Byte k_Ntfs_Signature[] = {'N', 'T', 'F', 'S', ' ', ' ', ' ', ' ', 0};

static bool Is_Ntfs(const Byte *p) {
    if (p[0x1FE] != 0x55 || p[0x1FF] != 0xAA)
        return false;
    switch (p[0]) {
    case 0xE9: /* codeOffset = 3 + (Int16)Get16(p + 1); */
        break;
    case 0xEB:
        if (p[2] != 0x90)
            return false; /* codeOffset = 2 + (int)(signed char)p[1]; */
        break;
    default:
        return false;
    }
    return memcmp(p + 3, k_Ntfs_Signature, Z7_ARRAY_SIZE(k_Ntfs_Signature)) == 0;
}

static const Byte k_ExFat_Signature[] = {'E', 'X', 'F', 'A', 'T', ' ', ' ', ' '};

static bool Is_ExFat(const Byte *p) {
    if (p[0x1FE] != 0x55 || p[0x1FF] != 0xAA)
        return false;
    if (p[0] != 0xEB || p[1] != 0x76 || p[2] != 0x90)
        return false;
    return memcmp(p + 3, k_ExFat_Signature, Z7_ARRAY_SIZE(k_ExFat_Signature)) == 0;
}

static bool AllAreZeros(const Byte *p, size_t size) {
    for (size_t i = 0; i < size; i++)
        if (p[i] != 0)
            return false;
    return true;
}

static const UInt32 k_Udf_StartPos = 0x8000;
static const Byte k_Udf_Signature[] = {0, 'B', 'E', 'A', '0', '1', 1, 0};

static bool Is_Udf(const Byte *p) {
    return memcmp(p + k_Udf_StartPos, k_Udf_Signature, sizeof(k_Udf_Signature)) == 0;
}

const char *GetFileSystem(ISequentialInStream *stream, UInt64 partitionSize) {
    const size_t kHeaderSize = 1 << 9;
    if (partitionSize >= kHeaderSize) {
        Byte buf[kHeaderSize];
        if (ReadStream_FAIL(stream, buf, kHeaderSize) == S_OK) {
            // NTFS is expected default filesystem for (Type == 7)
            if (Is_Ntfs(buf))
                return "NTFS";
            if (Is_ExFat(buf))
                return "exFAT";
            if (NFat::IsArc_Fat(buf, kHeaderSize))
                return "FAT";
            const size_t kHeaderSize2 = k_Udf_StartPos + (1 << 9);
            if (partitionSize >= kHeaderSize2) {
                if (AllAreZeros(buf, kHeaderSize)) {
                    CByteBuffer buffer(kHeaderSize2);
                    // memcpy(buffer, buf, kHeaderSize);
                    if (ReadStream_FAIL(stream, buffer + kHeaderSize, kHeaderSize2 - kHeaderSize) ==
                        S_OK)
                        if (Is_Udf(buffer))
                            return "UDF";
                }
            }
        }
    }
    return NULL;
}

#ifdef SEVENZIP_LINUX_MBR
static int32_t ReadNested(void *context, uint64_t offset, void *data, unsigned size) {
    IInStream *stream = static_cast<IInStream *>(context);
    RINOK(InStream_SeekSet(stream, offset))
    return ReadStream_FALSE(stream, data, size);
}

HRESULT CHandler::ReadSubtypes(IInStream *stream) {
    sevenzip_msdos_parent parents[4];
    for (unsigned i = 0; i < 4; ++i) {
        parents[i].offset = _primary[i].Lba << (_sectorSizeLog - 9);
        parents[i].size = _primary[i].NumBlocks << (_sectorSizeLog - 9);
        parents[i].type = _primary[i].Type;
    }
    sevenzip_msdos_part parts[251];
    unsigned count = 0;
    const int32_t result =
        sevenzip_msdos_scan(stream, ReadNested, _totalSize, parents, 4, _numLogical, parts, &count);
    if (result == -ENOMEM)
        return E_OUTOFMEMORY;
    if (result == -EINVAL)
        return E_INVALIDARG;
    RINOK(result)
    _numLogical += count;
    const UInt64 diskSectors = _totalSize >> 9;
    for (unsigned i = 0; i < count; ++i) {
        const sevenzip_msdos_part &part = parts[i];
        if (!part.size || part.offset >= diskSectors)
            continue;
        CItem item;
        item.IsPrim = false;
        item.IsReal = true;
        item.Part.Type = part.parent_type;
        item.Offset = part.offset << 9;
        item.Size = MyMin((UInt64)part.size, diskSectors - part.offset) << 9;
        _items.Add(item);
    }
    return S_OK;
}
#endif

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 * /* maxCheckStartPosition */,
                            IArchiveOpenCallback * /* openArchiveCallback */)) {
    COM_TRY_BEGIN
    Close();
    const HRESULT result = Open2(stream);
    if (result != S_OK)
        Close();
    return result;
    COM_TRY_END
}

HRESULT CHandler::Open2(IInStream *stream) {
    _sectorSizeLog = 9;
    CMyComPtr<IStreamGetProp> props;
    if (stream->QueryInterface(IID_IStreamGetProp, (void **)&props) == S_OK) {
        NCOM::CPropVariant sector;
        RINOK(props->GetProperty(kpidSectorSize, &sector))
        if (sector.vt == VT_UI4) {
            const UInt32 size = sector.ulVal;
            if (size < 512 || size > (1u << 16) || (size & (size - 1)))
                return E_INVALIDARG;
            while ((1u << _sectorSizeLog) < size)
                ++_sectorSizeLog;
            _sectorSizeKnown = true;
        }
    }
    RINOK(InStream_GetSize_SeekToEnd(stream, _totalSize))
    RINOK(ReadTables(stream, 0, 0, 0))
    if (_items.IsEmpty())
        return S_FALSE;
    {
        UInt64 lbaLimit = 0;
        FOR_VECTOR(i, _items)
        if (lbaLimit < _items[i].Part.GetLimit())
            lbaLimit = _items[i].Part.GetLimit();
        const UInt64 lim = (UInt64)lbaLimit << _sectorSizeLog;
        if (lim < _totalSize) {
            CItem n;
            n.Part.Lba = lbaLimit;
            n.Offset = lim;
            n.Size = _totalSize - lim;
            n.IsReal = false;
            _items.Add(n);
        }
    }

#ifdef SEVENZIP_LINUX_MBR
    RINOK(ReadSubtypes(stream))
#endif
    FOR_VECTOR(i, _items) {
        CItem &item = _items[i];
        if (item.Part.Type != kType_Windows_NTFS)
            continue;
        if (InStream_SeekSet(stream, item.Part.GetPos(_sectorSizeLog)) != S_OK)
            continue;
        item.FileSystem = GetFileSystem(stream, item.Size);
        item.WasParsed = true;
    }

    _stream = stream;
    return S_OK;
}

Z7_COM7F_IMF(CHandler::Close()) {
    _totalSize = 0;
    _signature = 0;
    _sectorSizeLog = 0;
    _sectorSizeKnown = false;
    _numLogical = 0;
    _buffer.Free();
    _items.Clear();
    _stream.Release();
    return S_OK;
}

enum { kpidPrimary = kpidUserDefined, kpidBegChs, kpidEndChs };

static const CStatProp kProps[] = {
    {NULL, kpidPath, VT_BSTR},         {NULL, kpidSize, VT_UI8},
    {NULL, kpidFileSystem, VT_BSTR},   {NULL, kpidOffset, VT_UI8},
    {"Primary", kpidPrimary, VT_BOOL}, {"Begin CHS", kpidBegChs, VT_BSTR},
    {"End CHS", kpidEndChs, VT_BSTR}};

static const Byte kArcProps[] = {kpidSectorSize, kpidId};

IMP_IInArchive_Props_WITH_NAME IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)) {
    NCOM::CPropVariant prop;
    switch (propID) {
    case kpidMainSubfile: {
        int mainIndex = -1;
        FOR_VECTOR(i, _items)
        if (_items[i].IsReal) {
            if (mainIndex >= 0) {
                mainIndex = -1;
                break;
            }
            mainIndex = (int)i;
        }
        if (mainIndex >= 0)
            prop = (UInt32)(Int32)mainIndex;
        break;
    }
    case kpidPhySize:
        prop = _totalSize;
        break;
    case kpidSectorSize:
        prop = (UInt32)((UInt32)1 << _sectorSizeLog);
        break;
    case kpidId:
        prop = _signature;
        break;
    }
    prop.Detach(value);
    return S_OK;
}

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems)) {
    *numItems = _items.Size();
    return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    NCOM::CPropVariant prop;

    const CItem &item = _items[index];
    const CPartition &part = item.Part;
    switch (propID) {
    case kpidPath: {
        AString s;
        s.Add_UInt32(index);
        if (item.IsReal) {
            s.Add_Dot();
            const char *ext = NULL;
            if (item.FileSystem) {
                ext = "";
                AString fs(item.FileSystem);
                fs.MakeLower_Ascii();
                s += fs;
            } else if (!item.WasParsed) {
                const int typeIndex = FindPartType(part.Type);
                if (typeIndex >= 0)
                    ext = kPartTypes[(unsigned)typeIndex].Ext;
            }
            if (!ext)
                ext = "img";
            s += ext;
        }
        prop = s;
        break;
    }
    case kpidFileSystem:
        if (item.IsReal) {
            char s[32];
            ConvertUInt32ToString(part.Type, s);
            const char *res = s;
            if (item.FileSystem) {
                // strcat(s, "-");
                // strcpy(s, item.FileSystem);
                res = item.FileSystem;
            } else if (!item.WasParsed) {
                const int typeIndex = FindPartType(part.Type);
                if (typeIndex >= 0 && kPartTypes[(unsigned)typeIndex].Name)
                    res = kPartTypes[(unsigned)typeIndex].Name;
            }
            prop = res;
        }
        break;
    case kpidSize:
    case kpidPackSize:
        prop = item.Size;
        break;
    case kpidOffset:
        prop = item.Offset;
        break;
    case kpidPrimary:
        if (item.IsReal)
            prop = item.IsPrim;
        break;
    case kpidBegChs:
        if (item.IsReal)
            part.BeginChs.ToString(prop);
        break;
    case kpidEndChs:
        if (item.IsReal)
            part.EndChs.ToString(prop);
        break;
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

// 3, { 1, 1, 0 },
// 2, { 0x55, 0x1FF },

#if defined(SEVENZIP_LINUX_MBR) || !defined(Z7_EXTERNAL_CODECS)
REGISTER_ARC_I_NO_SIG("MBR", "mbr", NULL, 0xDB, 0, NArcInfoFlags::kPureStartOpen, NULL)
#endif

} // namespace NMbr
} // namespace NArchive

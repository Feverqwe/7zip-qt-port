// Archive/IsoIn.cpp

#include "7zip/Archive/Iso/StdAfx.h"

#include "platform/cpu/CpuArch.h"

#include "Common/MyException.h"

#include "7zip/Common/StreamUtils.h"

#include "7zip/Archive/HandlerCont.h"

#include "formats/iso/IsoIn.h"
#include "formats/iso/isonames.h"

namespace NArchive {
namespace NIso {

static const char *const kMediaTypes[] = {"NoEmul", "1.2M", "1.44M", "2.88M", "HardDisk"};

bool CBootInitialEntry::Parse(const Byte *p) {
    Bootable = (p[0] == NBootEntryId::kInitialEntryBootable);
    BootMediaType = p[1];
    LoadSegment = GetUi16(p + 2);
    SystemType = p[4];
    SectorCount = GetUi16(p + 6);
    LoadRBA = GetUi32(p + 8);
    memcpy(VendorSpec, p + 12, 20);
    if (p[5] != 0)
        return false;
    if (p[0] != NBootEntryId::kInitialEntryBootable &&
        p[0] != NBootEntryId::kInitialEntryNotBootable)
        return false;
    return true;
}

AString CBootInitialEntry::GetName() const {
    AString s(Bootable ? "Boot" : "NotBoot");
    s.Add_Minus();

    if (BootMediaType < Z7_ARRAY_SIZE(kMediaTypes))
        s += kMediaTypes[BootMediaType];
    else
        s.Add_UInt32(BootMediaType);

    if (VendorSpec[0] == 1) {
        // "Language and Version Information (IBM)"

        unsigned i;
        for (i = 1; i < sizeof(VendorSpec); i++)
            if (VendorSpec[i] > 0x7F)
                break;
        if (i == sizeof(VendorSpec)) {
            s.Add_Minus();
            for (i = 1; i < sizeof(VendorSpec); i++) {
                char c = (char)VendorSpec[i];
                if (c == 0)
                    break;
                if (c == '\\' || c == '/')
                    c = '_';
                s += c;
            }
        }
    }

    s += ".img";
    return s;
}

Byte CInArchive::ReadByte() {
    if (m_BufferPos == m_BufferSize) {
        m_BufferPos = 0;
        if (_position >= _fileSize)
            throw CUnexpectedEndException();
        const size_t requested = (size_t)MyMin((UInt64)kBlockSize, _fileSize - _position);
        size_t processed = requested;
        HRESULT res = ReadStream(_stream, m_Buffer, &processed);
        if (res != S_OK)
            throw CSystemException(res);
        if (processed != requested)
            throw CUnexpectedEndException();
        m_BufferSize = (unsigned)processed;
        _processedBytes += processed;
        if (_openCallback && _processedBytes - _processedBytes_prev >= (1 << 24)) {
            _processedBytes_prev = _processedBytes;
            const HRESULT progress = _openCallback->SetCompleted(&_numFiles, &_processedBytes);
            if (progress != S_OK)
                throw CSystemException(progress);
        }
        const UInt64 end = _position + processed;
        if (PhySize < end)
            PhySize = end;
    }
    Byte b = m_Buffer[m_BufferPos++];
    _position++;
    return b;
}

void CInArchive::ReadBytes(Byte *data, UInt32 size) {
    for (UInt32 i = 0; i < size; i++)
        data[i] = ReadByte();
}

void CInArchive::Skip(size_t size) {
    while (size-- != 0)
        ReadByte();
}

void CInArchive::SkipZeros(size_t size) {
    while (size-- != 0) {
        Byte b = ReadByte();
        if (b != 0)
            throw CHeaderErrorException();
    }
}

UInt16 CInArchive::ReadUInt16() {
    Byte b[4];
    ReadBytes(b, 4);
    UInt32 val = 0;
    for (int i = 0; i < 2; i++) {
        val |= ((UInt32)(b[i]) << (8 * i));
    }
    return (UInt16)val;
}

UInt32 CInArchive::ReadUInt32Le() {
    UInt32 val = 0;
    for (int i = 0; i < 4; i++)
        val |= ((UInt32)(ReadByte()) << (8 * i));
    return val;
}

UInt32 CInArchive::ReadUInt32Be() {
    UInt32 val = 0;
    for (int i = 0; i < 4; i++) {
        val <<= 8;
        val |= ReadByte();
    }
    return val;
}

UInt32 CInArchive::ReadUInt32() {
    Byte b[8];
    ReadBytes(b, 8);
    UInt32 val = 0;
    for (int i = 0; i < 4; i++) {
        val |= ((UInt32)(b[i]) << (8 * i));
    }
    return val;
}

unsigned CInArchive::ReadDigits(unsigned numDigits) {
    unsigned res = 0;
    unsigned mode = 0;
    for (unsigned i = 0; i < numDigits; i++) {
        const Byte b = ReadByte();
        if (mode == 2)
            continue;
        const unsigned d = (unsigned)b - (unsigned)'0';
        if (d > 9) {
            // what exact cases are possible here?
            if (b != 0 && b != ' ')
                throw CHeaderErrorException();
            if (mode == 1)
                mode = 2;
        } else {
            mode = 1;
            res *= 10;
            res += d;
        }
    }
    return res;
}

void CInArchive::ReadDateTime(CDateTime &d) {
    d = CDateTime();
    ReadBytes(d.Raw, sizeof(d.Raw));
}

void CInArchive::ReadBootRecordDescriptor(CBootRecordDescriptor &d) {
    ReadBytes(d.BootSystemId, sizeof(d.BootSystemId));
    ReadBytes(d.BootId, sizeof(d.BootId));
    ReadBytes(d.BootSystemUse, sizeof(d.BootSystemUse));
}

void CInArchive::ReadRecordingDateTime(CRecordingDateTime &t) {
    t.Year = ReadByte();
    t.Month = ReadByte();
    t.Day = ReadByte();
    t.Hour = ReadByte();
    t.Minute = ReadByte();
    t.Second = ReadByte();
    t.GmtOffset = (signed char)ReadByte();
}

void CInArchive::ReadDirRecord(CDirRecord &r, unsigned len, bool isJolietDescriptor) {
    r.RecordPosition = _position - 1;
    r.ExtendedAttributeRecordLen = ReadByte();
    r.ExtentLocation = ReadUInt32();
    r.Size = ReadUInt32();
    ReadRecordingDateTime(r.DateTime);
    r.FileFlags = ReadByte();
    r.FileUnitSize = ReadByte();
    r.InterleaveGapSize = ReadByte();
    r.VolSequenceNumber = ReadUInt16();
    unsigned idLen = ReadByte();
    /* for Descriptor Root Directory Record:
        idLen == 1 : by ISO Specification
        idLen == 1 : for some Joliet archives
        idLen == 2 : for some Joliet archives for UCS-2 (UTF-16BE).
          but (len == 34) is fixed for Descriptor Root Directory Record
          so we ignore idLen == 2 value in Joliet Descriptor.
    */
    if (isJolietDescriptor) // descriptor root identity comes from extent + XAR
        idLen = 0;
    if (len < 33 || idLen > len - 33)
        throw CHeaderErrorException();
    r.FileId.Alloc(idLen);
    ReadBytes((Byte *)r.FileId, idLen);
    idLen += 33;
    const unsigned padSize = idLen & 1;
    // SkipZeros(padSize);
    Skip(padSize); // it's bug in some cd's. Must be zeros
    idLen += padSize;
    if (len < idLen)
        throw CHeaderErrorException();
    len -= idLen;
    r.SystemUse.Alloc(len);
    ReadBytes((Byte *)r.SystemUse, len);
}

void CInArchive::ReadVolumeDescriptor(CVolumeDescriptor &d) {
    d.VolFlags = ReadByte();
    ReadBytes(d.SystemId, sizeof(d.SystemId));
    ReadBytes(d.VolumeId, sizeof(d.VolumeId));
    Skip(8);
    d.VolumeSpaceSize = ReadUInt32();
    ReadBytes(d.EscapeSequence, sizeof(d.EscapeSequence));
    d.VolumeSetSize = ReadUInt16();
    d.VolumeSequenceNumber = ReadUInt16();
    d.LogicalBlockSize = ReadUInt16();
    d.PathTableSize = ReadUInt32();
    d.LPathTableLocation = ReadUInt32Le();
    d.LOptionalPathTableLocation = ReadUInt32Le();
    d.MPathTableLocation = ReadUInt32Be();
    d.MOptionalPathTableLocation = ReadUInt32Be();
    {
        unsigned len;
        /* len = */ ReadByte();
        // len == 34 is expected by ISO specification
        // len == 36 is possible for some Joliet archives
        // len == 48 is possible for Rock Ridge?
        len = 34; // we ignore (len) value from header because
                  // next field (VolumeSetId) always starts after 34 bytes
        ReadDirRecord(d.RootDirRecord, len, true);
    }
    ReadBytes(d.VolumeSetId, sizeof(d.VolumeSetId));
    ReadBytes(d.PublisherId, sizeof(d.PublisherId));
    ReadBytes(d.DataPreparerId, sizeof(d.DataPreparerId));
    ReadBytes(d.ApplicationId, sizeof(d.ApplicationId));
    ReadBytes(d.CopyrightFileId, sizeof(d.CopyrightFileId));
    ReadBytes(d.AbstractFileId, sizeof(d.AbstractFileId));
    ReadBytes(d.BibFileId, sizeof(d.BibFileId));
    ReadDateTime(d.CTime);
    ReadDateTime(d.MTime);
    ReadDateTime(d.ExpirationTime);
    ReadDateTime(d.EffectiveTime);
    const Byte fileStructureVersion = ReadByte();
    // d.FileStructureVersion = fileStructureVersion;
    if (fileStructureVersion != 1 && // ECMA-119
        fileStructureVersion != 2)   // some ISO files have fileStructureVersion == 2.
    {
        // v24.05: we ignore that field, because we don't know what exact values are allowed there
        // throw CHeaderErrorException();
    }
    Skip(1); // Linux ignores the reserved descriptor byte.
    ReadBytes(d.ApplicationUse, sizeof(d.ApplicationUse));
    // Most ISO contain zeros in the following field (reserved for future standardization).
    // But some ISO programs write some data to that area.
    // So we disable check for zeros.
    Skip(653); // SkipZeros(653);
}

static const Byte kSig_CD001[5] = {'C', 'D', '0', '0', '1'};

/*
static const Byte kSig_NSR02[5] = { 'N', 'S', 'R', '0', '2' };
static const Byte kSig_NSR03[5] = { 'N', 'S', 'R', '0', '3' };
static const Byte kSig_BEA01[5] = { 'B', 'E', 'A', '0', '1' };
static const Byte kSig_TEA01[5] = { 'T', 'E', 'A', '0', '1' };
*/

static inline bool CheckSignature(const Byte *sig, const Byte *data) {
    for (int i = 0; i < 5; i++)
        if (sig[i] != data[i])
            return false;
    return true;
}

void CInArchive::SeekToBlock(UInt32 blockIndex) {
    SeekToPosition((UInt64)blockIndex * VolDescs[MainVolDescIndex].LogicalBlockSize);
}

void CInArchive::SeekToPosition(UInt64 position) {
    const HRESULT res = _stream->Seek((Int64)position, STREAM_SEEK_SET, &_position);
    if (res != S_OK)
        throw CSystemException(res);
    m_BufferPos = 0;
    m_BufferSize = 0;
}

bool CInArchive::CheckRecord(const CDirRecord &record) const {
    const UInt64 start = GetDataPosition(record);
    const UInt64 volume = VolDescs[MainVolDescIndex].Get_VolumeSpaceSize_inBytes();
    return start <= volume && record.Size <= volume - start && start <= _fileSize &&
           record.Size <= _fileSize - start;
}

bool CInArchive::CheckRef(const CRef &ref) const {
    if (!ref.Valid)
        return false;
    UInt64 offset = 0;
    for (UInt32 i = 0; i < ref.NumExtents; i++) {
        const CDirRecord &record = ref.Dir->_subItems[ref.Index + i];
        const UInt64 size = GetExtentSize(ref, i, offset);
        offset += size;
        if (size == record.Size) {
            if (!CheckRecord(record))
                return false;
        } else if (size) {
            CDirRecord part = record;
            part.Size = (UInt32)size;
            if (!CheckRecord(part))
                return false;
        }
    }
    return true;
}

UInt64 CInArchive::GetExtentSize(const CRef &ref, UInt32 index, UInt64 offset) const {
    const UInt64 size = ref.Dir->_subItems[ref.Index + index].Size;
    const CDir &item = ref.Dir->_subItems[ref.Index];
    if (_rockState.cruft && !item.IsDir() &&
        !(item.RockDefined && (item.RockInode.i_mode & 0170000) == 0120000)) {
        if (offset >= ref.TotalSize)
            return 0;
        if (size > ref.TotalSize - offset)
            return ref.TotalSize - offset;
    }
    return size;
}

bool CInArchive::IsSupported(const CRef &ref) const {
    const CDir &item = ref.Dir->_subItems[ref.Index];
    if (item.RockDefined) {
        const unsigned type = item.RockInode.i_mode & 0170000;
        if (type != 0100000 && type != 0120000 && type != 0040000)
            return false;
    }
    for (UInt32 i = 0; i < ref.NumExtents; i++)
        if (ref.Dir->_subItems[ref.Index + i].InterleaveGapSize)
            return false;
    return true;
}

unsigned CInArchive::DirSectionCount(const CDir &d, const CRef *sections) const {
    if (d.DirectoryRead)
        return d.DirectorySections.Size();
    return sections ? sections->NumExtents : 1;
}

const CDirRecord &CInArchive::DirSection(const CDir &d, const CRef *sections,
                                         unsigned index) const {
    if (d.DirectoryRead)
        return d.DirectorySections[index];
    return sections ? sections->Dir->_subItems[sections->Index + index] : d;
}

HRESULT CInArchive::ReadDirRecords(CDir &d, bool root, const CRef *sections) {
    if (!d.IsDir())
        return S_OK;
    if (_openCallback)
        RINOK(_openCallback->SetCompleted(&_numFiles, &_processedBytes))

    RINOK(ReadDirectory(d))
    const unsigned sectionCount = DirSectionCount(d, sections);
    for (unsigned section = 0; section < sectionCount; section++) {
        const CDirRecord &part = DirSection(d, sections, section);
        if (!CheckRecord(part) || part.InterleaveGapSize)
            return S_FALSE;
        FOR_VECTOR(i, UniqStartLocations)
        if (UniqStartLocations[i] == part.GetDataBlock()) {
            SelfLinkedDirs = true;
            return S_FALSE;
        }
        UniqStartLocations.Add(part.GetDataBlock());
    }

    for (unsigned section = 0; section < sectionCount; section++) {
        const CDirRecord &part = DirSection(d, sections, section);
        SeekToPosition(GetDataPosition(part));
        const UInt64 startPos = _position;
        bool firstItem = true;
        for (;;) {
            if (_processedBytes > _fileSize + (1 << 24)) {
                TooBigMetadata = true;
                return S_FALSE;
            }

            const UInt64 recordPos = _position;
            const UInt64 posInDir = recordPos - startPos;
            if (part.Size < posInDir)
                return S_FALSE;
            const UInt64 rem = part.Size - posInDir;
            if (rem == 0)
                break;
            const unsigned blockSize = VolDescs[MainVolDescIndex].LogicalBlockSize;
            const unsigned len = ReadByte();
            if (len == 0) {
                // it must have zero padding to end of block:
                while ((unsigned)_position & (blockSize - 1)) {
                    if (_position - startPos >= part.Size)
                        break;
                    if (ReadByte()) {
                        HeadersError = true;
                        return S_FALSE;
                    }
                }
                continue;
            }
            // record can't cross block range
            if (len < 33 || len > rem ||
                (((unsigned)recordPos & (blockSize - 1)) + len) > blockSize) {
                HeadersError = true;
                return S_FALSE;
            }

            CDir subItem;
            ReadDirRecord(subItem, len);
            if (firstItem && root && section == 0)
                IsSusp = !ReadOptions.NoRock && subItem.CheckSusp(SuspSkipSize);
            if (!subItem.IsSystemItem() &&
                !sevenzip_iso_visible(subItem.FileFlags, ReadOptions.Hide,
                                      ReadOptions.ShowAssociated)) {
                firstItem = false;
                continue;
            }
            if (!subItem.IsSystemItem()) {
                RINOK(ReadRock(subItem, subItem.IsDir()))
                if (subItem.RockHidden) {
                    firstItem = false;
                    continue;
                }
            }
            RINOK(ReadName(subItem))
            if (subItem.NameDefined && subItem.RockName.IsEmpty()) {
                firstItem = false;
                continue;
            }

            if (!subItem.IsSystemItem()) {
                if (_numFiles >= (1u << 30)) {
                    TooBigMetadata = true; // Too many files
                    return S_FALSE;
                }
                d._subItems.Add(subItem);
                _numFiles++;
            }
            firstItem = false;
        }
    }
    return S_OK;
}

HRESULT CInArchive::ReadDir(CDir &d, const unsigned level, const CRef *sections) {
    if (!d.IsDir())
        return S_OK;
    struct CFrame {
        CDir *Dir;
        CRef Sections;
        unsigned Next;
        bool HasSections, Entered, Root;
    };
    CRecordVector<CFrame> stack;
    CFrame first = {};
    first.Dir = &d;
    first.Next = 0;
    first.HasSections = sections != NULL;
    if (sections)
        first.Sections = *sections;
    first.Entered = false;
    first.Root = level == 0;
    stack.Add(first);
    while (!stack.IsEmpty()) {
        CFrame &frame = stack.Back();
        if (!frame.Entered) {
            RINOK(
                ReadDirRecords(*frame.Dir, frame.Root, frame.HasSections ? &frame.Sections : NULL))
            frame.Entered = true;
        }
        if (frame.Next < frame.Dir->_subItems.Size()) {
            const CRef ref = ReadExtentGroup(*frame.Dir, frame.Next);
            CDir &child = frame.Dir->_subItems[ref.Index];
            if (!child.IsDir())
                continue;
            if (!ref.Valid)
                return S_FALSE;
            CFrame next;
            next.Dir = &child;
            next.Sections = ref;
            next.Next = 0;
            next.HasSections = true;
            next.Entered = false;
            next.Root = false;
            stack.Add(next);
        } else {
            const unsigned count =
                DirSectionCount(*frame.Dir, frame.HasSections ? &frame.Sections : NULL);
            for (unsigned i = 0; i < count; ++i)
                UniqStartLocations.DeleteBack();
            stack.DeleteBack();
        }
    }
    return S_OK;
}

CRef CInArchive::ReadExtentGroup(CDir &d, unsigned &i) {
    CRef ref;
    CDir &subItem = d._subItems[i];
    subItem.Parent = &d;
    ref.Dir = &d;
    ref.Index = i++;
    ref.NumExtents = 1;
    ref.TotalSize = subItem.Size;
    if (subItem.RockDefined && (subItem.RockInode.i_mode & 0170000) == 0120000)
        ref.TotalSize = subItem.RockLink.Len();
    ref.Valid = true;
    if (subItem.IsNonFinalExtent()) {
        const unsigned blockSize = VolDescs[MainVolDescIndex].LogicalBlockSize;
        if (subItem.Size & (blockSize - 1)) {
            HeadersError = true;
            ref.Valid = false;
        }
        for (;;) {
            if (i == d._subItems.Size()) {
                HeadersError = true;
                ref.Valid = false;
                break;
            }
            const CDir &next = d._subItems[i];
            if (!subItem.AreMultiPartEqualWith(next)) {
                HeadersError = true;
                ref.Valid = false;
                break;
            }
            i++;
            ref.NumExtents++;
            ref.TotalSize += next.Size;
            if (!next.IsNonFinalExtent())
                break;
            if (next.Size & (blockSize - 1)) {
                HeadersError = true;
                ref.Valid = false;
            }
        }
    }
    if (_rockState.cruft && !subItem.IsDir() &&
        !(subItem.RockDefined && (subItem.RockInode.i_mode & 0170000) == 0120000))
        ref.TotalSize &= 0x00ffffff;
    return ref;
}

void CInArchive::CreateRefs(CDir &d) {
    if (!d.IsDir())
        return;
    struct CFrame {
        CDir *Dir;
        unsigned Next;
    };
    CRecordVector<CFrame> stack;
    CFrame first = {&d, 0};
    stack.Add(first);
    while (!stack.IsEmpty()) {
        CFrame &frame = stack.Back();
        if (frame.Next == frame.Dir->_subItems.Size()) {
            stack.DeleteBack();
            continue;
        }
        const CRef ref = ReadExtentGroup(*frame.Dir, frame.Next);
        Refs.Add(ref);
        CDir &child = frame.Dir->_subItems[ref.Index];
        if (child.IsDir()) {
            CFrame next = {&child, 0};
            stack.Add(next);
        }
    }
}

void CInArchive::ReadBootInfo() {
    if (!_bootIsDefined)
        return;
    HeadersError = true;

    if (memcmp(_bootDesc.BootSystemId, kElToritoSpec, sizeof(_bootDesc.BootSystemId)) != 0)
        return;

    UInt32 blockIndex = GetUi32(_bootDesc.BootSystemUse);
    SeekToPosition((UInt64)blockIndex * kBlockSize);

    Byte buf[32];
    ReadBytes(buf, 32);

    if (buf[0] != NBootEntryId::kValidationEntry || buf[2] != 0 || buf[3] != 0 || buf[30] != 0x55 ||
        buf[31] != 0xAA)
        return;

    {
        UInt32 sum = 0;
        for (unsigned i = 0; i < 32; i += 2)
            sum += GetUi16(buf + i);
        if ((sum & 0xFFFF) != 0)
            return;
        /*
        CBootValidationEntry e;
        e.PlatformId = buf[1];
        memcpy(e.Id, buf + 4, sizeof(e.Id));
        // UInt16 checkSum = GetUi16(p + 28);
        */
    }

    ReadBytes(buf, 32);
    {
        CBootInitialEntry e;
        if (!e.Parse(buf))
            return;
        BootEntries.Add(e);
    }

    bool error = false;

    for (;;) {
        ReadBytes(buf, 32);
        Byte headerIndicator = buf[0];
        if (headerIndicator != NBootEntryId::kMoreHeaders &&
            headerIndicator != NBootEntryId::kFinalHeader)
            break;

        // Section Header
        // Byte platform = p[1];
        unsigned numEntries = GetUi16(buf + 2);
        // id[28]

        for (unsigned i = 0; i < numEntries; i++) {
            ReadBytes(buf, 32);
            CBootInitialEntry e;
            if (!e.Parse(buf)) {
                error = true;
                break;
            }
            if (e.BootMediaType & (1 << 5)) {
                // Section entry extension
                for (unsigned j = 0;; j++) {
                    ReadBytes(buf, 32);
                    if (j > 32 || buf[0] != NBootEntryId::kExtensionIndicator) {
                        error = true;
                        break;
                    }
                    if ((buf[1] & (1 << 5)) == 0)
                        break;
                    // info += (buf + 2, 30)
                }
            }
            BootEntries.Add(e);
        }

        if (headerIndicator != NBootEntryId::kMoreHeaders)
            break;
    }

    HeadersError = error;
}

HRESULT CInArchive::Open2() {
    _position = 0;
    RINOK(InStream_GetSize_SeekToEnd(_stream, _fileSize))
    UInt64 descriptorBase = 0;
    descriptorBase = (UInt64)ReadOptions.SbSector;
    int primaryIndex = -1, jolietIndex = -1;
    bool descriptorsComplete = false;
    const UInt64 descriptorStart = descriptorBase * kBlockSize + kStartPos;
    if (_fileSize < descriptorStart)
        return S_FALSE;
    RINOK(_stream->Seek(descriptorStart, STREAM_SEEK_SET, &_position))

    PhySize = _position;
    m_BufferPos = 0;
    m_BufferSize = 0;
    // BlockSize = kBlockSize;

    // Linux examines volume descriptor sectors 16 through 99.
    for (unsigned descriptor = 16; descriptor < 100; descriptor++) {
        // Tree choice is frozen at the first Joliet descriptor. Retain the
        // archive's El Torito extension without parsing later volume metadata.
        if (descriptorsComplete && (descriptorBase + descriptor + 1) * kBlockSize > _fileSize)
            break;
        Byte sig[7];
        ReadBytes(sig, 7);
        if (descriptorsComplete) {
            const bool iso = CheckSignature(kSig_CD001, sig + 1);
            if (iso && sig[0] == NVolDescType::kTerminator)
                break;
            if (!iso || sig[0] != NVolDescType::kBootRecord) {
                Skip(kBlockSize - 7);
                continue;
            }
        }

        if (!CheckSignature(kSig_CD001, sig + 1)) {
            Skip(kBlockSize - sizeof(sig));
            continue;
        }

        if (sig[0] == NVolDescType::kTerminator) {
            break;
            // Skip(0x800 - 7);
            // continue;
        }

        switch (sig[0]) {
        case NVolDescType::kBootRecord: {
            _bootIsDefined = true;
            ReadBootRecordDescriptor(_bootDesc);
            break;
        }
        case NVolDescType::kPrimaryVol:
        case NVolDescType::kSupplementaryVol: {
            // some ISOs have two PrimaryVols.
            CVolumeDescriptor vd;
            ReadVolumeDescriptor(vd);
            if (sig[0] == NVolDescType::kPrimaryVol) {
                // some burners write "Joliet" Escape Sequence to primary volume
                memset(vd.EscapeSequence, 0, sizeof(vd.EscapeSequence));
                if (primaryIndex < 0)
                    primaryIndex = (int)VolDescs.Size();
            } else if (vd.EscapeSequence[0] == 0x25 && vd.EscapeSequence[1] == 0x2f) {
                if (!ReadOptions.NoJoliet && vd.IsJoliet())
                    jolietIndex = (int)VolDescs.Size();
                descriptorsComplete = true;
            }
            VolDescs.Add(vd);
            break;
        }
        default:
            Skip(kBlockSize - 7);
            break;
        }
    }

    if (VolDescs.IsEmpty())
        return S_FALSE;
    MainVolDescIndex = 0;
    RINOK(SelectRockVolume(primaryIndex, jolietIndex))
    /* FIXME: some volume can contain Rock Ridge, that is better than
       Joliet volume. So we need some way to detect such case */
    // MainVolDescIndex = 0; // to read primary volume
    const CVolumeDescriptor &vd = VolDescs[MainVolDescIndex];
    if (vd.LogicalBlockSize != 512 && vd.LogicalBlockSize != 1024 &&
        vd.LogicalBlockSize != kBlockSize)
        return S_FALSE;
    if (vd.VolumeSpaceSize == 0)
        return S_FALSE;

    {
        UpdatePhySize(0, vd.Get_VolumeSpaceSize_inBytes());
    }

    IsArc = true;

    (CDirRecord &)_rootDir = vd.RootDirRecord;
    _rootDir.FileFlags |= NFileFlags::kDirectory;
    RINOK(ReadDir(_rootDir, 0))
    CreateRefs(_rootDir);
    ReadBootInfo();

    {
        FOR_VECTOR(i, Refs) {
            const CRef &ref = Refs[i];
            UInt64 offset = 0;
            for (UInt32 j = 0; j < ref.NumExtents; j++) {
                const CDir &item = ref.Dir->_subItems[ref.Index + j];
                const UInt64 size = GetExtentSize(ref, j, offset);
                offset += size;
                if (!item.IsDir() && size != 0)
                    UpdatePhySize(GetDataPosition(item), size);
            }
        }
    }

    {
        // find boot item for expand:
        // UEFI Specification : 13.3.2.1. ISO-9660 and El Torito
        _expand_BootEntries_index = -1;
        FOR_VECTOR(i, BootEntries) {
            const CBootInitialEntry &be = BootEntries[i];
            if (be.SectorCount <= 1 && be.BootMediaType == NBootMediaType::kNoEmulation)
                if (_expand_BootEntries_index == -1 ||
                    be.LoadRBA >= BootEntries[_expand_BootEntries_index].LoadRBA)
                    _expand_BootEntries_index = (int)i;
        }
    }

    {
        FOR_VECTOR(i, BootEntries) {
            const CBootInitialEntry &be = BootEntries[i];
            UpdatePhySize((UInt64)be.LoadRBA * kBlockSize, GetBootItemSize(i));
        }
    }

    if (PhySize < _fileSize) {
        UInt64 rem = _fileSize - PhySize;
        const UInt64 kRemMax = 1 << 21;
        if (rem <= kRemMax) {
            RINOK(InStream_SeekSet(_stream, PhySize))
            bool areThereNonZeros = false;
            UInt64 numZeros = 0;
            RINOK(ReadZeroTail(_stream, areThereNonZeros, numZeros, kRemMax))
            if (!areThereNonZeros)
                PhySize += numZeros;
        }
    }

    return S_OK;
}

HRESULT CInArchive::Open(IInStream *inStream, IArchiveOpenCallback *openCallback) {
    Clear();
    _stream = inStream;
    _openCallback = openCallback;
    _numFiles = _processedBytes = _processedBytes_prev = 0;
    try {
        return Open2();
    } catch (const CSystemException &e) {
        return e.ErrorCode;
    } catch (CUnexpectedEndException &) {
        UnexpectedEnd = true;
        return S_FALSE;
    } catch (CHeaderErrorException &) {
        HeadersError = true;
        return S_FALSE;
    }
}

void CInArchive::Clear() {
    memset(&_rockState, 0, sizeof(_rockState));
    _rockState.rock_offset = -1;
    _rockState.nocompress = ReadOptions.NoCompress;
    _rockState.cruft = ReadOptions.Cruft;
    _rockResult = S_OK;
    IsArc = false;
    UnexpectedEnd = false;
    HeadersError = false;
    TooDeepDirs = false;
    TooBigMetadata = false;
    SelfLinkedDirs = false;

    UniqStartLocations.Clear();

    Refs.Clear();
    _rootDir.Clear();
    VolDescs.Clear();
    _bootIsDefined = false;
    BootEntries.Clear();
    SuspSkipSize = 0;
    IsSusp = false;

    _expand_BootEntries_index = -1;
}

UInt64 CInArchive::GetBootItemSize(unsigned index) const {
    const CBootInitialEntry &be = BootEntries[index];
    UInt64 size = be.GetSize();
    if (be.BootMediaType == NBootMediaType::k1d2Floppy)
        size = 1200 << 10;
    else if (be.BootMediaType == NBootMediaType::k1d44Floppy)
        size = 1440 << 10;
    else if (be.BootMediaType == NBootMediaType::k2d88Floppy)
        size = 2880 << 10;
    const UInt64 startPos = (UInt64)be.LoadRBA * kBlockSize;
    if (startPos < _fileSize) {
        const UInt64 rem = _fileSize - startPos;
        /*
           UEFI modification to ISO specification:
           because SectorCount is 16-bit, size is limited by (32 MB).
           UEFI Specification :
            13.3.2.1. ISO-9660 and El Torito
            If the value of Sector Count is set to 0 or 1,
            EFI will assume the system partition consumes the space
            from the beginning of the "no emulation" image to the end of the CD-ROM.
        */
        //
        if ((int)index == _expand_BootEntries_index || rem < size)
            size = rem;
    }
    return size;
}

} // namespace NIso
} // namespace NArchive

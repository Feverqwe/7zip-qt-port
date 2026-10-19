// SPDX-License-Identifier: GPL-2.0-only
// Stream/record boundary for the original Linux Rock Ridge C readers.
#include "../../../../isonames.h"
#include "../../../Common/StringConvert.h"
#include "../../Common/StreamUtils.h"
#include "../IArchive.h"
#include "IsoIn.h"
#include "StdAfx.h"
#include <errno.h>

namespace NArchive {
namespace NIso {
int CInArchive::RockRead(void *context, uint64_t offset, void *data, size_t size) {
    CInArchive &archive = *(CInArchive *)context;
    try {
        if (offset > archive._fileSize || size > archive._fileSize - offset)
            return -EIO;
        archive._rockResult = InStream_SeekSet(archive._stream, offset);
        if (archive._rockResult == S_OK)
            archive._rockResult = ReadStream_FALSE(archive._stream, data, size);
        // CE/PX/name readers can revisit a shared block several times per
        // entry. Keep these reads outside the original traversal-size budget;
        // Linux bounds continuation chains separately in rock_continue.
        if (archive._rockResult == S_OK && archive._openCallback)
            archive._rockResult =
                archive._openCallback->SetCompleted(&archive._numFiles, &archive._processedBytes);
    } catch (...) {
        archive._rockResult = E_FAIL;
    }
    return archive._rockResult == S_OK ? 0 : -EIO;
}

int CInArchive::RockRelocate(void *context, uint32_t block, sevenzip_iso_inode *output) {
    CInArchive &archive = *(CInArchive *)context;
    return sevenzip_iso_relocated(&archive._rockState, block, output);
}

HRESULT CInArchive::ReadRock(CDirRecord &item, bool namesOnly) {
    const CVolumeDescriptor &volume = VolDescs[MainVolDescIndex];
    _rockState.block_size = volume.LogicalBlockSize;
    _rockState.volume_blocks = volume.VolumeSpaceSize;
    _rockState.context = this;
    _rockState.read = RockRead;
    _rockState.relocate = RockRelocate;
    _rockResult = S_OK;
    sevenzip_iso_record record{};
    record.system_use = item.SystemUse;
    record.system_use_size = item.SystemUse.Size();
    record.flags = item.FileFlags;
    record.size = item.Size;
    record.block = item.GetDataBlock();
    record.record_block = (unsigned long)(item.RecordPosition / volume.LogicalBlockSize);
    record.record_offset = (unsigned long)(item.RecordPosition % volume.LogicalBlockSize);
    if (item.IsDir()) {
        record.record_block = (unsigned long)record.block;
        record.record_offset = 0;
    }
    memcpy(record.date, &item.DateTime, 7);
    char name[256], link[4096] = {};
    const int result =
        namesOnly ? sevenzip_iso_rock_name(&_rockState, &record, name)
                  : sevenzip_iso_rock(&_rockState, &record, 0, &item.RockInode, name, link);
    // Linux callbacks seek the shared input; restore the physical end of the
    // existing metadata buffer without changing its logical cursor.
    const HRESULT restored = InStream_SeekSet(_stream, _position + m_BufferSize - m_BufferPos);
    if (_rockResult != S_OK)
        return _rockResult;
    RINOK(restored)
    if (result < -1)
        return result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
    item.RockHidden = result == -1;
    if (item.RockHidden)
        return S_OK;
    item.RockDefined = _rockState.rock != 0;
    item.RockName = name;
    item.RockLink = link;
    if (item.RockDefined && !namesOnly) {
        const unsigned type = item.RockInode.i_mode & 0170000;
        switch (type) {
        case 0040000:
            item.FileFlags |= NFileFlags::kDirectory;
            item.ExtentLocation = item.RockInode.i_first_extent;
            item.ExtendedAttributeRecordLen = 0;
            item.Size = (UInt32)item.RockInode.i_size;
            break;
        case 0100000:
        case 0120000:
        case 0020000:
        case 0060000:
        case 0010000:
        case 0140000:
            item.FileFlags &= ~NFileFlags::kDirectory;
            break;
        default:
            return S_FALSE;
        }
    }
    return S_OK;
}

HRESULT CInArchive::ReadDirectory(CDir &directory) {
    if (directory.DirectoryRead)
        return S_OK;
    const CVolumeDescriptor &volume = VolDescs[MainVolDescIndex];
    const UInt64 block = directory.GetDataBlock();
    if (block >= volume.VolumeSpaceSize ||
        (volume.LogicalBlockSize != 512 && volume.LogicalBlockSize != 1024 &&
         volume.LogicalBlockSize != 2048))
        return S_FALSE;
    _rockState.block_size = volume.LogicalBlockSize;
    _rockState.volume_blocks = volume.VolumeSpaceSize;
    _rockState.context = this;
    _rockState.read = RockRead;
    _rockState.relocate = RockRelocate;
    _rockResult = S_OK;
    sevenzip_iso_section sections[SEVENZIP_ISO_MAX_SECTIONS];
    sevenzip_iso_inode inode{};
    unsigned count = 0;
    const int result =
        sevenzip_iso_directory(&_rockState, (uint32_t)block, _highSierra, &inode, sections, &count);
    const HRESULT restored = InStream_SeekSet(_stream, _position + m_BufferSize - m_BufferPos);
    if (_rockResult != S_OK)
        return _rockResult;
    RINOK(restored)
    if (result)
        return result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
    for (unsigned i = 0; i < count; ++i) {
        CDirRecord part = CDirRecord();
        if (sections[i].block >= volume.VolumeSpaceSize)
            return S_FALSE;
        part.ExtentLocation = (UInt32)sections[i].block;
        part.Size = sections[i].size;
        part.InterleaveGapSize = sections[i].gap;
        if (!CheckRecord(part) || part.InterleaveGapSize)
            return S_FALSE;
        directory.DirectorySections.Add(part);
    }
    directory.RockInode = inode;
    directory.RockDefined = true;
    directory.DirectoryRead = true;
    return S_OK;
}

HRESULT CInArchive::ReadName(CDirRecord &item) {
    if (item.IsSystemItem())
        return S_OK;
    if (item.RockDefined && !item.RockName.IsEmpty()) {
        item.NameDefined = true;
        return S_OK;
    }
    const bool rockNames =
        item.RockDefined && (_rockState.rock == 1 || _rockState.rock_offset >= 0);
    if (!IsJoliet() && !rockNames && !NameMapping)
        return S_OK;
    char name[1024];
    const int length = sevenzip_iso_name(item.FileId, item.FileId.Size(), item.SystemUse,
                                         item.SystemUse.Size(), item.FileFlags, IsJoliet(),
                                         NameMapping ? NameMapping : 'n', NameCharset, name);
    if (length < 0)
        return S_FALSE;
    item.RockName.SetFrom(name, (unsigned)length);
    if (!IsJoliet())
        item.RockName =
            UnicodeStringToMultiByte(MultiByteToUnicodeString(item.RockName, CP_OEMCP), CP_UTF8);
    item.NameDefined = true;
    return S_OK;
}

HRESULT CInArchive::SelectRockVolume() {
    const int joliet = MainVolDescIndex;
    MainVolDescIndex = 0;
    const CVolumeDescriptor &primary = VolDescs[0];
    _rockState.rock = 2;
    _rockState.rock_offset = -1;
    HRESULT result = S_FALSE;
    bool primaryIsDir = false;
    try {
        if ((primary.LogicalBlockSize == 512 || primary.LogicalBlockSize == 1024 ||
             primary.LogicalBlockSize == 2048) &&
            primary.RootDirRecord.GetDataBlock() < primary.VolumeSpaceSize) {
            CDir root;
            (CDirRecord &)root = primary.RootDirRecord;
            root.FileFlags |= NFileFlags::kDirectory;
            result = ReadDirectory(root);
            primaryIsDir = result == S_OK;
        }
    } catch (CUnexpectedEndException &) {
        result = S_FALSE;
    } catch (CHeaderErrorException &) {
        result = S_FALSE;
    }
    if (result != S_OK && result != S_FALSE)
        return result;
    const bool hasJoliet = joliet > 0 && VolDescs[joliet].IsJoliet();
    const int choice =
        isofs_fill_super(&_rockState, result == S_OK ? 0 : -EIO, hasJoliet,
                         hasJoliet && VolDescs[joliet].RootDirRecord.GetDataBlock() !=
                                          primary.RootDirRecord.GetDataBlock(),
                         (uint32_t)primary.RootDirRecord.GetDataBlock());
    if (_rockResult != S_OK)
        return _rockResult;
    if (choice < 0 || (choice == 0 && !primaryIsDir))
        return S_FALSE;
    if (choice == 1) {
        MainVolDescIndex = joliet;
        _rockState.rock = 0;
        _rockState.rock_offset = -1;
    }
    return S_OK;
}
} // namespace NIso
} // namespace NArchive

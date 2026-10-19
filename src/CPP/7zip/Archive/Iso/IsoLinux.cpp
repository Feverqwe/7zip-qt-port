// SPDX-License-Identifier: GPL-2.0-only
// Stream/record boundary for the original Linux Rock Ridge C readers.
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

HRESULT CInArchive::ReadRock(CDirRecord &item) {
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
    char name[256], link[4096];
    const int result = sevenzip_iso_rock(&_rockState, &record, 0, &item.RockInode, name, link);
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
    if (item.RockDefined) {
        if (item.RockName.IsEmpty()) {
            item.RockName.SetFrom((const char *)(const Byte *)item.FileId,
                                  (unsigned)item.FileId.Size());
            if (item.RockName.Len() >= 2 && item.RockName[item.RockName.Len() - 2] == ';' &&
                item.RockName.Back() == '1')
                item.RockName.DeleteFrom(item.RockName.Len() - 2);
            if (!item.RockName.IsEmpty() && item.RockName.Back() == '.')
                item.RockName.DeleteBack();
        }
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
            CheckRecord(primary.RootDirRecord)) {
            SeekToPosition(GetDataPosition(primary.RootDirRecord));
            const unsigned len = ReadByte();
            if (len >= 34) {
                CDirRecord root;
                ReadDirRecord(root, len);
                result = ReadRock(root);
                primaryIsDir = root.IsDir();
            }
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

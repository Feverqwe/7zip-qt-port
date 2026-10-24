// SPDX-License-Identifier: GPL-2.0
// Native archive stream/callback services, shared with the retained file streams.
#include "btrfsarchivereader.h"
#include "7zip/Common/StreamUtils.h"
#include "Common/IntToString.h"
#include "Common/MyString.h"
#include <array>
#include <cerrno>
#include <cstring>

int BtrfsArchiveReader::Read(void *context, uint64_t id, const uint8_t *uuid, uint64_t offset,
                             void *data, size_t size) {
    auto &self = *static_cast<BtrfsArchiveReader *>(context);
    if (self.callbackStatus_ != S_OK)
        return -ECANCELED;
    Device *device = &self.primary_;
    if (id && id != device->info.devid) {
        auto found = self.devices_.find(id);
        if (found == self.devices_.end())
            return -ENODEV;
        device = &found->second;
    }
    const UInt64 base = device == &self.primary_ ? self.base_ : 0;
    if ((id && memcmp(uuid, device->info.uuid, 16)) || offset > device->size ||
        size > device->size - offset || offset > UINT64_MAX - base) {
        self.transport_ = S_FALSE;
        return -EIO;
    }
    try {
        self.transport_ = InStream_SeekSet(device->stream, base + offset);
        if (self.transport_ == S_OK)
            self.transport_ = ReadStream_FALSE(device->stream, data, size);
    } catch (const std::bad_alloc &) {
        self.callbackStatus_ = E_OUTOFMEMORY;
        return -ENOMEM;
    } catch (...) {
        self.transport_ = E_FAIL;
    }
    return self.transport_ == S_OK ? 0 : -EIO;
}

int BtrfsArchiveReader::Poll(void *context) {
    auto &self = *static_cast<BtrfsArchiveReader *>(context);
    if (self.callbackStatus_ != S_OK)
        return -ECANCELED;
    try {
        if (self.openCallback)
            self.callbackStatus_ = self.openCallback->SetCompleted(&self.files, nullptr);
        else if (self.extractCallback)
            self.callbackStatus_ = self.extractCallback->SetCompleted(&self.completed);
    } catch (...) {
        self.callbackStatus_ = E_FAIL;
    }
    return self.callbackStatus_ == S_OK ? 0 : -ECANCELED;
}

int BtrfsArchiveReader::OpenDevice(void *context, uint64_t id, const uint8_t *uuid) {
    auto &self = *static_cast<BtrfsArchiveReader *>(context);
    if (self.callbackStatus_ != S_OK)
        return -ECANCELED;
    if (id == self.primary_.info.devid)
        return memcmp(uuid, self.primary_.info.uuid, 16) ? -EINVAL : 0;
    auto found = self.devices_.find(id);
    if (found != self.devices_.end())
        return memcmp(uuid, found->second.info.uuid, 16) ? -EINVAL : 0;
    if (!self.volumes_ || self.missing_.count(id))
        return -ENODEV;
    try {
        wchar_t digits[32];
        ConvertUInt64ToString(id, digits);
        UString name(L"devid-");
        name += digits;
        name += L".img";
        Device device;
        const HRESULT result = self.volumes_->GetStream(name, &device.stream);
        if (result == E_ABORT || result == E_OUTOFMEMORY) {
            self.callbackStatus_ = result;
            return result == E_ABORT ? -ECANCELED : -ENOMEM;
        }
        if (result != S_OK || !device.stream) {
            self.missing_.insert(id);
            return -ENODEV;
        }
        std::array<Byte, 4096> super;
        if (InStream_GetSize_SeekToEnd(device.stream, device.size) != S_OK ||
            InStream_SeekSet(device.stream, 65536) != S_OK ||
            ReadStream_FALSE(device.stream, super.data(), super.size()) != S_OK ||
            btrfs_inspect_device(super.data(), super.size(), &device.info) ||
            device.info.devid != id || memcmp(uuid, device.info.uuid, 16) ||
            memcmp(device.info.fsid, self.primary_.info.fsid, 16) ||
            memcmp(device.info.metadata_uuid, self.primary_.info.metadata_uuid, 16)) {
            self.missing_.insert(id);
            return -ENODEV;
        }
        self.devices_.emplace(id, std::move(device));
        return 0;
    } catch (const std::bad_alloc &) {
        self.callbackStatus_ = E_OUTOFMEMORY;
        return -ENOMEM;
    } catch (...) {
        self.callbackStatus_ = E_FAIL;
        return -ECANCELED;
    }
}

HRESULT BtrfsArchiveReader::Result(int error) {
    lastError_ = error;
    if (callbackStatus_ != S_OK)
        return callbackStatus_;
    if (!error)
        return S_OK;
    if (transport_ != S_OK)
        return transport_;
    return error == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
}

HRESULT BtrfsArchiveReader::Open(IInStream *stream, IArchiveOpenCallback *callback) {
    std::lock_guard lock(mutex);
    primary_.stream = stream;
    openCallback = callback;
    struct Reset {
        BtrfsArchiveReader &reader;
        ~Reset() {
            reader.openCallback = nullptr;
            reader.volumes_.Release();
        }
    } reset{*this};
    RINOK(stream->Seek(0, STREAM_SEEK_CUR, &base_))
    UInt64 size;
    RINOK(InStream_GetSize_SeekToEnd(stream, size))
    if (size < base_ || size - base_ < 65536 + 4096)
        return S_FALSE;
    primary_.size = size - base_;
    std::array<Byte, 4096> super;
    RINOK(InStream_SeekSet(stream, base_ + 65536))
    RINOK(ReadStream_FALSE(stream, super.data(), super.size()))
    if (btrfs_inspect_device(super.data(), super.size(), &primary_.info))
        return S_FALSE;
    if (const char *reason = btrfs_read_limitation(primary_.info.incompat)) {
        openError = reason;
        return S_FALSE;
    }
    physicalSize = primary_.info.bytes;
    if (callback)
        callback->QueryInterface(IID_IArchiveOpenVolumeCallback,
                                 reinterpret_cast<void **>(&volumes_));
    btrfs_source source{
        this, primary_.size, Read, OpenDevice, Poll, btrfs_checksum_native, btrfs_decode_native};
    btrfs_volume *volume = nullptr;
    const int error = btrfs_open_volume(&source, &volume);
    volume_.reset(volume);
    RINOK(Result(error))
    btrfs_get_volume_info(volume_.get(), &info);
    BtrfsFile root;
    RINOK(OpenFile(rootId, root))
    btrfs_file_info rootInfo;
    btrfs_get_file_info(root.get(), &rootInfo);
    rootId = BtrfsFileId(rootInfo);
    return S_OK;
}

HRESULT BtrfsArchiveReader::OpenFile(BtrfsId id, BtrfsFile &file) {
    std::lock_guard lock(mutex);
    btrfs_file *raw = nullptr;
    const int error = btrfs_open_file(volume_.get(), id.first, id.second, &raw);
    file.reset(raw);
    return Result(error);
}

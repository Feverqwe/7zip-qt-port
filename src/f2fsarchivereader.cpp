// SPDX-License-Identifier: MIT
#include "f2fsarchivereader.h"
#include "7zip/Common/StreamUtils.h"
#include <limits>

int F2fsArchiveReader::Read(void *context, uint64_t offset, void *data, size_t size) {
    auto &self = *static_cast<F2fsArchiveReader *>(context);
    try {
        if (self.transport_ == S_OK)
            self.transport_ = self.input_->Seek(Int64(offset), STREAM_SEEK_SET, nullptr);
        if (self.transport_ == S_OK)
            self.transport_ = ReadStream_FALSE(self.input_, data, size);
    } catch (...) {
        self.transport_ = E_FAIL;
    }
    return self.transport_ == S_OK ? 0 : F2FS_IO;
}
int F2fsArchiveReader::Poll(void *context) {
    auto &self = *static_cast<F2fsArchiveReader *>(context);
    try {
        if (self.transport_ == S_OK && self.opening)
            self.transport_ = self.opening->SetCompleted(&self.files, nullptr);
        if (self.transport_ == S_OK && self.extracting)
            self.transport_ = self.extracting->SetCompleted(&self.completed);
    } catch (...) {
        self.transport_ = E_FAIL;
    }
    return self.transport_ == S_OK ? 0 : F2FS_IO;
}
HRESULT F2fsArchiveReader::Result(int error) const {
    if (transport_ != S_OK)
        return transport_;
    if (error == F2FS_MEMORY)
        return E_OUTOFMEMORY;
    if (error == F2FS_UNSUPPORTED)
        return E_NOTIMPL;
    return error ? S_FALSE : S_OK;
}
HRESULT F2fsArchiveReader::Open(IInStream *input, IArchiveOpenCallback *callback) {
    input_ = input;
    opening = callback;
    struct ResetOpening {
        F2fsArchiveReader &self;
        ~ResetOpening() { self.opening = nullptr; }
    } reset{*this};
    UInt64 size;
    RINOK(input_->Seek(0, STREAM_SEEK_END, &size))
    if (size > UInt64(INT64_MAX))
        return S_FALSE;
    f2fs_source source{this, size, Read, Poll};
    f2fs_volume *volume = nullptr;
    const int error = f2fs_open_volume(&source, &volume);
    volume_.reset(volume);
    return Result(error);
}
HRESULT F2fsArchiveReader::OpenFile(UInt32 ino, F2fsFile &file) {
    f2fs_file *opened = nullptr;
    const int error = f2fs_open_file(volume_.get(), ino, &opened);
    file.reset(opened);
    return Result(error);
}

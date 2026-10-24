// SPDX-License-Identifier: GPL-3.0-or-later
#include "xfsarchivereader.h"
#include "7zip/Common/StreamUtils.h"
#include <algorithm>
#include <limits>

int XfsArchiveReader::ReadSource(void *context, uint64_t offset, void *data, size_t size) {
    auto &self = *static_cast<XfsArchiveReader *>(context);
    if (self.transport_ != S_OK)
        return 1;
    self.transport_ = InStream_SeekSet(self.source_, offset);
    if (self.transport_ == S_OK)
        self.transport_ = ReadStream_FALSE(self.source_, data, size);
    return self.transport_ == S_OK ? 0 : 1;
}
int XfsArchiveReader::Poll(void *context) {
    auto &self = *static_cast<XfsArchiveReader *>(context);
    if (self.transport_ != S_OK)
        return 1;
    if (self.opening)
        self.transport_ = self.opening->SetCompleted(&self.files, nullptr);
    else if (self.extracting)
        self.transport_ = self.extracting->SetCompleted(&self.completed);
    return self.transport_ == S_OK ? 0 : 1;
}
HRESULT XfsArchiveReader::Result(int error) const {
    if (transport_ != S_OK)
        return transport_;
    if (error == XFS_NO_MEMORY)
        return E_OUTOFMEMORY;
    if (error == XFS_UNSUPPORTED)
        return E_NOTIMPL;
    return error ? S_FALSE : S_OK;
}
HRESULT XfsArchiveReader::Open(IArchiveOpenCallback *callback) {
    UInt64 size;
    RINOK(InStream_GetSize_SeekToEnd(source_, size))
    if (size > UInt64(INT64_MAX))
        return S_FALSE;
    opening = callback;
    struct Reset {
        IArchiveOpenCallback *&callback;
        ~Reset() { callback = nullptr; }
    } reset{opening};
    xfs_source source{this, size, ReadSource, Poll};
    xfs_volume *opened = nullptr;
    const int error = xfs_open(&source, &opened);
    volume.reset(opened);
    return Result(error);
}
HRESULT XfsArchiveReader::List(UInt64 inode, xfs_visit visit, void *context) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    transport_ = S_OK;
    return Result(xfs_list(volume.get(), inode, visit, context));
}
HRESULT XfsArchiveReader::Read(UInt64 inode, UInt64 offset, void *buffer, size_t size,
                               size_t *processed) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    transport_ = S_OK;
    return Result(xfs_read(volume.get(), inode, offset, buffer, size, processed));
}

// SPDX-License-Identifier: GPL-3.0-or-later
#include "xfsarchivereader.h"
#include "7zip/Common/StreamUtils.h"
#include <algorithm>
#include <limits>
#include <new>
#include <string_view>

#ifdef SEVENZIP_USE_GRUB_XFS_MODULE
import sevenzip.dependencies.grub_xfs;
#else
#include "dependencies/grub_xfs.h"
#endif

int XfsArchiveReader::read_source(uint64_t offset, std::span<std::byte> data) noexcept {
    if (transport_ != S_OK)
        return 1;
    try {
        transport_ = InStream_SeekSet(source_, offset);
        if (transport_ == S_OK)
            transport_ = ReadStream_FALSE(source_, data.data(), data.size());
    } catch (const std::bad_alloc &) {
        transport_ = E_OUTOFMEMORY;
    } catch (...) {
        transport_ = E_FAIL;
    }
    return transport_ == S_OK ? 0 : 1;
}
int XfsArchiveReader::checkpoint() noexcept {
    if (transport_ != S_OK)
        return 1;
    try {
        if (opening)
            transport_ = opening->SetCompleted(&files, nullptr);
        else if (extracting)
            transport_ = extracting->SetCompleted(&completed);
    } catch (const std::bad_alloc &) {
        transport_ = E_OUTOFMEMORY;
    } catch (...) {
        transport_ = E_FAIL;
    }
    return transport_ == S_OK ? 0 : 1;
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
    auto opened = sevenzip::grub_xfs::open(*this, size);
    if (!opened) {
        volume.reset();
        return Result(opened.error());
    }
    volume = std::move(*opened);
    return Result(0);
}
HRESULT XfsArchiveReader::List(UInt64 inode, xfs_visit visit, void *context) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    transport_ = S_OK;
    auto visitor = [this, visit, context](std::string_view name, const xfs_entry &entry) noexcept {
        try {
            return visit(context, name.data(), &entry);
        } catch (const std::bad_alloc &) {
            transport_ = E_OUTOFMEMORY;
        } catch (...) {
            transport_ = E_FAIL;
        }
        return 1;
    };
    const auto result = sevenzip::grub_xfs::list(*volume, inode, visitor);
    return Result(result ? 0 : result.error());
}
HRESULT XfsArchiveReader::Read(UInt64 inode, UInt64 offset, void *buffer, size_t size,
                               size_t *processed) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    transport_ = S_OK;
    if (!processed)
        return Result(XFS_BAD_FS);
    *processed = 0;
    if (!buffer && size)
        return Result(XFS_RANGE);
    const auto result =
        sevenzip::grub_xfs::read(*volume, inode, offset, {static_cast<std::byte *>(buffer), size});
    *processed = result ? *result : result.error().processed;
    return Result(result ? 0 : result.error().code);
}

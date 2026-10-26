// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef SEVENZIP_XFS_ARCHIVE_READER_H
#define SEVENZIP_XFS_ARCHIVE_READER_H
#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "xfsreader.h"
#include <cstddef>
#include <memory>
#include <mutex>
#include <span>

class XfsArchiveReader {
    CMyComPtr<IInStream> source_;
    HRESULT transport_ = S_OK;

  public:
    int read_source(uint64_t, std::span<std::byte>) noexcept;
    int checkpoint() noexcept;
    std::recursive_mutex mutex;
    std::unique_ptr<xfs_volume, decltype(&xfs_close)> volume{nullptr, xfs_close};
    IArchiveOpenCallback *opening = nullptr;
    IArchiveExtractCallback *extracting = nullptr;
    UInt64 completed = 0, files = 0;
    explicit XfsArchiveReader(IInStream *source) : source_(source) {}
    HRESULT Open(IArchiveOpenCallback *);
    HRESULT Result(int error) const;
    HRESULT List(UInt64, xfs_visit, void *);
    HRESULT Read(UInt64, UInt64, void *, size_t, size_t *);
};
#endif

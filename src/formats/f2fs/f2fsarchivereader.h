// SPDX-License-Identifier: MIT
#ifndef SEVENZIP_F2FS_ARCHIVE_READER_H
#define SEVENZIP_F2FS_ARCHIVE_READER_H
#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "formats/f2fs/f2fsreader.h"
#include <memory>
#include <mutex>
struct F2fsFileDeleter {
    void operator()(f2fs_file *file) const { f2fs_close_file(file); }
};
using F2fsFile = std::unique_ptr<f2fs_file, F2fsFileDeleter>;
class F2fsArchiveReader final {
    CMyComPtr<IInStream> input_;
    std::unique_ptr<f2fs_volume, decltype(&f2fs_close_volume)> volume_{nullptr, f2fs_close_volume};
    static int Read(void *, uint64_t, void *, size_t);
    static int Poll(void *);
    HRESULT transport_ = S_OK;

  public:
    std::recursive_mutex mutex;
    IArchiveOpenCallback *opening = nullptr;
    IArchiveExtractCallback *extracting = nullptr;
    UInt64 files = 0, completed = 0;
    HRESULT Open(IInStream *, IArchiveOpenCallback *);
    UInt32 Root() const { return f2fs_root(volume_.get()); }
    HRESULT OpenFile(UInt32, F2fsFile &);
    HRESULT Result(int) const;
    void Reset() { transport_ = S_OK; }
};
#endif

// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_EROFS_ARCHIVE_READER_H
#define SEVENZIP_EROFS_ARCHIVE_READER_H
#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "erofsreader.h"
#include <memory>
#include <vector>

struct ErofsFileDeleter {
    void operator()(erofs_file *file) const { erofs_close_file(file); }
};
using ErofsFile = std::unique_ptr<erofs_file, ErofsFileDeleter>;

class ErofsArchiveReader final {
    static int Read(void *, unsigned int, uint64_t, void *, size_t);
    static int Poll(void *);
    static int Decode(void *, unsigned int, unsigned int, const void *, size_t, void *, size_t,
                      int);
    static int OpenDevice(void *, unsigned int, const char *, uint64_t);
    std::vector<CMyComPtr<IInStream>> streams_;
    CMyComPtr<IArchiveOpenVolumeCallback> volumes_;
    std::unique_ptr<erofs_volume, decltype(&erofs_close_volume)> volume_{nullptr,
                                                                         erofs_close_volume};
    UInt64 base_ = 0;
    HRESULT transport_ = S_OK;
    bool missingDevice_ = false;

  public:
    IArchiveOpenCallback *openCallback = nullptr;
    IArchiveExtractCallback *extractCallback = nullptr;
    UInt64 files = 0, completed = 0;
    erofs_volume_info info{};
    HRESULT Open(IInStream *, IArchiveOpenCallback *);
    HRESULT OpenFile(uint64_t nid, ErofsFile &);
    HRESULT Result(int) const;
    void ResetTransport() { transport_ = S_OK; }
};
#endif

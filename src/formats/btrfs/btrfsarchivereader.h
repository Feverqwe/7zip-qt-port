// SPDX-License-Identifier: GPL-2.0
#ifndef SEVENZIP_BTRFS_ARCHIVE_READER_H
#define SEVENZIP_BTRFS_ARCHIVE_READER_H
#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Common/MyString.h"
#include "formats/btrfs/btrfsreader.h"
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
using BtrfsId = std::pair<uint64_t, uint64_t>;
inline BtrfsId BtrfsFileId(const btrfs_file_info &info) { return {info.root, info.ino}; }
struct BtrfsFileDeleter {
    void operator()(btrfs_file *file) const { btrfs_close_file(file); }
};
using BtrfsFile = std::unique_ptr<btrfs_file, BtrfsFileDeleter>;

class BtrfsArchiveReader final {
    struct Device {
        CMyComPtr<IInStream> stream;
        btrfs_device_info info{};
        UInt64 size = 0;
    } primary_;
    static int Read(void *, uint64_t, const uint8_t *, uint64_t, void *, size_t);
    static int Poll(void *);
    static int OpenDevice(void *, uint64_t, const uint8_t *);
    std::map<uint64_t, Device> devices_;
    std::set<uint64_t> missing_;
    CMyComPtr<IArchiveOpenVolumeCallback> volumes_;
    std::unique_ptr<btrfs_volume, decltype(&btrfs_close_volume)> volume_{nullptr,
                                                                         btrfs_close_volume};
    UInt64 base_ = 0;
    HRESULT transport_ = S_OK, callbackStatus_ = S_OK;
    int lastError_ = 0;

  public:
    std::recursive_mutex mutex;
    IArchiveOpenCallback *openCallback = nullptr;
    IArchiveExtractCallback *extractCallback = nullptr;
    UInt64 files = 0, completed = 0, physicalSize = 0;
    btrfs_volume_info info{};
    UString openError;
    BtrfsId rootId{5, 0};
    HRESULT Open(IInStream *, IArchiveOpenCallback *);
    HRESULT OpenFile(BtrfsId, BtrfsFile &);
    HRESULT Result(int);
    int LastError() const { return lastError_; }
    void ResetTransport() {
        transport_ = callbackStatus_ = S_OK;
        lastError_ = 0;
    }
};
#endif

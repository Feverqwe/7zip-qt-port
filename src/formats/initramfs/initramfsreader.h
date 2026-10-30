// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_INITRAMFS_READER_H
#define SEVENZIP_INITRAMFS_READER_H
#include "7zip/Archive/IArchive.h"
struct InitramfsRecord {
    UInt64 dataPosition, size, storedSize;
    bool isNew, isTrailer;
};
using InitramfsReadRecord = HRESULT (*)(IInStream *, UInt64, InitramfsRecord &);
HRESULT NormalizeInitramfs(IInStream *source, IArchiveOpenCallback *callback,
                           InitramfsReadRecord readRecord, IInStream **normalized,
                           UInt64 &physicalSize);
#endif

// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_ISO_ZISOFS_STREAM_H
#define SEVENZIP_ISO_ZISOFS_STREAM_H
#include "7zip/IStream.h"
#include "isoreader.h"
namespace NArchive {
namespace NIso {
HRESULT CreateZisofsStream(IInStream *, UInt64, unsigned, const sevenzip_iso_inode &,
                           ISequentialInStream **);
}
} // namespace NArchive
#endif

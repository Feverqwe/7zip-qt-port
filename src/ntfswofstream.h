// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_NTFS_WOF_STREAM_H
#define SEVENZIP_NTFS_WOF_STREAM_H
#include "7zip/IStream.h"
HRESULT SevenzipNtfsWofStream(IInStream *ads, UInt64 size, UInt64 validSize, UInt64 adsSize,
                              unsigned bits, IInStream **stream);
#endif

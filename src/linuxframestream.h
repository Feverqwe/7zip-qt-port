// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_LINUX_FRAME_STREAM_H
#define SEVENZIP_LINUX_FRAME_STREAM_H
#include "7zip/ICoder.h"
#include "7zip/IStream.h"
HRESULT SevenzipDecodeLinuxFrame(bool lzo, IInStream *source, UInt64 offset, UInt64 sourceSize,
                                 ISequentialOutStream *output, ICompressProgressInfo *progress,
                                 UInt64 &consumed, UInt64 &produced);
#endif

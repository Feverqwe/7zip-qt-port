// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_LZ4_FRAME_STREAM_H
#define SEVENZIP_LZ4_FRAME_STREAM_H
#include "7zip/ICoder.h"
#include "7zip/IStream.h"
bool SevenzipIsLz4FrameMagic(const Byte *bytes);
HRESULT SevenzipDecodeLz4Frame(IInStream *source, UInt64 offset, UInt64 sourceSize,
                               ISequentialOutStream *output, ICompressProgressInfo *progress,
                               UInt64 &consumed, UInt64 &produced, Int32 &operationResult);
#endif

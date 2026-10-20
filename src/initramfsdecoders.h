// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_INITRAMFS_DECODERS_H
#define SEVENZIP_INITRAMFS_DECODERS_H
#include "7zip/ICoder.h"
#include "7zip/IStream.h"

enum class InitramfsMethod { None, Gzip, Bzip2, Lzma, Xz, Lzo, Lz4, Zstd };
InitramfsMethod InitramfsCompression(const Byte *data, size_t size);
HRESULT DecodeInitramfsFrame(InitramfsMethod method, IInStream *source, UInt64 offset,
                             UInt64 sourceSize, ISequentialOutStream *output,
                             ICompressProgressInfo *progress, UInt64 &consumed);
#endif

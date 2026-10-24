// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEVENZIP_SQUASHFS_ZLIB_H
#define SEVENZIP_SQUASHFS_ZLIB_H

#include "7zip/IStream.h"

HRESULT SquashfsDecodeZlib(ISequentialInStream *input, ISequentialOutStream *output,
                           UInt32 outputLimit);

#endif

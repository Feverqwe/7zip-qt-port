// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_INITRAMFS_CACHE_H
#define SEVENZIP_INITRAMFS_CACHE_H
#include "7zip/IStream.h"
HRESULT CreateInitramfsCache(IInStream **input, ISequentialOutStream **output);
#endif

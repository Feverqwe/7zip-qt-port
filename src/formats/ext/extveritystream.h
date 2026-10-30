// SPDX-License-Identifier: MIT
#pragma once
#include "7zip/IStream.h"
class CExtVerityPolicy;

HRESULT CreateExtVerityStream(IInStream *raw, UInt64 fileSize, unsigned blockBits,
                              UInt32 allocatedEnd, const CExtVerityPolicy &, IInStream **result);

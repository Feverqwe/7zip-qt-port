// SPDX-License-Identifier: MIT
#pragma once
#include "7zip/IStream.h"

HRESULT CreateExtVerityStream(IInStream *raw, UInt64 fileSize, unsigned blockBits,
                              UInt32 allocatedEnd, IInStream **result);

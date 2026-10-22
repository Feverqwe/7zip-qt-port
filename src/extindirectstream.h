// SPDX-License-Identifier: MIT
#pragma once
#include "extfscrypt.h"
#include "extreader.h"

HRESULT CreateExtIndirectStream(const std::shared_ptr<ext_volume> &, IInStream *, const Byte *refs,
                                UInt32 inode, unsigned blockBits, UInt64 size,
                                bool requireAllocated, const CExtCryptInode &, IInStream **);
using ExtIndirectRangeCallback = HRESULT (*)(void *, UInt32 logical, UInt64 physical, UInt32 count);
// S_FALSE retains a stream and the ranges confirmed before the failed lookup.
HRESULT CreateExtIndirectDirectoryStream(const std::shared_ptr<ext_volume> &, IInStream *,
                                         const Byte *refs, UInt32 inode, unsigned blockBits,
                                         UInt64 size, void *, ExtIndirectRangeCallback,
                                         IInStream **);

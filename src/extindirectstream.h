// SPDX-License-Identifier: MIT
#pragma once
#include "extfscrypt.h"
#include "extreader.h"

HRESULT CreateExtIndirectStream(const std::shared_ptr<ext_volume> &, IInStream *, const Byte *refs,
                                UInt32 inode, unsigned blockBits, UInt64 size,
                                bool requireAllocated, const CExtCryptInode &, IInStream **);

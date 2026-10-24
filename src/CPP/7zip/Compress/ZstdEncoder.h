// (C) 2016 - 2018 Tino Reichardt. LGPL-2.1-or-later.
// ZIP adapter from 7-Zip-zstd; see docs/zip-zstd-adapter-baseline.json.
#pragma once

#include "../../Common/MyCom.h"
#include "../ICoder.h"

#define ZSTD_STATIC_LINKING_ONLY
#include "../../../zstd/lib/zstd.h"

namespace NCompress {
namespace NZSTD {
Z7_CLASS_IMP_COM_3(CEncoder, ICompressCoder, ICompressSetCoderProperties,
                   ICompressSetCoderPropertiesOpt)
ZSTD_CCtx *_ctx = nullptr;
void *_srcBuf = nullptr;
void *_dstBuf = nullptr;
size_t _srcBufSize;
size_t _dstBufSize;
int _Level = ZSTD_CLEVEL_DEFAULT;
UInt64 unpackSize = 0;

public:
CEncoder();
~CEncoder();
};
} // namespace NZSTD
} // namespace NCompress

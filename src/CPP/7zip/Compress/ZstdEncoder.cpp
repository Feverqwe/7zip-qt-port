// (C) 2016 - 2020 Tino Reichardt. LGPL-2.1-or-later.
// CEncoder adapted from 7-Zip-zstd. Compression remains official libzstd C.
#include "StdAfx.h"

#include "../../../C/Alloc.h"
#include "../../../zstd/lib/zstd_errors.h"
#include "../Common/StreamUtils.h"
#include "ZstdEncoder.h"
#include <limits.h>

namespace NCompress {
namespace NZSTD {
CEncoder::CEncoder() : _srcBufSize(ZSTD_CStreamInSize()), _dstBufSize(ZSTD_CStreamOutSize()) {}

CEncoder::~CEncoder() {
    ZSTD_freeCCtx(_ctx);
    MyFree(_srcBuf);
    MyFree(_dstBuf);
}

Z7_COM7F_IMF(CEncoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props,
                                          UInt32 numProps)) {
    _Level = ZSTD_CLEVEL_DEFAULT;
    for (UInt32 i = 0; i < numProps; ++i) {
        const PROPVARIANT &prop = props[i];
        switch (propIDs[i]) {
        case NCoderPropID::kLevel:
            if (prop.vt != VT_UI4 || prop.ulVal > (UInt32)ZSTD_maxCLevel())
                return E_INVALIDARG;
            _Level = prop.ulVal ? (int)prop.ulVal : ZSTD_CLEVEL_DEFAULT;
            break;
        case NCoderPropID::kNumThreads:
            // ZIP's existing worker pool parallelizes files; each coder is serial.
            if (prop.vt != VT_UI4)
                return E_INVALIDARG;
            break;
        case NCoderPropID::kReduceSize:
            if (prop.vt != VT_UI8)
                return E_INVALIDARG;
            break;
        default:
            return E_INVALIDARG;
        }
    }
    return S_OK;
}

Z7_COM7F_IMF(CEncoder::SetCoderPropertiesOpt(const PROPID *propIDs, const PROPVARIANT *props,
                                             UInt32 numProps)) {
    unpackSize = 0;
    for (UInt32 i = 0; i < numProps; ++i)
        if (propIDs[i] == NCoderPropID::kExpectedDataSize && props[i].vt == VT_UI8)
            unpackSize = props[i].uhVal.QuadPart;
    return S_OK;
}

Z7_COM7F_IMF(CEncoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
                            const UInt64 *, const UInt64 *, ICompressProgressInfo *progress)) {
    if (!_ctx)
        _ctx = ZSTD_createCCtx();
    if (!_srcBuf)
        _srcBuf = MyAlloc(_srcBufSize);
    if (!_dstBuf)
        _dstBuf = MyAlloc(_dstBufSize);
    if (!_ctx || !_srcBuf || !_dstBuf)
        return E_OUTOFMEMORY;

    // A ZIP worker reuses its coder for subsequent entries and Store retries.
    size_t err = ZSTD_CCtx_reset(_ctx, ZSTD_reset_session_and_parameters);
    if (ZSTD_isError(err))
        return E_FAIL;
    err = ZSTD_CCtx_setParameter(_ctx, ZSTD_c_compressionLevel, _Level);
    if (ZSTD_isError(err))
        return E_INVALIDARG;
    if (unpackSize && unpackSize != (UInt64)(Int64)-1) {
        err = ZSTD_CCtx_setParameter(_ctx, ZSTD_c_srcSizeHint,
                                     (int)(unpackSize <= INT_MAX ? unpackSize : INT_MAX));
        if (ZSTD_isError(err))
            return E_INVALIDARG;
    }

    UInt64 processedIn = 0, processedOut = 0;
    for (;;) {
        if (progress) {
            RINOK(progress->SetRatioInfo(&processedIn, &processedOut))
        }
        size_t srcSize = _srcBufSize;
        RINOK(ReadStream(inStream, _srcBuf, &srcSize))
        const ZSTD_EndDirective todo = srcSize ? ZSTD_e_continue : ZSTD_e_end;
        processedIn += srcSize;
        ZSTD_inBuffer input{_srcBuf, srcSize, 0};
        for (;;) {
            ZSTD_outBuffer output{_dstBuf, _dstBufSize, 0};
            err = ZSTD_compressStream2(_ctx, &output, &input, todo);
            if (ZSTD_isError(err)) {
                switch ((unsigned)ZSTD_getErrorCode(err)) {
                case ZSTD_error_memory_allocation:
                    return E_OUTOFMEMORY;
                case ZSTD_error_version_unsupported:
                case ZSTD_error_frameParameter_unsupported:
                    return E_NOTIMPL;
                case ZSTD_error_frameParameter_windowTooLarge:
                case ZSTD_error_parameter_unsupported:
                case ZSTD_error_parameter_outOfBound:
                    return E_INVALIDARG;
                default:
                    return E_FAIL;
                }
            }
            if (output.pos) {
                RINOK(WriteStream(outStream, _dstBuf, output.pos))
                processedOut += output.pos;
            }
            if (progress) {
                RINOK(progress->SetRatioInfo(&processedIn, &processedOut))
            }
            if (todo == ZSTD_e_end && err == 0)
                return S_OK;
            if (todo == ZSTD_e_continue && input.pos == input.size)
                break;
        }
    }
}
} // namespace NZSTD
} // namespace NCompress

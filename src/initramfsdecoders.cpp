// SPDX-License-Identifier: GPL-2.0-only
// Single-member stream boundaries around the original SDK decoders.
#include "initramfsdecoders.h"
#include "../third_party/7zip/C/Alloc.h"
#include "../third_party/7zip/C/ZstdDec.h"
#include "7zip/Common/CWrappers.h"
#include "7zip/Common/LimitedStreams.h"
#include "7zip/Common/StreamUtils.h"
#include "7zip/Compress/BZip2Decoder.h"
#include "C/Xz.h"
#include "linuxframestream.h"
#include <algorithm>
#include <memory>

namespace NArchive {
namespace NGz {
HRESULT DecodeInitramfsFrame(ISequentialInStream *, ISequentialOutStream *, ICompressProgressInfo *,
                             UInt64 &);
}
namespace NLzma {
HRESULT DecodeInitramfsFrame(ISequentialInStream *, ISequentialOutStream *, ICompressProgressInfo *,
                             UInt64 &);
}
} // namespace NArchive

InitramfsMethod InitramfsCompression(const Byte *p, size_t size) {
    // Linux lib/decompress.c::decompress_method dispatch, including its old
    // gzip marker (the actual gzip reader still validates the full header).
    if (size < 2)
        return InitramfsMethod::None;
    struct Format {
        Byte first, second;
        InitramfsMethod method;
    };
    static const Format formats[] = {
        {0x1f, 0x8b, InitramfsMethod::Gzip},  {0x1f, 0x9e, InitramfsMethod::Gzip},
        {0x42, 0x5a, InitramfsMethod::Bzip2}, {0x5d, 0x00, InitramfsMethod::Lzma},
        {0xfd, 0x37, InitramfsMethod::Xz},    {0x89, 0x4c, InitramfsMethod::Lzo},
        {0x02, 0x21, InitramfsMethod::Lz4},   {0x28, 0xb5, InitramfsMethod::Zstd}};
    for (const auto &format : formats)
        if (p[0] == format.first && p[1] == format.second)
            return format.method;
    return InitramfsMethod::None;
}

namespace {
HRESULT DecodeBzip2(ISequentialInStream *input, ISequentialOutStream *output,
                    ICompressProgressInfo *progress, UInt64 &consumed) {
    CMyComPtr2_Create<ICompressCoder, NCompress::NBZip2::CDecoder> decoder;
    decoder->FinishMode = true;
    decoder->Base.DecodeAllStreams = false;
    RINOK(decoder.Interface()->Code(input, output, nullptr, nullptr, progress))
    if (decoder->GetCrcError() || decoder->Base.NeedMoreInput || decoder->Base.NumStreams != 1)
        return S_FALSE;
    consumed = decoder->Base.FinishedPackSize;
    return S_OK;
}

struct XzState {
    CXzUnpacker decoder;
    XzState() {
        XzUnpacker_Construct(&decoder, &g_Alloc);
        XzUnpacker_Init(&decoder);
    }
    ~XzState() { XzUnpacker_Free(&decoder); }
};

HRESULT DecodeXz(ISequentialInStream *input, ISequentialOutStream *output,
                 ICompressProgressInfo *progress, UInt64 &consumed) {
    XzState state;
    Byte in[1 << 15], out[1 << 15];
    size_t position = 0, available = 0;
    UInt64 written = 0;
    bool eof = false;
    for (;;) {
        if (progress)
            RINOK(progress->SetRatioInfo(&consumed, &written))
        if (position == available && !eof) {
            available = sizeof(in);
            RINOK(ReadStream(input, in, &available))
            position = 0;
            eof = available < sizeof(in);
        }
        // decodeOnlyOneBlock stops before consuming the following header.
        // Bound metadata calls so a footer cannot enter the next XZ stream.
        size_t used = available - position;
        state.decoder.decodeOnlyOneBlock = state.decoder.state == XZ_STATE_BLOCK;
        if (state.decoder.state != XZ_STATE_BLOCK)
            used = std::min<size_t>(used, 1);
        size_t produced = sizeof(out);
        ECoderStatus status;
        const auto beforeState = state.decoder.state;
        const SRes result = XzUnpacker_Code(&state.decoder, out, &produced, in + position, &used,
                                            eof, CODER_FINISH_ANY, &status);
        position += used;
        consumed += used;
        RINOK(WriteStream(output, out, produced))
        written += produced;
        if (result != SZ_OK)
            return SResToHRESULT(result);
        if (state.decoder.numFinishedStreams == 1)
            return XzUnpacker_IsStreamWasFinished(&state.decoder) ? S_OK : S_FALSE;
        if (used == 0 && produced == 0 && state.decoder.state == beforeState)
            return S_FALSE;
    }
}

HRESULT DecodeZstd(ISequentialInStream *input, ISequentialOutStream *output,
                   ICompressProgressInfo *progress, UInt64 &consumed) {
    // Linux unzstd uses ZSTD_WINDOWLOG_MAX; its old 128 MB comment is stale.
    constexpr UInt64 maxWindowSize = UInt64(1) << (sizeof(size_t) == 4 ? 30 : 31);
    const auto deleter = [](CZstdDec *decoder) { ZstdDec_Destroy(decoder); };
    std::unique_ptr<CZstdDec, decltype(deleter)> decoder(
        ZstdDec_Create(&g_AlignedAlloc, &g_BigAlloc), deleter);
    if (!decoder)
        return E_OUTOFMEMORY;
    CZstdDecState state;
    ZstdDecState_Clear(&state);
    ZstdDec_Init(decoder.get());
    Byte bytes[1 << 15];
    state.inBuf = bytes;
    bool eof = false;
    for (;;) {
        if (progress)
            RINOK(progress->SetRatioInfo(&consumed, &state.outProcessed))
        if (state.inPos == state.inLim && !eof) {
            state.inPos = 0;
            state.inLim = sizeof(bytes);
            RINOK(ReadStream(input, bytes, &state.inLim))
            eof = state.inLim < sizeof(bytes);
        }
        const size_t before = state.inPos;
        const SRes result = ZstdDec_Decode(decoder.get(), &state);
        consumed += state.inPos - before;
        const size_t produced = state.winPos - state.wrPos;
        if (produced) {
            RINOK(WriteStream(output, state.win + state.wrPos, produced))
            state.wrPos = state.winPos;
        }
        if (result != SZ_OK)
            return SResToHRESULT(result);
        if (state.info.windowSize_MAX > maxWindowSize)
            return E_NOTIMPL;
        if (state.status == ZSTD_STATUS_FINISHED_FRAME) {
            CZstdDecResInfo info;
            ZstdDec_GetResInfo(decoder.get(), &state, result, &info);
            if (info.decode_SRes != SZ_OK || info.is_NonFinishedFrame || info.extraSize > consumed)
                return S_FALSE;
            consumed -= info.extraSize;
            return S_OK;
        }
        if (state.inPos == before && produced == 0)
            return S_FALSE;
    }
}
} // namespace

HRESULT DecodeInitramfsFrame(InitramfsMethod method, IInStream *source, UInt64 offset,
                             UInt64 sourceSize, ISequentialOutStream *output,
                             ICompressProgressInfo *progress, UInt64 &consumed) {
    consumed = 0;
    if (offset > sourceSize || sourceSize > INT64_MAX)
        return E_INVALIDARG;
    if (method == InitramfsMethod::Lzo || method == InitramfsMethod::Lz4) {
        UInt64 produced = 0;
        return SevenzipDecodeLinuxFrame(method == InitramfsMethod::Lzo, source, offset, sourceSize,
                                        output, progress, consumed, produced);
    }
    RINOK(InStream_SeekSet(source, offset))
    CMyComPtr2_Create<ISequentialInStream, CLimitedSequentialInStream> limited;
    limited->SetStream(source);
    limited->Init(sourceSize - offset);
    HRESULT result;
    switch (method) {
    case InitramfsMethod::Gzip:
        result = NArchive::NGz::DecodeInitramfsFrame(limited, output, progress, consumed);
        break;
    case InitramfsMethod::Bzip2:
        result = DecodeBzip2(limited, output, progress, consumed);
        break;
    case InitramfsMethod::Lzma:
        result = NArchive::NLzma::DecodeInitramfsFrame(limited, output, progress, consumed);
        break;
    case InitramfsMethod::Xz:
        result = DecodeXz(limited, output, progress, consumed);
        break;
    case InitramfsMethod::Zstd:
        result = DecodeZstd(limited, output, progress, consumed);
        break;
    default:
        return S_FALSE;
    }
    RINOK(result)
    if (consumed == 0 || consumed > sourceSize - offset)
        return S_FALSE;
    return InStream_SeekSet(source, offset + consumed);
}

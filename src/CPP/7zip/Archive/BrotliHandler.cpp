// BrotliHandler.cpp — adapted from 7-Zip-zstd (Tino Reichardt), LGPL-2.1-or-later.
// Raw RFC 7932, extraction only; see docs/upstream-port-audit/brotli.md.
#include "7zip/Archive/Common/DummyOutStream.h"
#include "7zip/Common/ProgressUtils.h"
#include "7zip/Common/RegisterArc.h"
#include "7zip/Common/StreamUtils.h"
#include "CPP/7zip/Compress/BrotliDecoder.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include <brotli/decode.h>
#include <memory>

using namespace NWindows;
namespace NArchive {
namespace NBROTLI {
// clang-format off
Z7_CLASS_IMP_CHandler_IInArchive_1(IArchiveOpenSeq)
    CMyComPtr<IInStream> _stream;
    CMyComPtr<ISequentialInStream> _seqStream;
    bool _packSize_Defined = false;
    bool _unpackSize_Defined = false;
    bool _dataAfterEnd = false;
    bool _needMoreInput = false;
    UInt64 _packSize = 0;
    UInt64 _unpackSize = 0;
};
// clang-format on

static const Byte kProps[] = {kpidSize, kpidPackSize};
static const Byte kArcProps[] = {kpidErrorFlags};
IMP_IInArchive_Props IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)) {
    NCOM::CPropVariant prop;
    if (propID == kpidErrorFlags) {
        UInt32 flags = 0;
        if (_dataAfterEnd)
            flags |= kpv_ErrorFlags_DataAfterEnd;
        if (_needMoreInput)
            flags |= kpv_ErrorFlags_UnexpectedEnd;
        prop = flags;
    }
    prop.Detach(value);
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems)) {
    *numItems = 1;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetProperty(UInt32, PROPID propID, PROPVARIANT *value)) {
    NCOM::CPropVariant prop;
    switch (propID) {
    case kpidPackSize:
        if (_packSize_Defined)
            prop = _packSize;
        break;
    case kpidSize:
        if (_unpackSize_Defined)
            prop = _unpackSize;
        break;
    }
    prop.Detach(value);
    return S_OK;
}

API_FUNC_static_IsArc IsArc_Brotli(const Byte *p, size_t size) {
    if (size == 0)
        return k_IsArc_Res_NEED_MORE;
    std::unique_ptr<BrotliDecoderState, decltype(&BrotliDecoderDestroyInstance)> state(
        BrotliDecoderCreateInstance(nullptr, nullptr, nullptr), BrotliDecoderDestroyInstance);
    if (!state)
        return k_IsArc_Res_NO;
    const uint8_t *nextIn = p;
    size_t availableIn = size;
    uint8_t output[256];
    uint8_t *nextOut = output;
    size_t availableOut = sizeof(output);
    const auto result = BrotliDecoderDecompressStream(state.get(), &availableIn, &nextIn,
                                                      &availableOut, &nextOut, nullptr);
    return result == BROTLI_DECODER_RESULT_ERROR ? k_IsArc_Res_NO : k_IsArc_Res_YES;
}
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback)) {
    COM_TRY_BEGIN
    Close();
    Byte input[1 << 12];
    size_t size = sizeof(input);
    RINOK(ReadStream(stream, input, &size))
    if (IsArc_Brotli(input, size) != k_IsArc_Res_YES)
        return S_FALSE;
    if (callback) {
        RINOK(callback->SetCompleted(nullptr, nullptr))
    }
    RINOK(InStream_GetSize_SeekToEnd(stream, _packSize))
    RINOK(InStream_SeekToBegin(stream))
    _packSize_Defined = true;
    _stream = stream;
    _seqStream = stream;
    return S_OK;
    COM_TRY_END
}
Z7_COM7F_IMF(CHandler::OpenSeq(ISequentialInStream *stream)) {
    Close();
    _seqStream = stream;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::Close()) {
    _packSize_Defined = _unpackSize_Defined = _dataAfterEnd = _needMoreInput = false;
    _packSize = _unpackSize = 0;
    _seqStream.Release();
    _stream.Release();
    return S_OK;
}
Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems, Int32 testMode,
                               IArchiveExtractCallback *extractCallback)) {
    COM_TRY_BEGIN
    if (numItems == 0)
        return S_OK;
    if (numItems != (UInt32)(Int32)-1 && (numItems != 1 || !indices || indices[0] != 0))
        return E_INVALIDARG;
    if (!_seqStream)
        return E_FAIL;
    if (_stream) {
        RINOK(InStream_SeekToBegin(_stream))
    }
    _unpackSize_Defined = _dataAfterEnd = _needMoreInput = false;
    if (_packSize_Defined) {
        RINOK(extractCallback->SetTotal(_packSize))
    }
    CMyComPtr<ISequentialOutStream> realOutStream;
    const Int32 askMode = testMode ? NExtract::NAskMode::kTest : NExtract::NAskMode::kExtract;
    RINOK(extractCallback->GetStream(0, &realOutStream, askMode))
    if (!testMode && !realOutStream)
        return S_OK;
    RINOK(extractCallback->PrepareOperation(askMode))
    NCompress::NBROTLI::CDecoder *decoderSpec = new NCompress::NBROTLI::CDecoder;
    CMyComPtr<ICompressCoder> decoder = decoderSpec;
    CLocalProgress *lps = new CLocalProgress;
    CMyComPtr<ICompressProgressInfo> progress = lps;
    lps->Init(extractCallback, true);
    // Test mode discards output through the original dummy stream.
    CDummyOutStream *outStreamSpec = new CDummyOutStream;
    CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
    outStreamSpec->SetStream(realOutStream);
    outStreamSpec->Init();
    realOutStream.Release();
    const HRESULT result = decoder->Code(_seqStream, outStream, nullptr, nullptr, progress);
    if (result != S_OK && result != S_FALSE)
        return result;
    _dataAfterEnd = decoderSpec->DataAfterEnd;
    _needMoreInput = decoderSpec->NeedMoreInput;
    Int32 opRes;
    if (_needMoreInput)
        opRes = NExtract::NOperationResult::kUnexpectedEnd;
    else if (_dataAfterEnd)
        opRes = NExtract::NOperationResult::kDataAfterEnd;
    else if (result == S_FALSE)
        opRes = NExtract::NOperationResult::kDataError;
    else {
        _unpackSize = decoderSpec->_processedOut;
        _unpackSize_Defined = true;
        opRes = NExtract::NOperationResult::kOK;
    }
    outStream.Release();
    return extractCallback->SetOperationResult(opRes);
    COM_TRY_END
}

REGISTER_ARC_I_NO_SIG("brotli", "br brotli", nullptr, 0x1F, 0,
                      NArcInfoFlags::kKeepName | NArcInfoFlags::kPureStartOpen |
                          NArcInfoFlags::kByExtOnlyOpen,
                      IsArc_Brotli)
} // namespace NBROTLI
} // namespace NArchive

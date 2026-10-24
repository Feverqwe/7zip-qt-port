// SPDX-License-Identifier: GPL-2.0-only
// Single-stream archive boundary adapted from BrotliHandler.cpp (LGPL-2.1-or-later).
// Modern frame parsing and decoding remain the official LZ4 C library.
// See docs/upstream-port-audit/lz4-frame.md.
#include "7zip/Archive/Common/DummyOutStream.h"
#include "7zip/Common/ProgressUtils.h"
#include "7zip/Common/RegisterArc.h"
#include "7zip/Common/StreamUtils.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "lz4framestream.h"
#include <cstring>

using namespace NWindows;
namespace NArchive {
namespace NLz4Frame {
// Length-prefixed signatures: normal Frame plus all sixteen skippable magics.
static const Byte kSignatures[] = {
    4, 4,    0x22, 0x4d, 0x18, 4, 0x50, 0x2a, 0x4d, 0x18, 4, 0x51, 0x2a, 0x4d, 0x18,
    4, 0x52, 0x2a, 0x4d, 0x18, 4, 0x53, 0x2a, 0x4d, 0x18, 4, 0x54, 0x2a, 0x4d, 0x18,
    4, 0x55, 0x2a, 0x4d, 0x18, 4, 0x56, 0x2a, 0x4d, 0x18, 4, 0x57, 0x2a, 0x4d, 0x18,
    4, 0x58, 0x2a, 0x4d, 0x18, 4, 0x59, 0x2a, 0x4d, 0x18, 4, 0x5a, 0x2a, 0x4d, 0x18,
    4, 0x5b, 0x2a, 0x4d, 0x18, 4, 0x5c, 0x2a, 0x4d, 0x18, 4, 0x5d, 0x2a, 0x4d, 0x18,
    4, 0x5e, 0x2a, 0x4d, 0x18, 4, 0x5f, 0x2a, 0x4d, 0x18};

Z7_CLASS_IMP_CHandler_IInArchive_0 CMyComPtr<IInStream> _stream;
UInt64 _packSize = 0, _unpackSize = 0;
UInt32 _errorFlags = 0;
bool _sizeDefined = false;
};

static const Byte kProps[] = {kpidSize, kpidPackSize};
static const Byte kArcProps[] = {kpidErrorFlags};
IMP_IInArchive_Props IMP_IInArchive_ArcProps

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)) {
    NCOM::CPropVariant prop;
    if (propID == kpidErrorFlags)
        prop = _stream ? _errorFlags : UInt32(kpv_ErrorFlags_IsNotArc);
    else if (propID == kpidPhySize && _stream)
        prop = _packSize;
    prop.Detach(value);
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems)) {
    *numItems = _stream ? 1 : 0;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) {
    if (!_stream || index)
        return E_INVALIDARG;
    NCOM::CPropVariant prop;
    if (propID == kpidPackSize)
        prop = _packSize;
    else if (propID == kpidSize && _sizeDefined)
        prop = _unpackSize;
    prop.Detach(value);
    return S_OK;
}
Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback)) {
    COM_TRY_BEGIN
    Close();
    if (!stream)
        return E_INVALIDARG;
    UInt64 size;
    RINOK(InStream_AtBegin_GetSize(stream, size))
    if (size > INT64_MAX)
        return S_FALSE;
    if (callback) {
        RINOK(callback->SetTotal(nullptr, &size))
        const UInt64 zero = 0;
        RINOK(callback->SetCompleted(nullptr, &zero))
    }
    Byte header[4];
    size_t count = sizeof(header);
    RINOK(ReadStream(stream, header, &count))
    if (count != sizeof(header) || !SevenzipIsLz4FrameMagic(header))
        return S_FALSE;
    if (callback) {
        const UInt64 completed = count;
        RINOK(callback->SetCompleted(nullptr, &completed))
    }
    _packSize = size;
    _stream = stream;
    return S_OK;
    COM_TRY_END
}
Z7_COM7F_IMF(CHandler::Close()) {
    _stream.Release();
    _packSize = _unpackSize = 0;
    _errorFlags = 0;
    _sizeDefined = false;
    return S_OK;
}
Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems, Int32 testMode,
                               IArchiveExtractCallback *extractCallback)) {
    COM_TRY_BEGIN
    if (numItems == 0)
        return S_OK;
    if (numItems != UINT32_MAX && (numItems != 1 || !indices || indices[0]))
        return E_INVALIDARG;
    if (!_stream)
        return E_FAIL;
    _unpackSize = 0;
    _sizeDefined = false;
    _errorFlags = 0;
    RINOK(extractCallback->SetTotal(_packSize))
    CMyComPtr<ISequentialOutStream> realOut;
    const Int32 askMode = testMode ? NExtract::NAskMode::kTest : NExtract::NAskMode::kExtract;
    RINOK(extractCallback->GetStream(0, &realOut, askMode))
    if (!testMode && !realOut)
        return S_OK;
    RINOK(extractCallback->PrepareOperation(askMode))
    CMyComPtr2_Create<ISequentialOutStream, CDummyOutStream> output;
    output->SetStream(realOut);
    output->Init();
    realOut.Release();
    CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> progress;
    progress->Init(extractCallback, true);
    UInt64 offset = 0;
    Int32 op = NExtract::NOperationResult::kOK;
    for (;;) {
        UInt64 consumed;
        Int32 frameResult;
        const HRESULT result = SevenzipDecodeLz4Frame(_stream, offset, _packSize, output, progress,
                                                      consumed, _unpackSize, frameResult);
        if (result != S_OK)
            return result;
        if (frameResult != NExtract::NOperationResult::kOK) {
            op = frameResult;
            _errorFlags = op == NExtract::NOperationResult::kUnexpectedEnd
                              ? kpv_ErrorFlags_UnexpectedEnd
                              : kpv_ErrorFlags_DataError;
            break;
        }
        offset += consumed;
        if (offset == _packSize)
            break;
        RINOK(InStream_SeekSet(_stream, offset))
        Byte next[4];
        size_t size = sizeof(next);
        RINOK(ReadStream(_stream, next, &size))
        if (size != sizeof(next) || !SevenzipIsLz4FrameMagic(next)) {
            op = NExtract::NOperationResult::kDataAfterEnd;
            _errorFlags = kpv_ErrorFlags_DataAfterEnd;
            break;
        }
    }
    _sizeDefined = op == NExtract::NOperationResult::kOK;
    RINOK(extractCallback->SetCompleted(&_packSize))
    return extractCallback->SetOperationResult(op);
    COM_TRY_END
}
} // namespace NLz4Frame
namespace NLz4Frame {
REGISTER_ARC_I_CLS(CHandler(), "Lz4", "lz4", nullptr, 0xF5, kSignatures, 0,
                   NArcInfoFlags::kKeepName | NArcInfoFlags::kPureStartOpen |
                       NArcInfoFlags::kMultiSignature,
                   nullptr)
} // namespace NLz4Frame
} // namespace NArchive

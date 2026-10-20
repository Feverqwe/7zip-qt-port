// SPDX-License-Identifier: GPL-2.0-only
// Single-stream archive boundary adapted from BrotliHandler.cpp (LGPL-2.1-or-later).
// Compression and block framing remain the original Linux C readers.
#include "7zip/Archive/Common/DummyOutStream.h"
#include "7zip/Common/ProgressUtils.h"
#include "7zip/Common/RegisterArc.h"
#include "7zip/Common/StreamUtils.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "linuxframe.h"
#include "linuxframestream.h"
#include <cstring>

using namespace NWindows;
namespace NArchive {
HRESULT ReadZeroTail(ISequentialInStream *, bool &, UInt64 &, UInt64);
namespace NLinuxFrame {
static const Byte kLz4Magic[] = {2, 0x21, 0x4c, 0x18};
static const Byte kLzoMagic[] = {0x89, 'L', 'Z', 'O', 0, 13, 10, 26, 10};

Z7_CLASS_IMP_CHandler_IInArchive_0 CMyComPtr<IInStream> _stream;
UInt64 _packSize = 0, _unpackSize = 0;
UInt32 _errorFlags = 0;
bool _sizeDefined = false;
const bool _lzo;

public:
explicit CHandler(bool lzo) : _lzo(lzo) {}
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
    Byte header[512];
    size_t count = _lzo ? sizeof(header) : sizeof(kLz4Magic);
    RINOK(ReadStream(stream, header, &count))
    if (_lzo ? !sevenzip_lzop_probe(header, count)
             : (count != sizeof(kLz4Magic) || memcmp(header, kLz4Magic, count)))
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
        const HRESULT result = SevenzipDecodeLinuxFrame(_lzo, _stream, offset, _packSize, output,
                                                        progress, consumed, _unpackSize);
        if (result != S_OK && result != S_FALSE)
            return result;
        if (result == S_FALSE) {
            op = NExtract::NOperationResult::kDataError;
            _errorFlags = kpv_ErrorFlags_DataError;
            break;
        }
        offset += consumed;
        bool more = false;
        while (offset < _packSize) {
            RINOK(InStream_SeekSet(_stream, offset))
            UInt64 zeros = 0;
            RINOK(ReadZeroTail(_stream, more, zeros, 1 << 16))
            offset += zeros;
            RINOK(extractCallback->SetCompleted(&offset))
            if (more)
                break;
            if (!zeros && offset < _packSize)
                return E_FAIL;
        }
        if (!more)
            break;
        RINOK(InStream_SeekSet(_stream, offset))
        Byte next[9];
        size_t size = _lzo ? sizeof(kLzoMagic) : sizeof(kLz4Magic);
        RINOK(ReadStream(_stream, next, &size))
        const size_t magicSize = _lzo ? sizeof(kLzoMagic) : sizeof(kLz4Magic);
        if (size != magicSize || memcmp(next, _lzo ? kLzoMagic : kLz4Magic, magicSize)) {
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
} // namespace NLinuxFrame
namespace NLz4Legacy {
REGISTER_ARC_I_CLS(NLinuxFrame::CHandler(false), "Lz4Legacy", "lz4", nullptr, 0xF3,
                   NLinuxFrame::kLz4Magic, 0,
                   NArcInfoFlags::kKeepName | NArcInfoFlags::kPureStartOpen, nullptr)
} // namespace NLz4Legacy
namespace NLzop {
REGISTER_ARC_I_CLS(NLinuxFrame::CHandler(true), "Lzop", "lzo lzop", nullptr, 0xF4,
                   NLinuxFrame::kLzoMagic, 0,
                   NArcInfoFlags::kKeepName | NArcInfoFlags::kPureStartOpen, nullptr)
} // namespace NLzop
} // namespace NArchive

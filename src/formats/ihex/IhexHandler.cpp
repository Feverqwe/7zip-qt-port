// IhexHandler.cpp

#include "7zip/Archive/StdAfx.h"

#include "platform/cpu/CpuArch.h"

#include "Common/ComTry.h"
#include "Common/DynamicBuffer.h"
#include "Common/IntToString.h"
#include "Common/StringToInt.h"

#include "Windows/PropVariant.h"

#include "7zip/Common/InBuffer.h"
#include "7zip/Common/ProgressUtils.h"
#include "7zip/Common/RegisterArc.h"
#include "7zip/Common/StreamObjects.h"
#include "7zip/Common/StreamUtils.h"
#include "archive/compression/MethodProps.h"

namespace NArchive {
namespace NIhex {

/* We still don't support files with custom record types: 20, 22: used by Samsung */

struct CBlock {
    CByteDynamicBuffer Data;
    UInt32 Offset;
};

Z7_CLASS_IMP_CHandler_IInArchive_2(IInArchiveGetStream, ISetProperties)

    bool _isArc;
bool _needMoreInput;
bool _dataError;

UInt64 _phySize;

CObjectVector<CBlock> _blocks;
bool _iqs5xx;
HRESULT Open2(IInStream *stream, IArchiveOpenCallback *openCallback);
HRESULT CheckProgress(const CInBuffer &stream, UInt64 &next, IArchiveOpenCallback *callback);

public:
CHandler() : _iqs5xx(false) { Close(); }
};

static const Byte kProps[] = {kpidPath, kpidSize, kpidVa};

IMP_IInArchive_Props IMP_IInArchive_ArcProps_NO_Table

Z7_COM7F_IMF(CHandler::GetNumberOfItems(UInt32 *numItems)) {
    *numItems = _blocks.Size();
    return S_OK;
}

Z7_COM7F_IMF(CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)) {
    NWindows::NCOM::CPropVariant prop;
    switch (propID) {
    case kpidPhySize:
        if (_phySize != 0)
            prop = _phySize;
        break;
    case kpidErrorFlags: {
        UInt32 v = 0;
        if (!_isArc)
            v |= kpv_ErrorFlags_IsNotArc;
        if (_needMoreInput)
            v |= kpv_ErrorFlags_UnexpectedEnd;
        if (_dataError)
            v |= kpv_ErrorFlags_DataError;
        prop = v;
        break;
    }
    }
    prop.Detach(value);
    return S_OK;
}

Z7_COM7F_IMF(CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    if (index >= _blocks.Size())
        return E_INVALIDARG;
    NWindows::NCOM::CPropVariant prop;
    const CBlock &block = _blocks[index];
    switch (propID) {
    case kpidSize:
        prop = (UInt64)block.Data.GetPos();
        break;
    case kpidVa:
        prop = block.Offset;
        break;
    case kpidPath: {
        if (_blocks.Size() != 1) {
            char s[16];
            ConvertUInt32ToString(index, s);
            prop = s;
        }
        break;
    }
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

static int Parse(const Byte *p) {
    unsigned v0 = p[0];
    Z7_PARSE_HEX_DIGIT(v0, return -1;)
    unsigned v1 = p[1];
    Z7_PARSE_HEX_DIGIT(v1, return -1;)
    return (int)((v0 << 4) | v1);
}

#define kType_Data 0
#define kType_Eof 1
#define kType_Seg 2
// #define kType_CsIp 3
#define kType_High 4
// #define kType_Ip32 5

#define kType_MAX 5

#define IS_LINE_DELIMITER(c) ((c) == 10 || (c) == 13 || (c) == 32 || (c) == 9)

API_FUNC_static_IsArc IsArc_Ihex(const Byte *p, size_t size) {
    if (size < 1)
        return k_IsArc_Res_NEED_MORE;
    if (p[0] != ':')
        return k_IsArc_Res_NO;
    p++;
    size--;

    const unsigned kNumLinesToCheck = 3; // 1 line is OK also, but we don't want false detection

    for (unsigned j = 0; j < kNumLinesToCheck; j++) {
        if (size < 4 * 2)
            return k_IsArc_Res_NEED_MORE;

        const int num = Parse(p);
        if (num < 0)
            return k_IsArc_Res_NO;

        const int type = Parse(p + 6);
        if (type < 0 || type > kType_MAX)
            return k_IsArc_Res_NO;

        unsigned numChars = ((unsigned)num + 5) * 2;
        unsigned sum = 0;

        for (unsigned i = 0; i < numChars; i += 2) {
            if (i + 2 > size)
                return k_IsArc_Res_NEED_MORE;
            int v = Parse(p + i);
            if (v < 0)
                return k_IsArc_Res_NO;
            sum += (unsigned)v;
        }

        if (sum & 0xFF)
            return k_IsArc_Res_NO;

        if (type == kType_Data) {
            // Empty data records are meaningful only in a complete record stream.
        } else {
            if (type == kType_Eof) {
                if (num != 0)
                    return k_IsArc_Res_NO;
                return k_IsArc_Res_YES;
            }
            if (type == kType_Seg || type == kType_High) {
                if (num != 2)
                    return k_IsArc_Res_NO;
            } else {
                if (num != 4)
                    return k_IsArc_Res_NO;
            }
        }

        p += numChars;
        size -= numChars;

        for (;;) {
            if (size == 0)
                return k_IsArc_Res_NEED_MORE;
            const Byte b = *p++;
            size--;
            if (b == ':')
                break;
            if (!IS_LINE_DELIMITER(b))
                return k_IsArc_Res_NO;
        }
    }

    return k_IsArc_Res_YES;
}
}

HRESULT CHandler::CheckProgress(const CInBuffer &stream, UInt64 &next,
                                IArchiveOpenCallback *callback) {
    const UInt64 processed = stream.GetProcessedSize();
    if (callback && processed >= next) {
        next = processed + (1 << 16);
        const UInt64 files = _blocks.Size();
        return callback->SetCompleted(&files, &processed);
    }
    return S_OK;
}

HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *openCallback) {
    try {
        UInt64 fileSize;
        RINOK(InStream_GetSize_SeekToEnd(stream, fileSize))
        RINOK(InStream_SeekToBegin(stream))
        if (openCallback) {
            RINOK(openCallback->SetTotal(NULL, &fileSize))
            const UInt64 completed = 0;
            RINOK(openCallback->SetCompleted(NULL, &completed))
        }
        const unsigned kStartSize = (2 + (256 + 5) + 2) * 2;
        Byte temp[kStartSize];
        if (!_iqs5xx) {
            size_t size = kStartSize;
            RINOK(ReadStream(stream, temp, &size))
            const UInt32 isArcRes = IsArc_Ihex(temp, size);
            if (isArcRes == k_IsArc_Res_NO)
                return S_FALSE;
            if (isArcRes == k_IsArc_Res_NEED_MORE && size != kStartSize)
                return S_FALSE;
        }
        _isArc = true;

        RINOK(InStream_SeekToBegin(stream))
        CInBuffer s;
        if (!s.Create(1 << 15))
            return E_OUTOFMEMORY;
        s.SetStream(stream);
        s.Init();
        {
            Byte b;
            if (!s.ReadByte(b)) {
                _needMoreInput = true;
                return S_FALSE;
            }
            if (b != ':') {
                _dataError = true;
                return S_FALSE;
            }
        }

        UInt32 globalOffset = 0;
        UInt64 progressNext = 1 << 16;
        Byte *iqsMap = NULL;
        if (_iqs5xx) {
            CBlock &block = _blocks.AddNew();
            block.Offset = 0x83C0;
            iqsMap = block.Data.GetCurPtrAndGrow(0xC000 - 0x83C0);
            memset(iqsMap, 0, 0xC000 - 0x83C0);
        }

        for (;;) {
            if (s.ReadBytes(temp, 2) != 2) {
                _needMoreInput = true;
                return S_FALSE;
            }
            const int num = Parse(temp);
            if (num < 0) {
                _dataError = true;
                return S_FALSE;
            }
            {
                const size_t numPairs = (unsigned)num + 4;
                const size_t numBytes = numPairs * 2;
                if (s.ReadBytes(temp, numBytes) != numBytes) {
                    _needMoreInput = true;
                    return S_FALSE;
                }
                unsigned sum = (unsigned)num;
                for (size_t i = 0; i < numPairs; i++) {
                    const int a = Parse(temp + i * 2);
                    if (a < 0) {
                        _dataError = true;
                        return S_FALSE;
                    }
                    temp[i] = (Byte)a;
                    sum += (unsigned)a;
                }
                if ((sum & 0xFF) && !(_iqs5xx && GetBe16(temp) >= 0xBE00)) {
                    _dataError = true;
                    return S_FALSE;
                }
            }

            const unsigned type = temp[2];
            if (type > kType_MAX || (_iqs5xx && type != kType_Data && type != kType_Eof)) {
                _dataError = true;
                return S_FALSE;
            }

            const UInt32 a = GetBe16(temp);

            if (type == kType_Data) {
                if (_iqs5xx) {
                    if (a < 0x83C0 || a > 0xBFFF || (unsigned)num > 0xC000 - a) {
                        _dataError = true;
                        return S_FALSE;
                    }
                    memcpy(iqsMap + a - 0x83C0, temp + 3, (unsigned)num);
                } else if (num != 0) {
                    const UInt32 offs = globalOffset + a;
                    if ((UInt64)offs + (unsigned)num > ((UInt64)1 << 32)) {
                        _dataError = true;
                        return S_FALSE;
                    }
                    CBlock *block = NULL;
                    if (!_blocks.IsEmpty()) {
                        block = &_blocks.Back();
                        if (block->Offset + block->Data.GetPos() != offs)
                            block = NULL;
                    }
                    if (!block) {
                        block = &_blocks.AddNew();
                        block->Offset = offs;
                    }
                    block->Data.AddData(temp + 3, (unsigned)num);
                }
            } else {
                if (type == kType_Eof) {
                    if (num != 0 && !_iqs5xx) {
                        _dataError = true;
                        return S_FALSE;
                    }
                    _phySize = s.GetProcessedSize();
                    Byte b;
                    if (s.ReadByte(b)) {
                        if (b == 10)
                            _phySize++;
                        else if (b == 13) {
                            _phySize++;
                            if (s.ReadByte(b)) {
                                if (b == 10)
                                    _phySize++;
                            }
                        }
                    }
                    if (openCallback) {
                        const UInt64 files = _blocks.Size();
                        RINOK(openCallback->SetCompleted(&files, &_phySize))
                    }
                    return S_OK;
                }

                if (type == kType_Seg || type == kType_High) {
                    if (num != 2) {
                        _dataError = true;
                        return S_FALSE;
                    }
                    // here we use optimization trick for num shift calculation: (type == kType_Seg
                    // ? 4 : 16)
                    globalOffset = (UInt32)GetBe16(temp + 3) << (1u << type);
                } else {
                    if (num != 4) {
                        _dataError = true;
                        return S_FALSE;
                    }
                }
            }

            RINOK(CheckProgress(s, progressNext, openCallback))

            for (;;) {
                Byte b;
                if (!s.ReadByte(b)) {
                    _needMoreInput = true;
                    return S_FALSE;
                }
                RINOK(CheckProgress(s, progressNext, openCallback))
                if (b == ':')
                    break;
                if (!_iqs5xx && !IS_LINE_DELIMITER(b)) {
                    _dataError = true;
                    return S_FALSE;
                }
            }
        }
    } catch (const CInBufferException &e) {
        return e.ErrorCode;
    }
}

Z7_COM7F_IMF(CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *callback)) {
    COM_TRY_BEGIN
    Close();
    HRESULT result;
    try {
        result = Open2(stream, callback);
    } catch (...) {
        Close();
        throw;
    }
    if (result != S_OK) {
        _blocks.Clear();
        _phySize = 0;
    }
    return result;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::GetStream(UInt32 index, ISequentialInStream **stream)) {
    COM_TRY_BEGIN
    *stream = NULL;
    if (index >= _blocks.Size())
        return E_INVALIDARG;
    const CByteDynamicBuffer &data = _blocks[index].Data;
    Create_BufInStream_WithNewBuffer(data, data.GetPos(), stream);
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CHandler::SetProperties(const wchar_t *const *names, const PROPVARIANT *values,
                                     UInt32 numProps)) {
    bool iqs5xx = false;
    for (UInt32 i = 0; i < numProps; i++) {
        if (StringsAreEqualNoCase_Ascii(names[i], "iqs5xx")) {
            RINOK(PROPVARIANT_to_bool(values[i], iqs5xx))
        } else if (!IsString1PrefixedByString2_NoCase_Ascii(names[i], "mt") &&
                   !IsString1PrefixedByString2_NoCase_Ascii(names[i], "memuse"))
            return E_INVALIDARG;
    }
    _iqs5xx = iqs5xx;
    return S_OK;
}

Z7_COM7F_IMF(CHandler::Close()) {
    _phySize = 0;
    _isArc = false;
    _needMoreInput = false;
    _dataError = false;
    _blocks.Clear();
    return S_OK;
}

Z7_COM7F_IMF(CHandler::Extract(const UInt32 *indices, UInt32 numItems, Int32 testMode,
                               IArchiveExtractCallback *extractCallback)) {
    COM_TRY_BEGIN
    const bool allFilesMode = (numItems == (UInt32)(Int32)-1);
    if (!allFilesMode && numItems && !indices)
        return E_INVALIDARG;
    if (allFilesMode)
        numItems = _blocks.Size();
    if (numItems == 0)
        return S_OK;

    UInt64 totalSize = 0;
    UInt32 i;
    for (i = 0; i < numItems; i++) {
        const UInt32 index = allFilesMode ? i : indices[i];
        if (index >= _blocks.Size())
            return E_INVALIDARG;
        totalSize += _blocks[index].Data.GetPos();
    }
    RINOK(extractCallback->SetTotal(totalSize))

    CMyComPtr2_Create<ICompressProgressInfo, CLocalProgress> lps;
    lps->Init(extractCallback, false);

    for (i = 0;; i++) {
        lps->InSize = lps->OutSize;
        RINOK(lps->SetCur())
        if (i >= numItems)
            break;
        const UInt32 index = allFilesMode ? i : indices[i];
        const CByteDynamicBuffer &data = _blocks[index].Data;
        lps->OutSize += data.GetPos();
        {
            CMyComPtr<ISequentialOutStream> realOutStream;
            const Int32 askMode =
                testMode ? NExtract::NAskMode::kTest : NExtract::NAskMode::kExtract;
            RINOK(extractCallback->GetStream(index, &realOutStream, askMode))
            if (!testMode && !realOutStream)
                continue;
            RINOK(extractCallback->PrepareOperation(askMode))
            if (realOutStream)
                RINOK(WriteStream(realOutStream, (const Byte *)data, data.GetPos()))
        }
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK))
    }

    return S_OK;
    COM_TRY_END
}

// k_Signature: { ':' }

REGISTER_ARC_I_NO_SIG("IHex", "ihex", NULL, 0xCD, 0, NArcInfoFlags::kStartOpen, IsArc_Ihex)
}
}

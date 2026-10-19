// SPDX-License-Identifier: GPL-2.0-only
#include "IsoZisofs.h"
#include "../../../../zisofsreader.h"
#include "../../../Common/MyCom.h"
#include "../../Common/StreamUtils.h"
#include <errno.h>

namespace NArchive {
namespace NIso {
class CZisofsStream final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    CMyComPtr<IInStream> _packed;
    sevenzip_zisofs *_state = NULL;
    UInt64 _size = 0, _pos = 0;
    HRESULT _result = S_OK;
    static int ReadPacked(void *context, uint64_t offset, void *data, size_t size) {
        CZisofsStream &self = *(CZisofsStream *)context;
        try {
            self._result = InStream_SeekSet(self._packed, offset);
            if (self._result == S_OK)
                self._result = ReadStream_FALSE(self._packed, data, size);
        } catch (...) {
            self._result = E_FAIL;
        }
        return self._result == S_OK ? 0 : -EIO;
    }

  public:
    ~CZisofsStream() { sevenzip_zisofs_close(_state); }
    HRESULT Init(IInStream *packed, UInt64 size, unsigned block, const sevenzip_iso_inode &inode) {
        _packed = packed;
        _size = inode.i_size;
        const int result =
            sevenzip_zisofs_open(size, _size, inode.i_format_parm[0], inode.i_format_parm[1], block,
                                 ReadPacked, this, &_state);
        return result == 0 ? S_OK : result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
    }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processedSize)) override {
        if (processedSize)
            *processedSize = 0;
        size_t processed = 0;
        _result = S_OK;
        const int result = sevenzip_zisofs_read(_state, _pos, data, size, &processed);
        _pos += processed;
        if (processedSize)
            *processedSize = (UInt32)processed;
        if (_result != S_OK)
            return _result;
        return result == 0 ? S_OK : result == -ENOMEM ? E_OUTOFMEMORY : S_FALSE;
    }
    // Original CExtentsStream::Seek, with the decoded EOF as the end base.
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)) override {
        UInt64 base;
        switch (seekOrigin) {
        case STREAM_SEEK_SET:
            base = 0;
            break;
        case STREAM_SEEK_CUR:
            base = _pos;
            break;
        case STREAM_SEEK_END:
            base = _size;
            break;
        default:
            return STG_E_INVALIDFUNCTION;
        }
        if (offset < 0) {
            const UInt64 delta = (UInt64)0 - (UInt64)offset;
            if (delta > base)
                return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
            base -= delta;
        } else {
            if (base > ((UInt64)1 << 63) - 1 - (UInt64)offset)
                return E_INVALIDARG;
            base += (UInt64)offset;
        }
        _pos = base;
        if (newPosition)
            *newPosition = _pos;
        return S_OK;
    }
};

HRESULT CreateZisofsStream(IInStream *packed, UInt64 size, unsigned block,
                           const sevenzip_iso_inode &inode, ISequentialInStream **stream) {
    CMyComPtr2<IInStream, CZisofsStream> decoder;
    decoder.SetFromCls(new CZisofsStream);
    RINOK(decoder->Init(packed, size, block, inode))
    *stream = decoder.Detach();
    return S_OK;
}
} // namespace NIso
} // namespace NArchive

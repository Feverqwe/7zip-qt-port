#include "qtarchiveitemstream.h"
#include "Common/ComTry.h"
#include "Common/MyCom.h"
#include "qtarchivefileresource.h"

namespace {
class CArchiveItemStream final : public IInStream, public CMyUnknownImp {
    Z7_COM_QI_BEGIN2(ISequentialInStream)
    else if (_seekable && iid == IID_IInStream) {
        *outObject = static_cast<IInStream *>(this);
    }
    Z7_COM_QI_END
    Z7_COM_ADDREF_RELEASE
    // Native interfaces release before the module/catalog/input owner.
    CMyComPtr<IUnknown> _resource;
    CMyComPtr<ISequentialInStream> _stream;
    CMyComPtr<IInStream> _seekable;

  public:
    CArchiveItemStream(IUnknown *resource, ISequentialInStream *stream)
        : _resource(resource), _stream(stream) {
        _stream.QueryInterface(IID_IInStream, &_seekable);
    }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        return _stream->Read(data, size, processed);
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        return _seekable ? _seekable->Seek(offset, origin, position) : E_NOTIMPL;
    }
};
} // namespace

HRESULT CreateArchiveItemStream(IUnknown *resource, UInt32 index, ISequentialInStream **stream) {
    COM_TRY_BEGIN
    if (!stream)
        return E_INVALIDARG;
    *stream = nullptr;
    auto *owner = dynamic_cast<QtArchiveFileResource *>(resource);
    IInArchive *native = owner ? owner->NativeItemProperties() : nullptr;
    CMyComPtr<IInArchiveGetStream> streams;
    if (!native || native->QueryInterface(IID_IInArchiveGetStream, (void **)&streams) != S_OK ||
        !streams)
        return S_OK;
    CMyComPtr<ISequentialInStream> source;
    const HRESULT result = streams->GetStream(index, &source);
    if (source) {
        CMyComPtr<ISequentialInStream> retained = new CArchiveItemStream(resource, source);
        *stream = retained.Detach();
    }
    return result;
    COM_TRY_END
}

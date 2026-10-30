// Seek and serialized-read patterns adapted from 7-Zip StreamObjects.cpp,
// LimitedStreams.cpp and Archive/7z/7zDecode.cpp, b88c6bc09339de32f7d18bbab900686b244f42ba.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "archive/resources/archiveinputlayer.h"
#include "7zip/Common/StreamUtils.h"
#include "Common/ComTry.h"
#include "archive/resources/archivereadstream.h"
#include <limits>
#include <utility>

namespace {
class CArchiveInputCursor final : public IInStream,
                                  public IStreamGetProp,
                                  public IArchiveLayerSource,
                                  public IArchiveReadStream,
                                  public CMyUnknownImp {
    Z7_COM_QI_BEGIN2(IInStream)
    Z7_COM_QI_ENTRY(ISequentialInStream)
    Z7_COM_QI_ENTRY(IArchiveLayerSource)
    Z7_COM_QI_ENTRY(IArchiveReadStream)
    else if (layer_->Properties() && iid == IID_IStreamGetProp) {
        *outObject = static_cast<IStreamGetProp *>(this);
    }
    Z7_COM_QI_END
    Z7_COM_ADDREF_RELEASE
    std::shared_ptr<ArchiveInputLayer> layer_;
    UInt64 position_ = 0;
    std::mutex mutex_;
    IProgress *request_ = nullptr;

  public:
    explicit CArchiveInputCursor(std::shared_ptr<ArchiveInputLayer> layer)
        : layer_(std::move(layer)) {}
    IProgress *SwapRequest(IProgress *request) { return std::exchange(request_, request); }
    Z7_COM7F_IMF(GetProperty(PROPID id, PROPVARIANT *value)) override {
        return layer_->Properties() ? layer_->Properties()->GetProperty(id, value) : E_NOTIMPL;
    }
    Z7_COM7F_IMF(ReloadProps()) override {
        return layer_->Properties() ? layer_->Properties()->ReloadProps() : E_NOTIMPL;
    }
    Z7_COM7F_IMF(GetContainingContainerDepth(UInt64 *depth)) override {
        if (!depth)
            return E_INVALIDARG;
        *depth = 0;
        const auto value = layer_->ContainingContainerDepth();
        if (!value)
            return S_FALSE;
        *depth = *value;
        return S_OK;
    }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        return ReadWithRequest(data, size, processed, request_);
    }
    Z7_COM7F_IMF(ReadWithRequest(void *data, UInt32 size, UInt32 *processed, IProgress *request))
    override {
        std::lock_guard<std::mutex> lock(mutex_);
        UInt32 actual = 0;
        const HRESULT result = layer_->ReadAt(position_, data, size, &actual, request);
        position_ += actual;
        if (processed)
            *processed = actual;
        return result;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        std::lock_guard<std::mutex> lock(mutex_);
        // StreamObjects.cpp CBufInStream / LimitedStreams.cpp seek contract.
        switch (origin) {
        case STREAM_SEEK_SET:
            break;
        case STREAM_SEEK_CUR:
            offset += position_;
            break;
        case STREAM_SEEK_END:
            offset += layer_->size();
            break;
        default:
            return STG_E_INVALIDFUNCTION;
        }
        if (offset < 0)
            return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
        position_ = UInt64(offset);
        if (position)
            *position = position_;
        return S_OK;
    }
    Z7_COM7F_IMF(SeekWithRequest(Int64 offset, UInt32 origin, UInt64 *position, IProgress *request))
    override {
        if (request) {
            RINOK(request->SetCompleted(nullptr))
        }
        return Seek(offset, origin, position);
    }
};
} // namespace

ArchiveInputReadRequestScope::ArchiveInputReadRequestScope(IInStream *cursor, IProgress *request) {
    if (auto *input = dynamic_cast<CArchiveInputCursor *>(cursor)) {
        cursor_ = cursor;
        previous_ = input->SwapRequest(request);
    }
}
ArchiveInputReadRequestScope::~ArchiveInputReadRequestScope() {
    if (cursor_)
        static_cast<CArchiveInputCursor *>(cursor_.Interface())->SwapRequest(previous_);
}

std::expected<std::shared_ptr<ArchiveInputLayer>, HRESULT>
ArchiveInputLayer::Create(IInStream *source, IProgress *request) {
    try {
        if (!source)
            return std::unexpected(E_INVALIDARG);
        auto owner = std::shared_ptr<ArchiveInputLayer>(new ArchiveInputLayer(source));
        const HRESULT result = owner->initialize(request);
        if (result != S_OK)
            return std::unexpected(result);
        return owner;
    } catch (...) {
        // Preserve the native COM_TRY exception mapping at this typed boundary.
        return std::unexpected(E_OUTOFMEMORY);
    }
}

HRESULT ArchiveInputLayer::initialize(IProgress *request) {
    source_->QueryInterface(IID_IStreamGetProp, (void **)&properties_);
    CMyComPtr<IArchiveLayerSource> context;
    const HRESULT query = source_->QueryInterface(IID_IArchiveLayerSource, (void **)&context);
    if (query != S_OK && query != E_NOINTERFACE)
        return query;
    if (context) {
        UInt64 depth;
        const HRESULT result = context->GetContainingContainerDepth(&depth);
        if (result == S_OK)
            containingDepth_ = depth;
        else if (result != S_FALSE)
            return result;
    }
    source_->QueryInterface(IID_IArchiveReadStream, (void **)&requested_);
    if (requested_) {
        RINOK(requested_->SeekWithRequest(0, STREAM_SEEK_END, &size_, request))
    } else {
        if (request) {
            RINOK(request->SetCompleted(nullptr))
        }
        RINOK(InStream_GetSize_SeekToEnd(source_, size_))
    }
    if (size_ > UInt64((std::numeric_limits<Int64>::max)()))
        return E_INVALIDARG;
    return S_OK;
}

HRESULT ArchiveInputLayer::ReadAt(UInt64 offset, void *data, UInt32 size, UInt32 *processed,
                                  IProgress *request) {
    if (processed)
        *processed = 0;
    if (offset >= size_ || !size)
        return S_OK;
    if (!data)
        return E_INVALIDARG;
    const UInt64 remaining = size_ - offset;
    if (size > remaining)
        size = UInt32(remaining);
    // 7zDecode.cpp CLockedSequentialInStreamMT: serialize seek and read in
    // the shared owner. Always restore position: no caller owns source's cursor.
    std::lock_guard<std::mutex> lock(mutex_);
    if (request) {
        RINOK(request->SetCompleted(nullptr))
    }
    if (requested_) {
        RINOK(requested_->SeekWithRequest(Int64(offset), STREAM_SEEK_SET, nullptr, request))
        return requested_->ReadWithRequest(data, size, processed, request);
    }
    RINOK(InStream_SeekSet(source_, offset))
    return source_->Read(data, size, processed);
}

HRESULT ArchiveInputLayer::OpenCursor(IInStream **cursor) {
    COM_TRY_BEGIN
    if (!cursor)
        return E_INVALIDARG;
    *cursor = nullptr;
    CMyComPtr<IInStream> stream = new CArchiveInputCursor(shared_from_this());
    *cursor = stream.Detach();
    return S_OK;
    COM_TRY_END
}

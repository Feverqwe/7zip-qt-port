#pragma once

#include "7zip/IProgress.h"
#include "7zip/IStream.h"
#include "Common/MyCom.h"
#include "archivelayercontext.h"
#include "archivereadstream.h"
#include <memory>
#include <mutex>
#include <optional>

// Read-only range owner shared by native reader generations. Each consumer
// opens an independent upstream IInStream cursor; no payload cache is allocated.
class ArchiveInputLayer final : public std::enable_shared_from_this<ArchiveInputLayer> {
    CMyComPtr<IInStream> source_;
    CMyComPtr<IStreamGetProp> properties_;
    CMyComPtr<IArchiveReadStream> requested_;
    UInt64 size_ = 0;
    std::mutex mutex_;
    std::optional<UInt64> containingDepth_;
    explicit ArchiveInputLayer(IInStream *source) : source_(source) {}

  public:
    static HRESULT Create(IInStream *source, std::shared_ptr<ArchiveInputLayer> &layer,
                          IProgress *request = nullptr);
    UInt64 size() const { return size_; }
    IStreamGetProp *Properties() const { return properties_; }
    std::optional<UInt64> ContainingContainerDepth() const { return containingDepth_; }
    HRESULT ReadAt(UInt64 offset, void *data, UInt32 size, UInt32 *processed,
                   IProgress *request = nullptr);
    HRESULT OpenCursor(IInStream **cursor);
};

// Bind while the native generation lock is held, or before publishing it.
// No callback survives the operation that owns its dialog.
class ArchiveInputReadRequestScope final {
    CMyComPtr<IInStream> cursor_;
    IProgress *previous_ = nullptr;

  public:
    ArchiveInputReadRequestScope(IInStream *cursor, IProgress *request);
    ~ArchiveInputReadRequestScope();
    ArchiveInputReadRequestScope(const ArchiveInputReadRequestScope &) = delete;
    ArchiveInputReadRequestScope &operator=(const ArchiveInputReadRequestScope &) = delete;
};

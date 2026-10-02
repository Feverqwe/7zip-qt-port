#pragma once

#include "CPP/7zip/UI/Common/OpenArchive.h"
#include <QString>
#include <memory>

class ArchiveCodecCatalog;

// A source reader and its exact codec-index owner form one lifetime unit.
// The catalog is destroyed after all handler/volume/stream references.
class ArchiveNativeOpen final : private IOpenCallbackUI {
    std::shared_ptr<const ArchiveCodecCatalog> catalog_;
    IOpenCallbackUI *callback_ = nullptr;
    QString transportType_;
    CArchiveLink link_;
    HRESULT Open_CheckBreak() override;
    HRESULT Open_SetTotal(const UInt64 *files, const UInt64 *bytes) override;
    HRESULT Open_SetCompleted(const UInt64 *files, const UInt64 *bytes) override;
    HRESULT Open_Finished() override;
    HRESULT Open_CryptoGetTextPassword(BSTR *password) override;

  public:
    explicit ArchiveNativeOpen(std::shared_ptr<const ArchiveCodecCatalog> catalog)
        : catalog_(std::move(catalog)) {}
    ~ArchiveNativeOpen();
    HRESULT Open(const QString &path, const QString &type, IOpenCallbackUI *callback);
    HRESULT Close();
    const CArc &arc() const { return *link_.GetArc(); }
    bool hasNestedStream() const { return link_.Arcs.Size() > 1; }
    QString transportType() const { return transportType_; }
};

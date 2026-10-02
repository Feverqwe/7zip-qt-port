#pragma once

#include "CPP/7zip/UI/Common/LoadCodecs.h"
#include "CPP/7zip/UI/Common/OpenArchive.h"
#include <QString>
#include <QStringList>

// Immutable after construction. Consumers retaining native handlers must keep
// the shared backend catalog alive until those handlers and streams are released.
class ArchiveCodecCatalog final {
    CMyComPtr<ICompressCodecsInfo> owner_;
    CCodecs *codecs_ = nullptr;
    HRESULT result_ = E_FAIL;
    QString error_;
    UInt32 version_ = 0;

  public:
    ArchiveCodecCatalog(const QString &module, UInt32 expectedVersion);
    ~ArchiveCodecCatalog();
    ArchiveCodecCatalog(const ArchiveCodecCatalog &) = delete;
    ArchiveCodecCatalog &operator=(const ArchiveCodecCatalog &) = delete;
    HRESULT result() const { return result_; }
    QString error() const { return error_; }
    UInt32 version() const { return version_; }
    const CCodecs &nativeCodecs() const { return *codecs_; }
    int formatIndex(const QString &type) const;
    bool canUpdate(const QString &type) const;
    QStringList extensions() const;
    bool parseOpenTypes(const QString &types, CObjectVector<COpenType> &parsed) const;
    HRESULT openArchive(CArchiveLink &link, const QString &path, const QString &type,
                        IOpenCallbackUI *callback) const;
};

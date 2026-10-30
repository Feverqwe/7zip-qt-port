#pragma once

#include "archive/codecs/LoadCodecs.h"
#include "archive/opening/OpenArchive.h"
#include "archive/sfx/archivesfxmodule.h"
#include <QString>
#include <QStringList>
#include <memory>
class ArchiveCodecImage;
struct CUpdateOptions;
struct CUpdateErrorInfo;
struct IUpdateCallbackUI2;
namespace NWildcard {
class CCensor;
}

// Immutable after construction. Consumers retaining native handlers must keep
// the shared backend catalog alive until those handlers and streams are released.
class ArchiveCodecCatalog final {
    // Images outlive COM owners and unloaded DLLs (reverse member destruction).
    std::unique_ptr<ArchiveCodecImage> image_;
    CMyComPtr<ICompressCodecsInfo> owner_;
    CCodecs *codecs_ = nullptr;
    HRESULT result_ = E_FAIL;
    QString error_;
    UInt32 version_ = 0;
    HRESULT loadDeploymentPlugins();

  public:
    enum class Scope { Module, Deployment };
    ArchiveCodecCatalog(const QString &module, UInt32 expectedVersion, Scope scope = Scope::Module);
    ~ArchiveCodecCatalog();
    ArchiveCodecCatalog(const ArchiveCodecCatalog &) = delete;
    ArchiveCodecCatalog &operator=(const ArchiveCodecCatalog &) = delete;
    HRESULT result() const { return result_; }
    QString error() const { return error_; }
    UInt32 version() const { return version_; }
    const CCodecs &nativeCodecs() const { return *codecs_; }
    void getUserCodecs(CObjectVector<CCodecInfoUser> &codecs) const;
    int formatIndex(const QString &type) const;
    bool canUpdate(const QString &type) const;
    ArchiveSfxModule sfxModule(const QString &path = {}) const;
    QStringList extensions() const;
    bool parseOpenTypes(const QString &types, CObjectVector<COpenType> &parsed) const;
    HRESULT openArchive(CArchiveLink &link, const QString &path, const QString &type,
                        IOpenCallbackUI *callback) const;
    HRESULT openArchive(CArchiveLink &link, const COpenOptions &requested,
                        IOpenCallbackUI *callback) const;
    HRESULT updateArchive(const QString &path, const QString &type, NWildcard::CCensor &censor,
                          CUpdateOptions &options, CUpdateErrorInfo &errorInfo,
                          IOpenCallbackUI *openCallback, IUpdateCallbackUI2 *callback,
                          bool needSetPath) const;
    HRESULT updateArchive(const UString &path, const CObjectVector<COpenType> &types,
                          NWildcard::CCensor &censor, CUpdateOptions &options,
                          CUpdateErrorInfo &errorInfo, IOpenCallbackUI *openCallback,
                          IUpdateCallbackUI2 *callback, bool needSetPath) const;
};

// Typed 7zz transport for the source FileFolderPluginOpen worker.
#pragma once
#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyCom.h"
#include "qtarchivestreamopen.h"
#include "qtprogresstransport.h"
#include <QObject>
#include <memory>

class ArchiveBackend;
class ArchiveCodecCatalog;
class CArchiveFolderManager;
struct COpenOptions;
class QtArchiveFolderManager final : public QObject,
                                     public IFolderManager,
                                     public IQtProgressServices,
                                     public IQtArchiveStreamOpenServices,
                                     public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderManager)
    struct CRequest;
    ArchiveBackend &_backend;
    std::shared_ptr<CRequest> _request;
    std::shared_ptr<const ArchiveCodecCatalog> _associationCatalog;
    std::unique_ptr<CArchiveFolderManager> _associationManager;
    CArchiveFolderManager *AssociationManager();
    void Start(const std::shared_ptr<CRequest> &request, const QString &path, const QString &format,
               const QString &password);
    HRESULT OpenFolderFileImpl(IInStream *stream, const wchar_t *path, const wchar_t *format,
                               IFolderFolder **folder, IProgress *progress, bool ownedStreamFile,
                               const COpenOptions *options = nullptr);

  public:
    explicit QtArchiveFolderManager(ArchiveBackend &backend);
    ~QtArchiveFolderManager() override;
    Z7_IFACE_COM7_IMP(IFolderManager)
    HRESULT OpenArchive(const COpenOptions &options, const wchar_t *format, IFolderFolder **folder,
                        IProgress *progress);
    HRESULT OpenOwnedStreamFile(IInStream *stream, const wchar_t *path, const wchar_t *format,
                                IFolderFolder **folder, IProgress *progress) override;
    std::unique_ptr<IQtProgressTransport> CreateProgressTransport(CProgressDialog &dialog) override;
};

// Typed 7zz transport for the source FileFolderPluginOpen worker.
#pragma once
#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyCom.h"
#include "qtprogresstransport.h"
#include <QObject>
#include <memory>

class ArchiveBackend;
class QtArchiveFolderManager final : public QObject,
                                     public IFolderManager,
                                     public IQtProgressServices,
                                     public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderManager)
    struct CRequest;
    ArchiveBackend &_backend;
    std::shared_ptr<CRequest> _request;
    void Start(const std::shared_ptr<CRequest> &request, const QString &path, const QString &format,
               const QString &password);

  public:
    explicit QtArchiveFolderManager(ArchiveBackend &backend);
    ~QtArchiveFolderManager() override;
    Z7_IFACE_COM7_IMP(IFolderManager)
    std::unique_ptr<IQtProgressTransport> CreateProgressTransport(CProgressDialog &dialog) override;
};

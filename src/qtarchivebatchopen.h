// File-only 7zz opener for the source Common/Extract.cpp archive loop.
#pragma once
#include "7zip/UI/FileManager/IFolder.h"
#include "CPP/7zip/UI/Common/OpenArchive.h"
#include "qtprogresstransport.h"
#include <QString>
#include <memory>

class ArchiveBackend;
class QtArchiveFolderManager;
class CProgressDialog;
struct ExtractionOptions;

class QtArchiveBatchOpen final : public IQtArchiveOpenService, public IQtProgressServices {
    ArchiveBackend &_backend;
    CMyComPtr<IFolderManager> _manager;
    QString _destination;
    const ExtractionOptions *_extractOptions = nullptr;

  public:
    explicit QtArchiveBatchOpen(ArchiveBackend &backend);
    void SetExtractionPolicy(const QString &destination, const ExtractionOptions &options);
    HRESULT Open(CArchiveLink &link, const COpenOptions &options,
                 IOpenCallbackUI *callback) override;
    std::unique_ptr<IQtProgressTransport> CreateProgressTransport(CProgressDialog &dialog) override;
};

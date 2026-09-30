// File-only 7zz opener for the source Common/Extract.cpp archive loop.
#pragma once
#include "7zip/UI/FileManager/IFolder.h"
#include "CPP/7zip/UI/Common/OpenArchive.h"
#include "qtprogresstransport.h"
#include <memory>

class ArchiveBackend;
class QtArchiveFolderManager;
class CProgressDialog;

class QtArchiveBatchOpen final : public IQtArchiveOpenService, public IQtProgressServices {
    ArchiveBackend &_backend;
    CMyComPtr<IFolderManager> _manager;

  public:
    explicit QtArchiveBatchOpen(ArchiveBackend &backend);
    HRESULT Open(CArchiveLink &link, const COpenOptions &options,
                 IOpenCallbackUI *callback) override;
    std::unique_ptr<IQtProgressTransport> CreateProgressTransport(CProgressDialog &dialog) override;
};

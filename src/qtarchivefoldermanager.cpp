#include "qtarchivefoldermanager.h"
#include "CPP/7zip/UI/FileManager/OpenCallback.h"
#include "Common/ComTry.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "archivenativeopen.h"
#include "archiveprogresstransport.h"
#include "qtarchivefileresource.h"
#include "qtarchivenativeopenprogress.h"
#include "qtlocationfolder.h"
#include <QFile>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QScopeGuard>
#include <QThread>
#include <atomic>

struct QtArchiveFolderManager::CRequest {
    QMutex Mutex;
    bool Completed = false;
    HRESULT Result = E_FAIL;
    QList<ArchiveEntry> Entries;
    ArchiveInfo Info;
    CMyComPtr<IFolderFolder> Folder;
    CMyComPtr<IUnknown> Resource;
    std::atomic<bool> PasswordRequested{false};
};

QtArchiveFolderManager::QtArchiveFolderManager(ArchiveBackend &backend) : _backend(backend) {
    connect(&backend, &ArchiveBackend::infoReady, this, [this](const ArchiveInfo &info) {
        if (_request)
            _request->Info = info;
    });
    connect(&backend, &ArchiveBackend::entriesReady, this,
            [this](const QList<ArchiveEntry> &entries) {
                if (_request)
                    _request->Entries = entries;
            });
    connect(&backend, &ArchiveBackend::passwordRequired, this, [this] {
        if (_request)
            _request->PasswordRequested.store(true);
    });
    connect(&backend, &ArchiveBackend::finished, this, [this](bool success, const QString &) {
        if (!_request)
            return;
        const auto request = std::move(_request);
        HRESULT result = success ? E_FAIL : _backend.lastResult();
        if (!success && result == S_OK)
            result = E_FAIL;
        try {
            if (success) {
                auto *resource =
                    dynamic_cast<QtArchiveFileResource *>(request->Resource.Interface());
                if (resource) {
                    resource->Info = request->Info;
                    resource->AttachCodecCatalog(_backend.codecCatalog());
                    resource->SetArchiveMetadata(request->Info.physicalSize, request->Info.offset);
                    const QString transportFormat = resource->TransportFormat();
                    resource->AttachReopenService(_backend, transportFormat);
                    // A split archive opens through the outer Split handler before its
                    // contained format; forcing -t7z on .001 bypasses that handler.
                    resource->AttachExtractService(_backend, transportFormat);
                    if (_backend.supportsArchiveUpdate(request->Info.format))
                        resource->AttachUpdateService(_backend, request->Info.format);
                }
                result = CreateArchiveFolder(request->Entries,
                                             request->Info.format.toStdWString().c_str(),
                                             &request->Folder, request->Resource);
            }
        } catch (...) {
            result = E_FAIL;
        }
        QMutexLocker lock(&request->Mutex);
        request->Result = result;
        request->Completed = true;
    });
}

QtArchiveFolderManager::~QtArchiveFolderManager() {
    // Source Create joins the worker before its manager reference is released.
    Q_ASSERT(!_request);
    disconnect(&_backend, nullptr, this, nullptr);
}

void QtArchiveFolderManager::Start(const std::shared_ptr<CRequest> &request, const QString &path,
                                   const QString &format, const QString &password) {
    if (_request || _backend.busy()) {
        QMutexLocker lock(&request->Mutex);
        request->Result = E_FAIL;
        request->Completed = true;
        return;
    }
    _request = request;
    try {
        _backend.openFolder(path, format, password);
    } catch (...) {
        // A start implementation may throw after launching its owned process.
        _backend.cancelAndWait();
        _request.reset();
        QMutexLocker lock(&request->Mutex);
        request->Result = E_FAIL;
        request->Completed = true;
    }
}

Z7_COM7F_IMF(QtArchiveFolderManager::OpenFolderFile(IInStream *inStream, const wchar_t *filePath,
                                                    const wchar_t *arcFormat,
                                                    IFolderFolder **resultFolder,
                                                    IProgress *progress)) {
    return OpenFolderFileImpl(inStream, filePath, arcFormat, resultFolder, progress, false);
}

HRESULT QtArchiveFolderManager::OpenOwnedStreamFile(IInStream *stream, const wchar_t *path,
                                                    const wchar_t *format, IFolderFolder **folder,
                                                    IProgress *progress) {
    return OpenFolderFileImpl(stream, path, format, folder, progress, true);
}

HRESULT QtArchiveFolderManager::OpenFolderFileImpl(IInStream *inStream, const wchar_t *filePath,
                                                   const wchar_t *arcFormat,
                                                   IFolderFolder **resultFolder,
                                                   IProgress *progress, bool ownedStreamFile) {
    COM_TRY_BEGIN
    if (!resultFolder)
        return E_INVALIDARG;
    *resultFolder = nullptr;
    // This adapter waits on the GUI process service. Only the source worker
    // can call it; synchronous GUI calls would block the service event loop.
    if (QThread::currentThread() == thread())
        return E_FAIL;
    if (!filePath || !*filePath)
        return E_INVALIDARG;
    const auto request = std::make_shared<CRequest>();
    auto *resource = new QtArchiveFileResource(QString::fromWCharArray(filePath));
    request->Resource = resource;
    QString ownedPath;
    bool keepOwnedPath = false;
    const auto cleanup = qScopeGuard([&] {
        if (!keepOwnedPath && !ownedPath.isEmpty()) {
            resource->CloseNativeOpen();
            QFile::remove(ownedPath);
        }
    });
    const QString format = QString::fromWCharArray(arcFormat);
    auto *callback = dynamic_cast<COpenCallbackImp *>(progress);
    auto *ui = callback ? dynamic_cast<COpenArchiveCallback *>(callback->Callback) : nullptr;
    if (ui)
        ui->ProgressDialog.WaitCreating();
    if (inStream) {
        if (ownedStreamFile) {
            RINOK(resource->SpoolToOwnedFile(inStream, QString::fromWCharArray(filePath), progress))
            ownedPath = QString::fromWCharArray(filePath);
        } else
            RINOK(resource->Spool(inStream, progress))
    }
    const QString path = resource->EnginePath;
    CNativeOpenProgress nativeProgress(progress, callback ? callback->Callback : nullptr);
    std::shared_ptr<ArchiveNativeOpen> nativeOpen;
    RINOK(_backend.openNativeArchive(path, format, &nativeProgress, nativeOpen))
    resource->AttachNativeOpen(std::move(nativeOpen));
    const QString listingFormat =
        resource->RequiresOuterOpen() ? resource->TransportFormat() : format;
    const QString password = ui && ui->PasswordIsDefined
                                 ? QString::fromWCharArray(ui->Password.Ptr())
                                 : nativeProgress.Password;
    QMetaObject::invokeMethod(
        this,
        [this, request, path, listingFormat, password] {
            Start(request, path, listingFormat, password);
        },
        Qt::QueuedConnection);
    HRESULT stopResult = S_OK;
    bool cancelRequested = false;
    for (;;) {
        {
            QMutexLocker lock(&request->Mutex);
            if (request->Completed) {
                if (stopResult != S_OK)
                    return stopResult;
                if (request->Result == S_OK) {
                    *resultFolder = request->Folder.Detach();
                    keepOwnedPath = true;
                }
                return request->Result;
            }
        }
        if (stopResult == S_OK && request->PasswordRequested.exchange(false)) {
            try {
                CMyComPtr<ICryptoGetTextPassword> crypto;
                if (progress)
                    progress->QueryInterface(IID_ICryptoGetTextPassword, (void **)&crypto);
                if (!crypto)
                    stopResult = E_NOINTERFACE;
                else {
                    CMyComBSTR password;
                    stopResult = crypto->CryptoGetTextPassword(&password);
                    if (stopResult == S_OK) {
                        const QString value = QString::fromWCharArray(password);
                        QMetaObject::invokeMethod(
                            this,
                            [this, request, value] {
                                if (_request == request)
                                    _backend.providePassword(value);
                            },
                            Qt::QueuedConnection);
                    }
                }
            } catch (...) {
                stopResult = E_FAIL;
            }
        }
        // An external child can finish while paused. Do not enter the source
        // cooperative pause wait before observing its completion latch.
        if (stopResult == S_OK && ui && ui->ProgressDialog.Sync.Get_Stopped())
            stopResult = E_ABORT;
        if (stopResult != S_OK && !cancelRequested) {
            cancelRequested = true;
            QMetaObject::invokeMethod(
                this,
                [this, request] {
                    if (_request == request)
                        _backend.cancel();
                },
                Qt::QueuedConnection);
        }
        QThread::msleep(10);
    }
    COM_TRY_END
}

Z7_COM7F_IMF(QtArchiveFolderManager::GetExtensions(BSTR *extensions)) {
    if (!extensions)
        return E_INVALIDARG;
    *extensions = nullptr;
    const auto catalog = _backend.codecCatalog();
    if (!catalog || catalog->result() != S_OK)
        return E_FAIL;
    // Qt transports the source CArcInfoEx extension catalog. Windows icon
    // resource enumeration remains separate presentation/association scope.
    const QString value = catalog->extensions().join(' ');
    return StringToBstr(UString(value.toStdWString().c_str()), extensions);
}

Z7_COM7F_IMF(QtArchiveFolderManager::GetIconPath(const wchar_t *, BSTR *iconPath,
                                                 Int32 *iconIndex)) {
    if (iconPath)
        *iconPath = nullptr;
    if (iconIndex)
        *iconIndex = -1;
    return E_NOTIMPL; // The source icon owner belongs to P0-09.
}

std::unique_ptr<IQtProgressTransport>
QtArchiveFolderManager::CreateProgressTransport(CProgressDialog &dialog) {
    return std::make_unique<ArchiveProgressTransport>(dialog, _backend, true);
}

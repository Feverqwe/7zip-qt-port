// Reopen the file-only engine metadata without creating another archive owner.
#include "qtarchivereopen.h"
#include "7zip/Archive/IArchive.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/OpenCallback.h"
#include "Common/ComTry.h"
#include "Common/StringConvert.h"
#include "qtarchivebackendrequest.h"
#include "qtarchivefileresource.h"
#include "qtlocationfolder.h"
#include <QThread>
#include <memory>

namespace {
struct CMetadataRequest {
    ArchiveInfo Info;
    QList<ArchiveEntry> Entries;
};
class CConnection {
    QMetaObject::Connection _connection;

  public:
    explicit CConnection(QMetaObject::Connection connection) : _connection(connection) {}
    ~CConnection() { QObject::disconnect(_connection); }
};
} // namespace

// Native FileFolderPluginOpen worker/modal lifecycle, with ReOpen as its action.
HRESULT ReOpenArchiveFolder(CAgentFolder &folder, const QString &settingsPath, QWidget *parent,
                            IFolderFolder **resultFolder, bool usePassword,
                            const UString &password) {
    if (!resultFolder)
        return E_INVALIDARG;
    *resultFolder = nullptr;
    struct CThreadArchiveReOpen {
        CAgentFolder *Folder;
        COpenArchiveCallback UI;
        CMyComPtr<IArchiveOpenCallback> Callback;
        CMyComPtr<IFolderFolder> ResultFolder;
        HRESULT Result = E_FAIL;
        static THREAD_FUNC_DECL Run(void *param) {
            auto &request = *static_cast<CThreadArchiveReOpen *>(param);
            CProgressCloser closer(request.UI.ProgressDialog);
            try {
                request.UI.ProgressDialog.WaitCreating();
                request.Result = request.Folder->ReOpen(request.Callback, &request.ResultFolder);
            } catch (...) {
                request.Result = E_FAIL;
            }
            return 0;
        }
    } request;
    request.Folder = &folder;
    request.UI.SettingsPath = settingsPath;
    request.UI.ParentWindow = parent;
    request.UI.PasswordIsDefined = usePassword;
    request.UI.Password = password;
    request.UI.ProgressDialog.MainWindow = parent;
    request.UI.ProgressDialog.MainTitle = L"7-Zip";
    request.UI.ProgressDialog.WaitMode = true;
    auto *callback = new COpenCallbackImp;
    request.Callback = callback;
    callback->Callback = &request.UI;
    auto transport = folder.CreateProgressTransport(request.UI.ProgressDialog);
    request.UI.ProgressDialog.AttachTransport(transport.get());
    struct CScopedSync {
        QtArchiveFileResource *Resource;
        explicit CScopedSync(CAgentFolder &folder, CProgressSync &sync)
            : Resource(dynamic_cast<QtArchiveFileResource *>(folder.GetEngineResource())) {
            if (Resource)
                Resource->SetUpdateProgress(&sync);
        }
        ~CScopedSync() {
            if (Resource)
                Resource->SetUpdateProgress(nullptr);
        }
    } sync(folder, request.UI.ProgressDialog.Sync);
    NWindows::CThread thread;
    const WRes started = thread.Create(CThreadArchiveReOpen::Run, &request);
    if (started != 0)
        return HRESULT_FROM_WIN32(started);
    request.UI.StartProgressDialog(UString(L"Обновление архива"), thread);
    if (request.Result == S_OK)
        *resultFolder = request.ResultFolder.Detach();
    return request.Result;
}

void QtArchiveFileResource::AttachReopenService(ArchiveBackend &backend, const QString &format) {
    _reopen = [this, &backend, format](IFolderFolder **metadata,
                                       IArchiveOpenCallback *callback) -> HRESULT {
        COM_TRY_BEGIN
        if (!metadata)
            return E_INVALIDARG;
        *metadata = nullptr;
        if (QThread::currentThread() == backend.thread())
            return E_FAIL;
        const auto request = std::make_shared<CMetadataRequest>();
        CConnection info(
            QObject::connect(&backend, &ArchiveBackend::infoReady, &backend,
                             [request](const ArchiveInfo &value) { request->Info = value; }));
        CConnection entries(QObject::connect(
            &backend, &ArchiveBackend::entriesReady, &backend,
            [request](const QList<ArchiveEntry> &value) { request->Entries = value; }));
        const QString path = EnginePath;
        RINOK(RunArchiveBackendRequest(
            backend, [&backend, path, format] { backend.openFolder(path, format, {}); }, callback,
            _sync))
        RINOK(CreateArchiveMetadata(request->Entries, metadata))
        Info = request->Info;
        SetArchiveMetadata(request->Info.physicalSize, request->Info.offset);
        return S_OK;
        COM_TRY_END
    };
}

#include "7zip/UI/Common/ArchiveOpenCallback.h"
#include "Common/ComTry.h"
#include "archivenativeopen.h"
#include "deviceaccess.h"
#include "qtarchivefileresource.h"
#include "qtarchivenativeopenprogress.h"

bool QtArchiveFileResource::IgnoresDeviceJournal() const {
    const auto *direct = dynamic_cast<DirectDeviceInput *>(_directInput.Interface());
    return direct && direct->ignoresDeviceJournal();
}

HRESULT QtArchiveFileResource::RefreshDeviceInput(ArchiveBackend &backend,
                                                  IOpenCallbackUI *callback, QString *error) {
    std::shared_ptr<ArchiveNativeOpen> opened;
    RINOK(backend.openDeviceFilesystem(_codecs, _directInput, EnginePath, callback, opened, error))
    AttachNativeOpen(std::move(opened));
    _nativeReadOnly = true;
    BindNativeItemOrder(nullptr);
    Info.format = QStringLiteral("Ext");
    Info.physicalSize = qint64(_nativeOpen->arc().FileSize);
    Info.offset = 0;
    SetArchiveMetadata(Info.physicalSize, 0);
    return S_OK;
}

HRESULT QtArchiveFileResource::OpenDeviceInput(ArchiveBackend &backend, IInStream *input,
                                               IOpenCallbackUI *callback, QString *error) {
    COM_TRY_BEGIN
    if (!dynamic_cast<DirectDeviceInput *>(input) || _directInput)
        return E_INVALIDARG;
    _directInput = input;
    const HRESULT result = RefreshDeviceInput(backend, callback, error);
    if (result != S_OK) {
        _directInput.Release();
        return result;
    }
    AttachProgressService(backend);
    _extract = [this, &backend](IInArchive *, const UInt32 *indices, UInt32 count, Int32 testMode,
                                IArchiveExtractCallback *progress) {
        return HasNativeExtractService()
                   ? backend.extractNativeArchive(_nativeOpen, indices, count, testMode, progress)
                   : E_FAIL;
    };
    _reopen = [this, &backend](IFolderFolder **metadata,
                               IArchiveOpenCallback *callback) -> HRESULT {
        COM_TRY_BEGIN
        if (!metadata)
            return E_INVALIDARG;
        *metadata = nullptr;
        auto *open = dynamic_cast<COpenCallbackImp *>(callback);
        CMyComPtr<IProgress> progress;
        if (callback)
            callback->QueryInterface(IID_IProgress, (void **)&progress);
        CNativeOpenProgress nativeProgress(progress, open ? open->Callback : nullptr);
        RINOK(CloseNativeOpen())
        return RefreshDeviceInput(backend, &nativeProgress);
        COM_TRY_END
    };
    return S_OK;
    COM_TRY_END
}

#include "qtarchivesourceupdate.h"
#include "7zip/Common/FileStreams.h"
#include "7zip/Common/StreamUtils.h"
#include "7zip/Compress/CopyCoder.h"
#include "7zip/IPassword.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "Common/ComTry.h"
#include "Common/MyCom.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
#include "archiveprogresstransport.h"
#include "qtarchivebackendrequest.h"
#include "qtarchivefileresource.h"
#include <QDir>
#include <QTemporaryDir>

namespace {
class CResourceProgress final : public IQtProgressTransport {
    CMyComPtr<IUnknown> _resource;
    QtArchiveFileResource *_resourceSpec;
    ArchiveProgressTransport _transport;

  public:
    CResourceProgress(QtArchiveFileResource *resource, CProgressDialog &dialog,
                      ArchiveBackend &backend)
        : _resource(resource), _resourceSpec(resource), _transport(dialog, backend) {
        resource->SetUpdateProgress(&dialog.Sync);
    }
    ~CResourceProgress() override { _resourceSpec->SetUpdateProgress(nullptr); }
    HRESULT SetPaused(bool paused) override { return _transport.SetPaused(paused); }
    HRESULT SetBackground(bool background) override { return _transport.SetBackground(background); }
};
} // namespace

void QtArchiveFileResource::AttachProgressService(ArchiveBackend &backend) {
    _progress = [this, &backend](CProgressDialog &dialog) {
        return std::make_unique<CResourceProgress>(this, dialog, backend);
    };
}

void QtArchiveFileResource::AttachUpdateService(ArchiveBackend &backend, const QString &format) {
    AttachReopenService(backend, format);
    AttachProgressService(backend);
    _update = [this, &backend, format](ISequentialOutStream *output, IInArchive *metadata,
                                       UInt32 count, IArchiveUpdateCallback *callback) {
        return UpdateArchiveSourceStream(backend, EnginePath, format, output, metadata, count,
                                         callback, _sync);
    };
}

namespace {
class CCallbackProgress final : public ICompressProgressInfo, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ICompressProgressInfo)
    IArchiveUpdateCallback *_callback;

  public:
    explicit CCallbackProgress(IArchiveUpdateCallback *callback) : _callback(callback) {}
    Z7_COM7F_IMF(SetRatioInfo(const UInt64 *inSize, const UInt64 *)) override {
        return _callback->SetCompleted(inSize);
    }
};

HRESULT FindChangedItem(IInArchive *metadata, UInt32 count, IArchiveUpdateCallback *callback,
                        UInt32 &changed, QString &name) {
    UInt32 originalCount;
    RINOK(metadata->GetNumberOfItems(&originalCount))
    // This engine service implements source UpdateOneFile only. Unsupported
    // callback plans must not silently become a different archive operation.
    if (count != originalCount)
        return E_NOTIMPL;
    changed = UInt32(-1);
    for (UInt32 i = 0; i < count; i++) {
        Int32 newData, newProps;
        UInt32 index;
        RINOK(callback->GetUpdateItemInfo(i, &newData, &newProps, &index))
        if (index != i || newData != newProps)
            return E_NOTIMPL;
        if (!newData)
            continue;
        if (changed != UInt32(-1))
            return E_NOTIMPL;
        NWindows::NCOM::CPropVariant original, requested, directory;
        RINOK(metadata->GetProperty(index, kpidPath, &original))
        RINOK(callback->GetProperty(i, kpidPath, &requested))
        RINOK(callback->GetProperty(i, kpidIsDir, &directory))
        if (original.vt != VT_BSTR || requested.vt != VT_BSTR || directory.vt != VT_BOOL ||
            directory.boolVal != VARIANT_FALSE)
            return E_NOTIMPL;
        name = QString::fromWCharArray(requested.bstrVal);
        if (name != QString::fromWCharArray(original.bstrVal) ||
            !ArchiveBackend::sourceUpdatePathIsSafe(name))
            return E_NOTIMPL;
        changed = i;
    }
    return changed == UInt32(-1) ? E_NOTIMPL : S_OK;
}

HRESULT StageCallbackStream(const QString &root, const QString &name, UInt32 index,
                            IArchiveUpdateCallback *callback) {
    const QString path = QDir(root).filePath(name);
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        return E_FAIL;
    CMyComPtr<ISequentialInStream> input;
    RINOK(callback->GetStream(index, &input))
    if (!input)
        return E_FAIL;
    auto *file = new COutFileStream;
    CMyComPtr<ISequentialOutStream> output = file;
    if (!file->Create_NEW(us2fs(path.toStdWString().c_str())))
        return GetLastError_noZero_HRESULT();
    CMyComPtr<ICompressProgressInfo> progress = new CCallbackProgress(callback);
    RINOK(NCompress::CopyStream(input, output, progress))
    return file->Close();
}
} // namespace

HRESULT UpdateArchiveSourceStream(ArchiveBackend &backend, const QString &archive,
                                  const QString &format, ISequentialOutStream *output,
                                  IInArchive *metadata, UInt32 count,
                                  IArchiveUpdateCallback *callback, CProgressSync *sync) {
    COM_TRY_BEGIN
    if (!output || !metadata || !callback)
        return E_INVALIDARG;
    if (format != "zip" && format != "7z")
        return E_NOTIMPL;
    // Prefix publication is owned by native CommonUpdateOperation. Until that
    // consumer is connected, this file service accepts only a zero-offset arc.
    NWindows::NCOM::CPropVariant offset;
    RINOK(metadata->GetArchiveProperty(kpidOffset, &offset))
    if ((offset.vt == VT_I8 && offset.hVal.QuadPart != 0) ||
        (offset.vt == VT_UI8 && offset.uhVal.QuadPart != 0) ||
        (offset.vt != VT_EMPTY && offset.vt != VT_I8 && offset.vt != VT_UI8))
        return E_NOTIMPL;
    UInt32 changed;
    QString name;
    RINOK(FindChangedItem(metadata, count, callback, changed, name))
    QTemporaryDir stage;
    if (!stage.isValid())
        return E_FAIL;
    const QString sourceRoot = stage.filePath("source");
    RINOK(StageCallbackStream(sourceRoot, name, changed, callback))
    CMyComPtr<ICryptoGetTextPassword2> crypto;
    callback->QueryInterface(IID_ICryptoGetTextPassword2, (void **)&crypto);
    QString password;
    if (crypto) {
        Int32 defined = 0;
        CMyComBSTR value;
        RINOK(crypto->CryptoGetTextPassword2(&defined, &value))
        if (defined)
            password = QString::fromWCharArray(value);
    }
    const QString result = stage.filePath("result." + format);
    RINOK(RunArchiveBackendRequest(
        backend,
        [&backend, archive, name, sourceRoot, result, format, password] {
            backend.updateSourceFile(archive, name, sourceRoot, result, format, password);
        },
        callback, sync))
    auto *inputSpec = new CInFileStream;
    CMyComPtr<ISequentialInStream> input = inputSpec;
    if (!inputSpec->Open(us2fs(result.toStdWString().c_str())))
        return GetLastError_noZero_HRESULT();
    CMyComPtr<ICompressProgressInfo> progress = new CCallbackProgress(callback);
    RINOK(NCompress::CopyStream(input, output, progress))
    return callback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
    COM_TRY_END
}

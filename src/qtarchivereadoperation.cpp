// Callback item/block projection adapted from UI/Common/ArchiveExtractCallback.cpp,
// b88c6bc09339de32f7d18bbab900686b244f42ba, Igor Pavlov, LGPL-2.1-or-later.
// Test decoding remains in the official Open/Extract handlers; Qt owns the worker.
#include "qtarchivereadoperation.h"
#include "7zip/IPassword.h"
#include "CPP/7zip/UI/Common/ArchiveOpenCallback.h"
#include "Common/ComTry.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "archivenativeopen.h"
#include <QMutexLocker>

namespace {
class CTestCallback final : public IArchiveExtractCallback,
                            public IArchiveExtractCallbackMessage2,
                            public ICryptoGetTextPassword,
                            public IArchiveRequestMemoryUseCallback,
                            public IOpenCallbackUI,
                            public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_5(IArchiveExtractCallback, IProgress, ICryptoGetTextPassword,
                         IArchiveRequestMemoryUseCallback, IArchiveExtractCallbackMessage2)
    QtArchiveReadOperation &operation_;
    const CArc *archive_ = nullptr;
    UInt64 total_ = 0;
    QString path_;
    bool encrypted_ = false;
    QMutex stateMutex_;
    HRESULT ReadItem(UInt32 index, QString &path, bool &encrypted) {
        if (!archive_)
            return E_FAIL;
        CReadArcItem item;
        RINOK(archive_->GetItem(index, item))
        path = QString::fromWCharArray(item.Path);
        return Archive_GetItemBoolProp(archive_->Archive, index, kpidEncrypted, encrypted);
    }

  public:
    explicit CTestCallback(QtArchiveReadOperation &operation) : operation_(operation) {}
    void Attach(const CArc *archive) { archive_ = archive; }
    Z7_COM7F_IMF(SetTotal(UInt64 total)) override {
        {
            QMutexLocker lock(&stateMutex_);
            total_ = total;
        }
        return operation_.checkpoint();
    }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *completed)) override {
        UInt64 total;
        QString path;
        {
            QMutexLocker lock(&stateMutex_);
            total = total_;
            path = path_;
        }
        if (completed)
            operation_.report(*completed, total, path);
        return operation_.checkpoint();
    }
    Z7_COM7F_IMF(GetStream(UInt32 index, ISequentialOutStream **stream, Int32)) override {
        if (!stream || !archive_)
            return E_INVALIDARG;
        *stream = nullptr; // Official test mode consumes data without publishing files.
        QString path;
        bool encrypted = false;
        RINOK(ReadItem(index, path, encrypted)) {
            QMutexLocker lock(&stateMutex_);
            path_ = path;
            encrypted_ = encrypted;
        }
        return operation_.checkpoint();
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return operation_.checkpoint(); }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        // Extract's HRESULT alone does not account for CRC/partial item results.
        if (result != NArchive::NExtract::NOperationResult::kOK) {
            QMutexLocker lock(&stateMutex_);
            operation_.Errors.append({path_, result, encrypted_});
        }
        return operation_.checkpoint();
    }
    Z7_COM7F_IMF(ReportExtractResult(UInt32 indexType, UInt32 index, Int32 result)) override {
        // ArchiveExtractCallback::ReportExtractResult's item/block projection.
        QString path;
        bool encrypted = false;
        if (indexType == NArchive::NEventIndexType::kInArcIndex && index != UInt32(-1)) {
            RINOK(ReadItem(index, path, encrypted))
        } else
            path = '#' + QString::number(index);
        if (result != NArchive::NExtract::NOperationResult::kOK) {
            QMutexLocker lock(&stateMutex_);
            operation_.Errors.append({path, result, encrypted});
        }
        return operation_.checkpoint();
    }
    Z7_COM7F_IMF(CryptoGetTextPassword(BSTR *value)) override { return operation_.password(value); }
    Z7_COM7F_IMF(RequestMemoryUse(UInt32 flags, UInt32, UInt32, const wchar_t *, UInt64 required,
                                  UInt64 *allowed, UInt32 *answer))
    override {
        RINOK(operation_.checkpoint())
        if (!allowed || !answer)
            return E_INVALIDARG;
        // ExtractCallback::RequestMemoryUse's non-dialog fast path: preserve
        // the engine's already allowed budget, without silently increasing it.
        if ((flags & NRequestMemoryUseFlags::k_IsReport) == 0 && required <= *allowed) {
            *answer = NRequestMemoryAnswerFlags::k_Allow;
            return S_OK;
        }
        return E_OUTOFMEMORY; // This non-dialog API has no memory-limit consent service.
    }
    HRESULT Open_CheckBreak() override { return operation_.checkpoint(); }
    HRESULT Open_SetTotal(const UInt64 *, const UInt64 *bytes) override {
        return bytes ? SetTotal(*bytes) : operation_.checkpoint();
    }
    HRESULT Open_SetCompleted(const UInt64 *, const UInt64 *bytes) override {
        return SetCompleted(bytes);
    }
    HRESULT Open_Finished() override { return operation_.checkpoint(); }
    HRESULT Open_CryptoGetTextPassword(BSTR *value) override { return operation_.password(value); }
};
} // namespace

QtArchiveReadOperation::QtArchiveReadOperation(QString path, QString password,
                                               std::shared_ptr<const ArchiveCodecCatalog> catalog,
                                               Work work, Kind kind, QString format)
    : catalog_(std::move(catalog)), path_(std::move(path)), work_(std::move(work)), kind_(kind),
      format_(std::move(format)), password_(std::move(password)),
      passwordDefined_(!password_.isEmpty()) {}

void QtArchiveReadOperation::cancel() {
    progress_.cancel();
    QMutexLocker lock(&passwordMutex_);
    passwordReady_.wakeAll();
}
void QtArchiveReadOperation::setPaused(bool paused) { progress_.setPaused(paused); }
HRESULT QtArchiveReadOperation::checkpoint() { return progress_.checkpoint(); }
void QtArchiveReadOperation::providePassword(const QString &password) {
    QMutexLocker lock(&passwordMutex_);
    password_ = password;
    passwordDefined_ = true;
    passwordReady_.wakeAll();
}
HRESULT QtArchiveReadOperation::password(BSTR *value) {
    if (!value)
        return E_INVALIDARG;
    *value = nullptr;
    for (;;) {
        RINOK(checkpoint())
        QMutexLocker lock(&passwordMutex_);
        if (passwordDefined_)
            return StringToBstr(password_.toStdWString().c_str(), value);
        if (!passwordRequested_) {
            if (!PasswordRequired)
                return E_ABORT;
            passwordRequested_ = true;
            lock.unlock();
            PasswordRequired();
            continue;
        }
        passwordReady_.wait(&passwordMutex_, 50);
    }
}
void QtArchiveReadOperation::report(quint64 completed, quint64 total, const QString &path) {
    if (Progress)
        Progress(completed, total, path);
}
void QtArchiveReadOperation::recordOpenError(const QString &path, const CArcErrorInfo &error) {
    if (error.IsThereErrorOrWarning())
        OpenErrors.append({path, error.GetErrorFlags(), error.GetWarningFlags(),
                           QString::fromWCharArray(error.ErrorMessage),
                           QString::fromWCharArray(error.WarningMessage)});
}
void QtArchiveReadOperation::run() {
    try {
        Result = checkpoint();
        if (Result != S_OK)
            return;
        auto *callback = new CTestCallback(*this);
        CMyComPtr<IArchiveExtractCallback> retained = callback;
        auto reader = std::make_shared<ArchiveNativeOpen>(catalog_);
        Result = reader->Open(path_, format_, callback);
        if (Result != S_OK)
            recordOpenError(path_, reader->archiveLink().NonOpen_ErrorInfo);
        for (unsigned i = 0; i < reader->archiveLink().Arcs.Size(); ++i) {
            const auto &arc = reader->archiveLink().Arcs[i];
            recordOpenError(QString::fromWCharArray(arc.Path), arc.ErrorInfo);
        }
        if (Result != S_OK)
            return;
        callback->Attach(&reader->arc());
        Result = work_(reader, callback);
        callback->Attach(nullptr);
        if (Result == S_OK && (!Errors.isEmpty() || (kind_ == Kind::Test && !OpenErrors.isEmpty())))
            Result = E_FAIL;
    } catch (...) {
        Result = E_OUTOFMEMORY;
    }
}

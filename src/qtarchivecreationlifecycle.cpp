#include "qtarchivecreationlifecycle.h"
#include "CPP/7zip/UI/Common/EnumDirItems.h"
#include "Common/StringConvert.h"
#include "qtlanguageservices.h"
#include <QMutexLocker>

namespace {
class CCreationScanCallback final : public IDirItemsCallback {
    QtArchiveCreationLifecycle &owner_;

  public:
    explicit CCreationScanCallback(QtArchiveCreationLifecycle &owner) : owner_(owner) {}
    HRESULT ScanError(const FString &, DWORD error) override {
        return error ? HRESULT_FROM_WIN32(error) : E_FAIL;
    }
    HRESULT ScanProgress(const CDirItemsStat &, const FString &path, bool isDir) override {
        const HRESULT result = owner_.checkpoint();
        if (result == S_OK && owner_.Progress)
            owner_.Progress(QString::fromWCharArray(fs2us(path).Ptr()), isDir);
        return result;
    }
};
} // namespace

QtArchiveCreationLifecycle::QtArchiveCreationLifecycle(QStringList sources)
    : sources_(std::move(sources)) {}

void QtArchiveCreationLifecycle::cancel() {
    QMutexLocker lock(&mutex_);
    stopped_ = true;
    resumed_.wakeAll();
}
void QtArchiveCreationLifecycle::setPaused(bool paused) {
    QMutexLocker lock(&mutex_);
    paused_ = paused;
    resumed_.wakeAll();
}
HRESULT QtArchiveCreationLifecycle::checkpoint() {
    QMutexLocker lock(&mutex_);
    while (paused_ && !stopped_)
        resumed_.wait(&mutex_);
    return stopped_ ? E_ABORT : S_OK;
}
void QtArchiveCreationLifecycle::beginDeletion() {
    deleting_ = true;
    Result = E_FAIL;
    inputs_.MarkCreatedInputsProcessed();
    start();
}

HRESULT QtArchiveCreationLifecycle::scan() {
    CCreationScanCallback callback(*this);
    inputs_.DirItems.Callback = &callback;
    // Enumerate links as links so a selected directory cannot lead deletion
    // through an unported link workflow. Link storage remains the P2 owner.
    inputs_.DirItems.SymLinks = true;
    FStringVector names;
    for (const QString &source : sources_)
        names.Add(us2fs(source.toStdWString().c_str()));
    const HRESULT result = inputs_.DirItems.EnumerateItems2(FString(), UString(), names, nullptr);
    inputs_.DirItems.Callback = nullptr;
    if (result != S_OK)
        return result;
    FOR_VECTOR(i, inputs_.DirItems.Items) {
        if (inputs_.DirItems.Items[i].AreReparseData()) {
            Error = QtLocalText(
                "Deleting inputs after compression is unavailable for sources containing links.",
                QStringLiteral(
                    "Удаление после сжатия для источников со ссылками пока недоступно."));
            return E_NOTIMPL;
        }
    }
    return checkpoint();
}

void QtArchiveCreationLifecycle::run() {
    try {
        Result = checkpoint();
        if (Result == S_OK) {
            if (deleting_) {
                QtUpdateDeletionCallback callback;
                callback.Progress = [this](const FString &path, bool isDir) {
                    const HRESULT result = checkpoint();
                    if (result == S_OK && Progress)
                        Progress(QString::fromWCharArray(fs2us(path).Ptr()), isDir);
                    return result;
                };
                Result = inputs_.Delete(callback);
            } else
                Result = scan();
        }
    } catch (...) {
        Result = E_OUTOFMEMORY;
    }
    if (Result != S_OK && Error.isEmpty())
        Error = Result == E_ABORT
                    ? QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."))
                    : QtLocalText("Could not process input files.",
                                  QStringLiteral("Не удалось обработать исходные файлы."));
}

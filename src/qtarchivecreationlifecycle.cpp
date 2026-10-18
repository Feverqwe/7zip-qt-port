#include "qtarchivecreationlifecycle.h"
#include "CPP/7zip/UI/Common/EnumDirItems.h"
#include "Common/StringConvert.h"
#include "qtlanguageservices.h"

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

QtArchiveCreationLifecycle::QtArchiveCreationLifecycle(QStringList sources,
                                                       const CBoolPair &symLinks)
    : sources_(std::move(sources)) {
    inputs_.SymLinks = symLinks;
}

void QtArchiveCreationLifecycle::cancel() { progress_.cancel(); }
void QtArchiveCreationLifecycle::setPaused(bool paused) { progress_.setPaused(paused); }
HRESULT QtArchiveCreationLifecycle::checkpoint() { return progress_.checkpoint(); }
void QtArchiveCreationLifecycle::beginDeletion() {
    deleting_ = true;
    Result = E_FAIL;
    inputs_.MarkCreatedInputsProcessed();
    start();
}

HRESULT QtArchiveCreationLifecycle::scan() {
    CCreationScanCallback callback(*this);
    inputs_.DirItems.Callback = &callback;
    // Detect links without following them. Deletion can use this snapshot only
    // when the engine request also stores links rather than their targets.
    inputs_.DirItems.SymLinks = true;
    NWildcard::CCensor censor;
    for (const QString &source : sources_)
        censor.AddPreItem_NoWildcard(source.toStdWString().c_str());
    censor.AddPathsToCensor(NWildcard::k_RelatPath);
    // Common/Update.cpp's censor enumeration sets original reparse properties
    // for selected leaves too; Agent's EnumerateItems2 omits that operation.
    const HRESULT result =
        EnumerateItems(censor, NWildcard::k_RelatPath, UString(), inputs_.DirItems);
    inputs_.DirItems.Callback = nullptr;
    if (result != S_OK)
        return result;
    FOR_VECTOR(i, inputs_.DirItems.Items) {
        if (inputs_.DirItems.Items[i].AreReparseData() && !inputs_.SymLinks.Val) {
            Error = QCoreApplication::translate(
                "qtarchivecreationlifecycle",
                "Deleting inputs after compression is unavailable for sources containing links.");
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
        Error = Result == E_ABORT ? QCoreApplication::translate("qtarchivecreationlifecycle",
                                                                "Operation canceled.")
                                  : QCoreApplication::translate("qtarchivecreationlifecycle",
                                                                "Could not process input files.");
}

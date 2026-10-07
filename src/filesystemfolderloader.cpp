#include "filesystemfolderloader.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "qtlocationfolder.h"
#include <QScopeGuard>

FilesystemFolderLoader::FilesystemFolderLoader(QString directory, QObject *parent, bool flat,
                                               IFolderFolder *boundFolder)
    : QThread(parent), directory_(std::move(directory)), flat_(flat) {
    // CFSFolder::Init arms native notifications. Their owner thread must outlive
    // the worker: the published folder is polled after that worker has exited.
    // Keep only the original LoadItems walk in run(), as in native panel binding.
    try {
        if (boundFolder) {
            if (dynamic_cast<NFsFolder::CFSFolder *>(boundFolder)) {
                folder_ = boundFolder;
                result_ = S_OK;
            } else
                result_ = E_INVALIDARG;
        } else
            result_ = CreateFilesystemFolder(directory_, &folder_);
        loadingFolder_ = folder_;
    } catch (...) {
        result_ = E_OUTOFMEMORY;
    }
}

void FilesystemFolderLoader::cancel() { canceled_ = true; }

HRESULT FilesystemFolderLoader::result() const { return isRunning() ? E_FAIL : result_; }

IFolderFolder *FilesystemFolderLoader::takeFolder() {
    return isRunning() || result_ != S_OK ? nullptr : folder_.Detach();
}

void FilesystemFolderLoader::run() {
    if (canceled_) {
        result_ = E_ABORT;
        return;
    }
    try {
        if (result_ != S_OK)
            return;
        auto *spec = static_cast<NFsFolder::CFSFolder *>((IFolderFolder *)folder_);
        const auto clearCallbacks = qScopeGuard([&] {
            spec->CheckBreak = {};
            spec->SetCompleted = {};
        });
        spec->CheckBreak = [this](const QString &) { return !canceled_; };
        spec->SetCompleted = [this](quint64 count) {
            completed_.store(count);
            return !canceled_;
        };
        result_ = spec->SetFlatMode(BoolToInt(flat_));
        if (result_ == S_OK)
            result_ = spec->LoadItems();
        if (canceled_)
            result_ = E_ABORT;
    } catch (...) {
        result_ = E_OUTOFMEMORY;
    }
    // CMyComPtr uses non-atomic reference counts. Release even failed/canceled
    // folders on the GUI owner thread after wait(), alongside panel references.
}

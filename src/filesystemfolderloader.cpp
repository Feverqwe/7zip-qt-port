#include "filesystemfolderloader.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "qtlocationfolder.h"

FilesystemFolderLoader::FilesystemFolderLoader(QString directory, QObject *parent, bool flat)
    : QThread(parent), directory_(std::move(directory)), flat_(flat) {}

void FilesystemFolderLoader::cancel() { canceled_ = true; }

HRESULT FilesystemFolderLoader::result() const { return isRunning() ? E_FAIL : result_; }

IFolderFolder *FilesystemFolderLoader::takeFolder() {
    return isRunning() ? nullptr : folder_.Detach();
}

void FilesystemFolderLoader::run() {
    if (canceled_) {
        result_ = E_ABORT;
        return;
    }
    try {
        result_ = CreateFilesystemFolder(directory_, &folder_);
        if (result_ != S_OK)
            return;
        auto *spec = static_cast<NFsFolder::CFSFolder *>((IFolderFolder *)folder_);
        spec->CheckBreak = [this](const QString &) { return !canceled_; };
        spec->SetCompleted = [this](quint64 count) {
            completed_.store(count);
            return !canceled_;
        };
        result_ = spec->SetFlatMode(BoolToInt(flat_));
        if (result_ == S_OK)
            result_ = spec->LoadItems();
        spec->CheckBreak = {};
        spec->SetCompleted = {};
        if (canceled_)
            result_ = E_ABORT;
    } catch (...) {
        result_ = E_OUTOFMEMORY;
    }
    if (result_ != S_OK)
        folder_.Release();
}

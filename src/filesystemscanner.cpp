#include "filesystemscanner.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "Common/StringConvert.h"
#include "filesystemfolderloader.h"
#include "qtlocationfolder.h"
#include <QDir>
#include <QElapsedTimer>
#include <QPointer>
#include <QTimer>

FilesystemScanner::FilesystemScanner(QObject *parent) : QObject(parent) {
    progressTimer_.setInterval(33);
    connect(&progressTimer_, &QTimer::timeout, this, [this] {
        if (!loader_)
            return;
        const quint64 count = loader_->completedCount();
        if (count != progressCount_) {
            progressCount_ = count;
            emit advanced(count);
        }
    });
}

FilesystemScanner::~FilesystemScanner() { cancelAndWait(); }

void FilesystemScanner::cancelAndWait() {
    cancel();
    const auto loaders = loaders_;
    loaders_.clear();
    for (auto *loader : loaders) {
        disconnect(loader, nullptr, this, nullptr);
        loader->cancel();
        loader->wait();
        delete loader;
    }
}

void FilesystemScanner::start(const QString &directory, bool flat) {
    cancel();
    loader_ = new FilesystemFolderLoader(directory, this, flat);
    loaders_.append(loader_);
    const quint64 generation = generation_;
    auto *loader = loader_;
    progressTimer_.start();
    const QPointer<FilesystemFolderLoader> guard(loader);
    connect(loader, &QThread::finished, this, [this, guard, generation] {
        // Disconnect does not remove already queued signals. A close-time join
        // may have deleted this loader before its finished delivery reaches us.
        if (guard)
            loaderFinished(guard, generation);
    });
    loader->start();
}

void FilesystemScanner::loaderFinished(FilesystemFolderLoader *loader, quint64 generation) {
    loader->wait();
    loaders_.removeOne(loader);
    const bool current = generation == generation_ && loader == loader_;
    if (current) {
        loader_ = nullptr;
        progressTimer_.stop();
    }
    const quint64 count = loader->completedCount();
    const HRESULT result = loader->result();
    CMyComPtr<IFolderFolder> loaded;
    loaded.Attach(loader->takeFolder());
    loader->deleteLater();
    if (!current)
        return;
    const QPointer<FilesystemScanner> progressGuard(this);
    if (count != progressCount_) {
        progressCount_ = count;
        emit advanced(count);
    }
    if (!progressGuard || generation != generation_)
        return;
    if (result != S_OK) {
        emit failed(qint32(result));
        return;
    }
    folder_ = loaded;
    directory_ = QtFilesystemPath(us2fs(GetFolderPath(folder_)));
    presenting_ = true;
    const QPointer<FilesystemScanner> guard(this);
    emit folderReady(folder_);
    if (!guard)
        return;
    if (generation == generation_ && presenting_)
        QTimer::singleShot(0, this, [this, generation] { scanBatch(generation); });
}

void FilesystemScanner::cancel() {
    ++generation_;
    progressTimer_.stop();
    progressCount_ = 0;
    if (loader_)
        loader_->cancel();
    loader_ = nullptr;
    folder_.Release();
    nextIndex_ = 0;
    presenting_ = false;
}

bool FilesystemScanner::isRunning() const { return loader_ || presenting_; }

void FilesystemScanner::scanBatch(quint64 generation) {
    if (generation != generation_ || !presenting_ || !folder_)
        return;
    QElapsedTimer elapsed;
    elapsed.start();
    QFileInfoList files;
    const quint32 firstIndex = nextIndex_;
    auto *spec = static_cast<NFsFolder::CFSFolder *>((IFolderFolder *)folder_);
    // Row creation can include native icon lookup. Keep each GUI delivery
    // bounded independently of the time spent preparing QFileInfo values.
    while (files.size() < 32 && elapsed.elapsed() < 8 && nextIndex_ < spec->Files.Size()) {
        const FString relPath = spec->GetRelPath(spec->Files[nextIndex_++]);
        const QString name = QtFilesystemPath(relPath);
        files.append(QFileInfo(QDir(directory_).filePath(name)));
    }
    const QPointer<FilesystemScanner> guard(this);
    if (!files.isEmpty())
        emit batchReady(files, firstIndex);
    if (!guard)
        return;
    if (generation != generation_ || !presenting_ || !folder_)
        return;
    if (nextIndex_ < spec->Files.Size()) {
        QTimer::singleShot(0, this, [this, generation] { scanBatch(generation); });
        return;
    }
    presenting_ = false;
    emit completed();
}

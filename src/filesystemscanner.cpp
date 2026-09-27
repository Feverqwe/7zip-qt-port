#include "filesystemscanner.h"

#include <QDirIterator>
#include <QElapsedTimer>
#include <QTimer>

FilesystemScanner::FilesystemScanner(QObject *parent) : QObject(parent) {}

FilesystemScanner::~FilesystemScanner() = default;

void FilesystemScanner::start(const QString &directory, QDir::Filters filters) {
    cancel();
    iterator_ = std::make_unique<QDirIterator>(directory, filters, QDirIterator::Subdirectories);
    const quint64 generation = generation_;
    QTimer::singleShot(0, this, [this, generation] { scanBatch(generation); });
}

void FilesystemScanner::cancel() {
    ++generation_;
    iterator_.reset();
}

bool FilesystemScanner::isRunning() const { return iterator_ != nullptr; }

void FilesystemScanner::scanBatch(quint64 generation) {
    if (generation != generation_ || !iterator_)
        return;
    QElapsedTimer elapsed;
    elapsed.start();
    QFileInfoList files;
    while (files.size() < 128 && elapsed.elapsed() < 8 && iterator_->hasNext()) {
        iterator_->next();
        files.append(iterator_->fileInfo());
    }
    if (!files.isEmpty())
        emit batchReady(files);
    if (generation != generation_ || !iterator_)
        return;
    if (iterator_->hasNext()) {
        QTimer::singleShot(0, this, [this, generation] { scanBatch(generation); });
        return;
    }
    iterator_.reset();
    emit completed();
}

#include "filetransferoperation.h"
#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "qtfiletransfertransaction.h"
#include "qtlinkservices.h"
#include <QMutexLocker>

FileTransferOperation::FileTransferOperation(QStringList sources, QString destination, bool move,
                                             QObject *parent, FileSystemAdapter adapter,
                                             DestinationKind destinationKind)
    : QThread(parent), sources_(std::move(sources)), destination_(std::move(destination)),
      move_(move), destinationKind_(destinationKind), adapter_(std::move(adapter)) {
    if (!adapter_.trySourceRename)
        adapter_.trySourceRename = [](const QString &source, const QString &stage) {
            return QtRenameFileSystemEntry(source, stage);
        };
    if (!adapter_.removeSource)
        adapter_.removeSource = QtRemoveFileSystemEntry;
}

void FileTransferOperation::cancel() {
    canceled_ = true;
    overwriteCondition_.wakeAll();
    emit dismissOverwriteRequested();
}

void FileTransferOperation::submitOverwriteAnswer(int answer) {
    QMutexLocker lock(&overwriteMutex_);
    if (!overwriteWaiting_)
        return;
    overwriteAnswer_ = answer;
    overwriteCondition_.wakeAll();
}

int FileTransferOperation::askOverwrite(const QString &existing, const QString &incoming) {
    QMutexLocker lock(&overwriteMutex_);
    if (canceled_ || receivers(SIGNAL(overwriteRequested(QString, QString))) == 0) {
        canceled_ = true;
        return NOverwriteAnswer::kCancel;
    }
    overwriteWaiting_ = true;
    overwriteAnswer_ = -1;
    lock.unlock();
    emit overwriteRequested(existing, incoming);
    lock.relock();
    while (overwriteAnswer_ < 0 && !canceled_)
        overwriteCondition_.wait(&overwriteMutex_);
    const int answer = canceled_ ? NOverwriteAnswer::kCancel : overwriteAnswer_;
    overwriteWaiting_ = false;
    if (answer == NOverwriteAnswer::kCancel)
        canceled_ = true;
    return answer;
}

void FileTransferOperation::run() { QtFileTransferTransaction(*this).run(); }

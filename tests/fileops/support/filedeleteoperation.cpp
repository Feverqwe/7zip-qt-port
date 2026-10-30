#include "fileops/support/filedeleteoperation.h"
#include "filemanager/providers/FSFolder.h"

FileDeleteOperation::FileDeleteOperation(QString directory, QStringList paths, QObject *parent)
    : QThread(parent), directory_(std::move(directory)), paths_(std::move(paths)) {}

void FileDeleteOperation::cancel() { canceled_ = true; }

void FileDeleteOperation::run() {
    NFsFolder::CFSFolder folder(directory_);
    CRecordVector<UInt32> indices;
    for (const QString &path : paths_) {
        folder.AddPath(path);
        indices.Add(folder.Files.Size() - 1);
    }
    folder.SetTotal = [this](quint64 total) {
        emit advanced(0, int(total));
        return !canceled_;
    };
    folder.SetCompleted = [this](quint64 completed) {
        // Upstream reports the just-deleted zero-based index. Qt displays count.
        emit advanced(int(completed + 1), int(paths_.size()));
        return !canceled_;
    };
    folder.CheckBreak = [this](const QString &path) {
        if (canceled_)
            return false;
        emit currentFile(path);
        return !canceled_;
    };
    const bool ok = folder.Delete(indices.ConstData(), indices.Size());
    emit result(ok, ok          ? QStringLiteral("Объекты удалены.")
                    : canceled_ ? QStringLiteral("Операция отменена.")
                                : folder.ErrorMessage);
}

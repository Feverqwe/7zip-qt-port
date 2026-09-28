#include "filetransferoperation.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>

FileTransferOperation::FileTransferOperation(QStringList sources, QString destination, bool move,
                                             QObject *parent)
    : QThread(parent), sources_(std::move(sources)), destination_(std::move(destination)),
      move_(move) {}

void FileTransferOperation::cancel() { canceled_ = true; }

void FileTransferOperation::removeEntry(const QString &path) {
    if (QFileInfo(path).isDir())
        QDir(path).removeRecursively();
    else
        QFile::remove(path);
}

bool FileTransferOperation::copyFile(const QString &source, const QString &target, QString *error) {
    NFsFolder::CCopyStateIO copyState;
    quint64 previous = 0;
    copyState.Progress = [this, &previous](quint64 completed) {
        emit advanced(qint64(completed - previous));
        previous = completed;
        return !canceled_;
    };
    const bool completed = copyState.MyCopyFile(source, target, QFileInfo(source).permissions());
    if (!completed) {
        *error = QStringLiteral("Операция отменена.");
        return false;
    }
    if (copyState.ErrorFileIndex >= 0 || !copyState.ErrorMessage.isEmpty()) {
        *error = QStringLiteral("Ошибка чтения или записи файла: %1")
                     .arg(copyState.ErrorFileIndex == 1 ? target : source);
        return false;
    }
    return true;
}

bool FileTransferOperation::validateEntry(const QString &source, QString *error) {
    if (canceled_) {
        *error = QStringLiteral("Операция отменена.");
        return false;
    }
    const QFileInfo info(source);
    if (!info.exists() || info.isSymLink() || (!info.isFile() && !info.isDir())) {
        *error = QStringLiteral("Источник недоступен: %1").arg(source);
        return false;
    }
    if (!info.isDir())
        return true;
    const QFileInfoList children = QDir(source).entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    for (const QFileInfo &child : children) {
        if (!validateEntry(child.absoluteFilePath(), error))
            return false;
    }
    return true;
}

bool FileTransferOperation::copyEntry(const QString &source, const QString &target,
                                      QString *error) {
    if (canceled_) {
        *error = QStringLiteral("Операция отменена.");
        return false;
    }
    const QFileInfo info(source);
    if (!info.exists() || info.isSymLink()) {
        *error = QStringLiteral("Источник недоступен: %1").arg(source);
        return false;
    }
    emit currentFile(source);
    if (!info.isDir())
        return copyFile(source, target, error);
    if (!QDir().mkdir(target)) {
        *error = QStringLiteral("Не удалось создать папку: %1").arg(target);
        return false;
    }
    const QFileInfoList children = QDir(source).entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    for (const QFileInfo &child : children) {
        if (!copyEntry(child.absoluteFilePath(), QDir(target).filePath(child.fileName()), error))
            return false;
    }
    return true;
}

void FileTransferOperation::run() {
    QString error;
    const QFileInfo destinationInfo(destination_);
    const QString canonicalDestination = destinationInfo.canonicalFilePath();
    QStringList targets;
    if (!destinationInfo.isDir() || destinationInfo.isSymLink() || sources_.isEmpty())
        error = QStringLiteral("Папка назначения недоступна.");
    for (const QString &source : sources_) {
        if (!error.isEmpty())
            break;
        const QFileInfo info(source);
        const QString target = QDir(destination_).filePath(info.fileName());
        if (!info.exists() || info.isSymLink() || (!info.isFile() && !info.isDir()) ||
            QFileInfo::exists(target) || QFileInfo(target).isSymLink() ||
            targets.contains(target) ||
            (info.isDir() &&
             NFsFolder::IsDestChild(info.canonicalFilePath(), canonicalDestination)))
            error = QStringLiteral("Источник или назначение недоступны либо имя уже занято.");
        targets << target;
    }
    QStringList completed;
    for (qsizetype i = 0; i < sources_.size() && error.isEmpty(); ++i) {
        if (canceled_) {
            error = QStringLiteral("Операция отменена.");
            break;
        }
        const QString stage =
            targets[i] +
            QStringLiteral(".sevenzip-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
        emit currentFile(sources_[i]);
        const bool valid = !move_ || validateEntry(sources_[i], &error);
        const bool staged = valid && (move_ ? QDir().rename(sources_[i], stage)
                                            : copyEntry(sources_[i], stage, &error));
        if (!staged && error.isEmpty())
            error = QStringLiteral("Не удалось переместить объект: %1").arg(sources_[i]);
        if (error.isEmpty() && canceled_)
            error = QStringLiteral("Операция отменена.");
        if (error.isEmpty() &&
            (QFileInfo::exists(targets[i]) || QFileInfo(targets[i]).isSymLink() ||
             !QDir().rename(stage, targets[i])))
            error = QStringLiteral("Не удалось завершить перенос: %1").arg(targets[i]);
        if (!error.isEmpty()) {
            if (move_ && staged)
                QDir().rename(stage, sources_[i]);
            else
                removeEntry(stage);
            break;
        }
        completed << targets[i];
    }
    if (!error.isEmpty()) {
        for (qsizetype i = completed.size(); i > 0; --i) {
            if (move_)
                QDir().rename(completed[i - 1], sources_[i - 1]);
            else
                removeEntry(completed[i - 1]);
        }
        emit result(false, error);
    } else {
        emit result(true, move_ ? QStringLiteral("Объекты перемещены.")
                                : QStringLiteral("Объекты скопированы."));
    }
}

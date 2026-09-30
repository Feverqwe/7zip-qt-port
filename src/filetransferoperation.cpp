#include "filetransferoperation.h"
#include "7zip/Common/FilePathAutoRename.h"
#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "Common/StringConvert.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>

FileTransferOperation::FileTransferOperation(QStringList sources, QString destination, bool move,
                                             QObject *parent, FileSystemAdapter adapter)
    : QThread(parent), sources_(std::move(sources)), destination_(std::move(destination)),
      move_(move), adapter_(std::move(adapter)) {
    if (!adapter_.trySourceRename)
        adapter_.trySourceRename = [](const QString &source, const QString &stage) {
            return QDir().rename(source, stage);
        };
    if (!adapter_.removeSource)
        adapter_.removeSource = removeEntry;
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
    if (canceled_ || receivers(SIGNAL(overwriteRequested(QString, QString))) == 0)
        return NOverwriteAnswer::kCancel;
    overwriteWaiting_ = true;
    overwriteAnswer_ = -1;
    lock.unlock();
    emit overwriteRequested(existing, incoming);
    lock.relock();
    while (overwriteAnswer_ < 0 && !canceled_)
        overwriteCondition_.wait(&overwriteMutex_);
    const int answer = canceled_ ? NOverwriteAnswer::kCancel : overwriteAnswer_;
    overwriteWaiting_ = false;
    return answer;
}

bool FileTransferOperation::removeEntry(const QString &path) {
    if (QFileInfo(path).isDir())
        return QDir(path).removeRecursively();
    return QFile::remove(path);
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
    NFsFolder::CCopyState state;
    quint64 previous = 0;
    state.Progress = [this, &previous](quint64 completed) {
        if (completed != previous)
            emit advanced(qint64(completed - previous));
        previous = completed;
        return !canceled_;
    };
    state.CurrentFile = [this](const QString &path) { emit currentFile(path); };
    if (!state.CallProgress()) {
        *error = state.ErrorMessage;
        return false;
    }
    const QFileInfo info(source);
    const bool copied = info.isDir() ? NFsFolder::CopyFolder(state, source, target)
                                     : NFsFolder::CopyFile_Ask(state, source, info, target);
    *error = state.ErrorMessage;
    return copied;
}

void FileTransferOperation::run() {
    struct Completed {
        QString source;
        QString target;
        QString backup;
        bool copiedMove = false;
        bool sourceDeleted = false;
    };
    enum class OverwriteMode { Ask, Skip, Overwrite, Rename };
    OverwriteMode overwriteMode = OverwriteMode::Ask;
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
            QFileInfo(target).isSymLink() || targets.contains(target) ||
            (info.isDir() &&
             NFsFolder::IsDestChild(info.canonicalFilePath(), canonicalDestination)))
            error = QStringLiteral("Источник или назначение недоступны либо имя уже занято.");
        targets << target;
    }
    QList<Completed> completed;
    for (qsizetype i = 0; i < sources_.size() && error.isEmpty(); ++i) {
        if (canceled_) {
            error = QStringLiteral("Операция отменена.");
            break;
        }
        const QString source = sources_[i];
        QString target = targets[i];
        bool mayReplace = false;
        const QFileInfo sourceInfo(source);
        const QFileInfo existingInfo(target);
        if (existingInfo.exists()) {
            if (existingInfo.isDir() != sourceInfo.isDir()) {
                error =
                    QStringLiteral("Нельзя заменить файл папкой или папку файлом: %1").arg(target);
                break;
            }
            if (overwriteMode == OverwriteMode::Ask) {
                const int answer = askOverwrite(target, source);
                switch (answer) {
                case NOverwriteAnswer::kNo:
                    continue;
                case NOverwriteAnswer::kNoToAll:
                    overwriteMode = OverwriteMode::Skip;
                    continue;
                case NOverwriteAnswer::kYes:
                    break;
                case NOverwriteAnswer::kYesToAll:
                    overwriteMode = OverwriteMode::Overwrite;
                    break;
                case NOverwriteAnswer::kAutoRename:
                    overwriteMode = OverwriteMode::Rename;
                    break;
                default:
                    error = QStringLiteral("Операция отменена.");
                    break;
                }
            }
            if (!error.isEmpty())
                break;
            if (overwriteMode == OverwriteMode::Skip)
                continue;
            if (overwriteMode == OverwriteMode::Rename) {
                FString renamed = us2fs(target.toStdWString().c_str());
                if (!AutoRenamePath(renamed)) {
                    error = QStringLiteral("Не удалось подобрать свободное имя: %1").arg(target);
                    break;
                }
                target = QString::fromWCharArray(fs2us(renamed));
            } else {
                mayReplace = true;
            }
        }
        const QString suffix =
            QStringLiteral(".sevenzip-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
        const QString stage = target + suffix;
        QString backup;
        emit currentFile(sources_[i]);
        const bool valid = !move_ || validateEntry(source, &error);
        const bool renamed = valid && move_ && adapter_.trySourceRename(source, stage);
        const bool copiedMove = valid && move_ && !renamed;
        const bool staged = valid && (renamed || copyEntry(source, stage, &error));
        if (!staged && error.isEmpty())
            error = QStringLiteral("Не удалось переместить объект: %1").arg(sources_[i]);
        if (error.isEmpty() && canceled_)
            error = QStringLiteral("Операция отменена.");
        if (error.isEmpty() && QFileInfo(target).isSymLink())
            error = QStringLiteral("Назначение изменилось во время переноса: %1").arg(target);
        if (error.isEmpty() && QFileInfo::exists(target) && !mayReplace)
            error = QStringLiteral("Назначение изменилось во время переноса: %1").arg(target);
        if (error.isEmpty() && QFileInfo::exists(target)) {
            // Keep the old entry until every source has published successfully.
            backup = target + suffix + QStringLiteral("-backup");
            if (!QDir().rename(target, backup))
                error = QStringLiteral("Не удалось сохранить прежний объект: %1").arg(target);
        }
        if (error.isEmpty() && !QDir().rename(stage, target))
            error = QStringLiteral("Не удалось завершить перенос: %1").arg(target);
        if (!error.isEmpty()) {
            if (renamed)
                QDir().rename(stage, source);
            else
                removeEntry(stage);
            if (!backup.isEmpty())
                QDir().rename(backup, target);
            break;
        }
        completed.append({source, target, backup, copiedMove, move_ && !copiedMove});
    }
    if (!error.isEmpty()) {
        for (qsizetype i = completed.size(); i > 0; --i) {
            const Completed &entry = completed[i - 1];
            if (move_ && !entry.copiedMove)
                QDir().rename(entry.target, entry.source);
            else
                removeEntry(entry.target);
            if (!entry.backup.isEmpty())
                QDir().rename(entry.backup, entry.target);
        }
        emit result(false, error);
    } else if (move_) {
        for (Completed &entry : completed) {
            if (!entry.copiedMove)
                continue;
            if (canceled_) {
                error = QStringLiteral("Операция отменена.");
                break;
            }
            if (!adapter_.removeSource(entry.source)) {
                // Recursive deletion can fail after removing some children.
                // Retain the published copy so the remaining bytes stay available.
                entry.sourceDeleted = true;
                error = QStringLiteral("Не удалось удалить источник после копирования: %1")
                            .arg(entry.source);
                break;
            }
            entry.sourceDeleted = true;
        }
        if (!error.isEmpty()) {
            for (qsizetype i = completed.size(); i > 0; --i) {
                const Completed &entry = completed[i - 1];
                if (entry.sourceDeleted) {
                    if (!entry.backup.isEmpty())
                        removeEntry(entry.backup);
                    continue;
                }
                removeEntry(entry.target);
                if (!entry.backup.isEmpty())
                    QDir().rename(entry.backup, entry.target);
            }
            emit result(false, error + QStringLiteral(" Часть объектов могла быть перемещена."));
            return;
        }
        for (const Completed &entry : completed)
            if (!entry.backup.isEmpty())
                removeEntry(entry.backup);
        emit result(true, QStringLiteral("Объекты перемещены."));
    } else {
        for (const Completed &entry : completed)
            if (!entry.backup.isEmpty())
                removeEntry(entry.backup);
        emit result(true, QStringLiteral("Объекты скопированы."));
    }
}

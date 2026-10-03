#include "filetransferoperation.h"
#include "7zip/Common/FilePathAutoRename.h"
#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "Common/StringConvert.h"
#include "qtfiletransfercopycallback.h"
#include "qtlanguageservices.h"
#include "qtlinkservices.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <algorithm>

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

bool FileTransferOperation::removeEntry(const QString &path) {
    return QtRemoveFileSystemEntry(path);
}

bool FileTransferOperation::validateEntry(const QString &source, QString *error) {
    if (canceled_) {
        *error = QtLocalText("Operation canceled.", "Операция отменена.");
        return false;
    }
    const QFileInfo info(source);
    if (QtIsFileSystemLink(source))
        return true;
    if (!info.exists() || (!info.isFile() && !info.isDir())) {
        *error = QtLocalText("Source unavailable: %1", "Источник недоступен: %1").arg(source);
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

bool FileTransferOperation::copyEntry(const QString &source, const QString &target, QString *error,
                                      bool reportProgress) {
    NFsFolder::CCopyState state;
    quint64 previous = 0;
    state.Progress = [this, &previous, reportProgress](quint64 completed) {
        if (reportProgress && completed != previous)
            emit advanced(qint64(completed - previous));
        previous = completed;
        return !canceled_;
    };
    if (reportProgress)
        state.CurrentFile = [this](const QString &path) { emit currentFile(path); };
    if (!state.CallProgress()) {
        *error = state.ErrorMessage;
        return false;
    }
    const QFileInfo info(source);
    const bool copied = info.isDir() && !QtIsFileSystemLink(source)
                            ? NFsFolder::CopyFolder(state, source, target)
                            : NFsFolder::CopyFile_Ask(state, source, info, target);
    *error = state.ErrorMessage;
    return copied;
}

bool FileTransferOperation::mergeEntry(
    const QString &source, const QString &stage, const QString &target,
    const std::function<int(const QString &, const QString &)> &answer, QStringList *writtenSources,
    QString *error) {
    CFileTransferCopyCallback callback(
        [this, &answer, &stage, &target](const QString &incoming, const QString &requested,
                                         QString &effective, bool &write) -> HRESULT {
            if (canceled_)
                return E_ABORT;
            const QFileInfo existing(requested);
            if (existing.isSymLink() || (existing.exists() && !existing.isFile()))
                return E_ABORT;
            if (existing.exists()) {
                const QString visible = target + requested.mid(stage.size());
                const int decision = answer(visible, incoming);
                if (decision == NOverwriteAnswer::kNo || decision == NOverwriteAnswer::kNoToAll) {
                    write = false;
                    return S_OK;
                }
                if (decision == NOverwriteAnswer::kAutoRename) {
                    FString renamed = us2fs(requested.toStdWString().c_str());
                    if (!AutoRenamePath(renamed))
                        return E_ABORT;
                    effective = QString::fromWCharArray(fs2us(renamed));
                } else if (decision == NOverwriteAnswer::kYes ||
                           decision == NOverwriteAnswer::kYesToAll) {
                    if (!QFile::remove(requested))
                        return E_ABORT;
                } else {
                    return E_ABORT;
                }
            }
            write = true;
            return S_OK;
        },
        [this](quint64 delta) {
            if (delta)
                emit advanced(qint64(delta));
            return !canceled_;
        },
        [this](const QString &path) { emit currentFile(path); });
    NFsFolder::CCopyState state;
    state.Callback = &callback;
    const bool merged = NFsFolder::CopyFolder(state, source, stage);
    *writtenSources = callback.writtenSources;
    *error = !state.ErrorMessage.isEmpty() ? state.ErrorMessage : callback.errorMessage;
    if (!merged && error->isEmpty())
        *error =
            QtLocalText("Could not merge folders: %1", "Не удалось слить папки: %1").arg(target);
    return merged;
}

bool FileTransferOperation::removeMergedSource(const QString &source,
                                               const QStringList &writtenSources, QString *error) {
    for (const QString &path : writtenSources) {
        if (canceled_) {
            *error = QtLocalText("Operation canceled.", "Операция отменена.");
            return false;
        }
        if (!adapter_.removeSource(path)) {
            *error = QtLocalText("Could not delete source after copying: %1",
                                 "Не удалось удалить источник после копирования: %1")
                         .arg(path);
            return false;
        }
    }
    QStringList directories;
    QDirIterator iterator(source, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        if (canceled_) {
            *error = QtLocalText("Operation canceled.", "Операция отменена.");
            return false;
        }
        directories << iterator.next();
    }
    std::sort(directories.begin(), directories.end(),
              [](const QString &a, const QString &b) { return a.size() > b.size(); });
    for (const QString &directory : directories)
        QDir().rmdir(directory);
    QDir().rmdir(source); // Skipped files intentionally keep the source folder.
    if (canceled_) {
        *error = QtLocalText("Operation canceled.", "Операция отменена.");
        return false;
    }
    return true;
}

void FileTransferOperation::run() {
    struct Completed {
        QString source;
        QString target;
        QString backup;
        bool copiedMove = false;
        bool sourceDeleted = false;
        bool merged = false;
        QStringList writtenSources;
    };
    enum class OverwriteMode { Ask, Skip, Overwrite, Rename };
    OverwriteMode overwriteMode = OverwriteMode::Ask;
    auto nestedAnswer = [this, &overwriteMode](const QString &existing,
                                               const QString &incoming) -> int {
        if (overwriteMode == OverwriteMode::Skip)
            return NOverwriteAnswer::kNo;
        if (overwriteMode == OverwriteMode::Overwrite)
            return NOverwriteAnswer::kYes;
        if (overwriteMode == OverwriteMode::Rename)
            return NOverwriteAnswer::kAutoRename;
        const int answer = askOverwrite(existing, incoming);
        if (answer == NOverwriteAnswer::kNoToAll)
            overwriteMode = OverwriteMode::Skip;
        else if (answer == NOverwriteAnswer::kYesToAll)
            overwriteMode = OverwriteMode::Overwrite;
        else if (answer == NOverwriteAnswer::kAutoRename)
            overwriteMode = OverwriteMode::Rename;
        return answer;
    };
    QString error;
    const bool directPath = destinationKind_ == DestinationKind::DirectPath;
    const QString destinationDirectory =
        directPath ? QFileInfo(destination_).absolutePath() : destination_;
    const QFileInfo destinationInfo(destinationDirectory);
    const QString canonicalDestination = destinationInfo.canonicalFilePath();
    QStringList targets;
    if (!destinationInfo.isDir() || destinationInfo.isSymLink() || sources_.isEmpty() ||
        (directPath && sources_.size() != 1))
        error =
            QtLocalText("The destination folder is unavailable.", "Папка назначения недоступна.");
    for (const QString &source : sources_) {
        if (!error.isEmpty())
            break;
        const QFileInfo info(source);
        const QString target =
            directPath ? destination_ : QDir(destinationDirectory).filePath(info.fileName());
        const QFileInfo targetInfo(target);
        const bool sourceLink = QtIsFileSystemLink(source);
        const bool samePath = QDir::cleanPath(info.absoluteFilePath()) ==
                                  QDir::cleanPath(targetInfo.absoluteFilePath()) ||
                              (!sourceLink && targetInfo.exists() &&
                               info.canonicalFilePath() == targetInfo.canonicalFilePath());
        if ((!sourceLink && (!info.exists() || (!info.isFile() && !info.isDir()))) ||
            QtIsFileSystemLink(target) || samePath || targets.contains(target) ||
            (!sourceLink && info.isDir() &&
             NFsFolder::IsDestChild(info.canonicalFilePath(), canonicalDestination)))
            error = QtLocalText("Source or destination unavailable, or the name is already in use.",
                                "Источник или назначение недоступны либо имя уже занято.");
        targets << target;
    }
    QList<Completed> completed;
    for (qsizetype i = 0; i < sources_.size() && error.isEmpty(); ++i) {
        if (canceled_) {
            error = QtLocalText("Operation canceled.", "Операция отменена.");
            break;
        }
        const QString source = sources_[i];
        QString target = targets[i];
        bool mayReplace = false;
        const QFileInfo sourceInfo(source);
        const QFileInfo existingInfo(target);
        const bool merge =
            existingInfo.isDir() && sourceInfo.isDir() && !QtIsFileSystemLink(source);
        if (existingInfo.exists()) {
            if (existingInfo.isDir() != sourceInfo.isDir()) {
                error =
                    QtLocalText("Cannot replace a file with a folder or a folder with a file: %1",
                                "Нельзя заменить файл папкой или папку файлом: %1")
                        .arg(target);
                break;
            }
            if (merge) {
                mayReplace = true;
            } else if (overwriteMode == OverwriteMode::Ask) {
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
                    error = QtLocalText("Operation canceled.", "Операция отменена.");
                    break;
                }
            }
            if (!error.isEmpty())
                break;
            if (!merge && overwriteMode == OverwriteMode::Skip)
                continue;
            if (!merge && overwriteMode == OverwriteMode::Rename) {
                FString renamed = us2fs(target.toStdWString().c_str());
                if (!AutoRenamePath(renamed)) {
                    error = QtLocalText("Could not find an available name: %1",
                                        "Не удалось подобрать свободное имя: %1")
                                .arg(target);
                    break;
                }
                target = QString::fromWCharArray(fs2us(renamed));
            } else if (!merge) {
                mayReplace = true;
            }
        }
        const QString suffix =
            QStringLiteral(".sevenzip-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
        const QString stage = target + suffix;
        QString backup;
        QStringList writtenSources;
        emit currentFile(sources_[i]);
        const bool valid = !move_ || validateEntry(source, &error);
        const bool renamed = valid && move_ && !merge && adapter_.trySourceRename(source, stage);
        const bool copiedMove = valid && move_ && !renamed;
        const bool staged =
            valid &&
            (merge ? (copyEntry(target, stage, &error, false) &&
                      mergeEntry(source, stage, target, nestedAnswer, &writtenSources, &error))
                   : (renamed || copyEntry(source, stage, &error)));
        if (!staged && error.isEmpty())
            error = QtLocalText("Could not move item: %1", "Не удалось переместить объект: %1")
                        .arg(sources_[i]);
        if (error.isEmpty() && canceled_)
            error = QtLocalText("Operation canceled.", "Операция отменена.");
        if (error.isEmpty() && QFileInfo(target).isSymLink())
            error = QtLocalText("Destination changed during transfer: %1",
                                "Назначение изменилось во время переноса: %1")
                        .arg(target);
        if (error.isEmpty() && QFileInfo::exists(target) && !mayReplace)
            error = QtLocalText("Destination changed during transfer: %1",
                                "Назначение изменилось во время переноса: %1")
                        .arg(target);
        if (error.isEmpty() && QFileInfo::exists(target)) {
            // Keep the old entry until every source has published successfully.
            backup = target + suffix + QStringLiteral("-backup");
            if (!QDir().rename(target, backup))
                error = QtLocalText("Could not preserve the previous item: %1",
                                    "Не удалось сохранить прежний объект: %1")
                            .arg(target);
        }
        if (error.isEmpty() && !QDir().rename(stage, target))
            error = QtLocalText("Could not finish transfer: %1", "Не удалось завершить перенос: %1")
                        .arg(target);
        if (!error.isEmpty()) {
            if (renamed)
                QDir().rename(stage, source);
            else
                removeEntry(stage);
            if (!backup.isEmpty())
                QDir().rename(backup, target);
            break;
        }
        completed.append(
            {source, target, backup, copiedMove, move_ && !copiedMove, merge, writtenSources});
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
        resultKind_ = canceled_ ? ResultKind::Canceled : ResultKind::Failure;
        emit result(false, error);
    } else if (move_) {
        for (Completed &entry : completed) {
            if (!entry.copiedMove)
                continue;
            if (canceled_) {
                error = QtLocalText("Operation canceled.", "Операция отменена.");
                break;
            }
            if (entry.merged ? !removeMergedSource(entry.source, entry.writtenSources, &error)
                             : !adapter_.removeSource(entry.source)) {
                // Recursive deletion can fail after removing some children.
                // Retain the published copy so the remaining bytes stay available.
                entry.sourceDeleted = true;
                if (error.isEmpty())
                    error = QtLocalText("Could not delete source after copying: %1",
                                        "Не удалось удалить источник после копирования: %1")
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
            resultKind_ = ResultKind::Failure;
            emit result(false, error + QtLocalText(" Some items may have been moved.",
                                                   " Часть объектов могла быть перемещена."));
            return;
        }
        for (const Completed &entry : completed)
            if (!entry.backup.isEmpty())
                removeEntry(entry.backup);
        const bool retainedSource =
            std::any_of(completed.cbegin(), completed.cend(), [](const Completed &entry) {
                return entry.merged && QFileInfo::exists(entry.source);
            });
        resultKind_ = ResultKind::Success;
        emit result(true, retainedSource
                              ? QtLocalText("Some files skipped; sources retained.",
                                            "Часть файлов пропущена; источники сохранены.")
                              : QtLocalText("Items moved.", "Объекты перемещены."));
    } else {
        for (const Completed &entry : completed)
            if (!entry.backup.isEmpty())
                removeEntry(entry.backup);
        resultKind_ = ResultKind::Success;
        emit result(true, QtLocalText("Items copied.", "Объекты скопированы."));
    }
}

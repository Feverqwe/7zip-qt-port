#include "qtfiletransfertransaction.h"
#include "7zip/Common/FilePathAutoRename.h"
#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "Common/StringConvert.h"
#include "filetransferoperation.h"
#include "qtfiletransfercopycallback.h"
#include "qtlanguageservices.h"
#include "qtlinkservices.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <algorithm>

bool QtFileTransferTransaction::removeEntry(const QString &path) {
    return QtRemoveFileSystemEntry(path);
}

bool QtFileTransferTransaction::validateEntry(const QString &source, QString *error) {
    if (worker_.canceled_) {
        *error = QCoreApplication::translate("qtfiletransfertransaction", "Operation canceled.");
        return false;
    }
    const QFileInfo info(source);
    if (QtIsFileSystemLink(source))
        return true;
    if (!info.exists() || (!info.isFile() && !info.isDir())) {
        *error = QCoreApplication::translate("qtfiletransfertransaction", "Source unavailable: %1")
                     .arg(source);
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

bool QtFileTransferTransaction::copyEntry(const QString &source, const QString &target,
                                          QString *error, bool reportProgress) {
    NFsFolder::CCopyState state;
    quint64 previous = 0;
    state.Progress = [this, &previous, reportProgress](quint64 completed) {
        if (reportProgress && completed != previous)
            emit worker_.advanced(qint64(completed - previous));
        previous = completed;
        return !worker_.canceled_;
    };
    if (reportProgress)
        state.CurrentFile = [this](const QString &path) { emit worker_.currentFile(path); };
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

bool QtFileTransferTransaction::mergeEntry(
    const QString &source, const QString &stage, const QString &target,
    const std::function<int(const QString &, const QString &)> &answer, QStringList *writtenSources,
    QString *error) {
    CFileTransferCopyCallback callback(
        [this, &answer, &stage, &target](const QString &incoming, const QString &requested,
                                         QString &effective, bool &write) -> HRESULT {
            if (worker_.canceled_)
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
                emit worker_.advanced(qint64(delta));
            return !worker_.canceled_;
        },
        [this](const QString &path) { emit worker_.currentFile(path); });
    NFsFolder::CCopyState state;
    state.Callback = &callback;
    const bool merged = NFsFolder::CopyFolder(state, source, stage);
    *writtenSources = callback.writtenSources;
    *error = !state.ErrorMessage.isEmpty() ? state.ErrorMessage : callback.errorMessage;
    if (!merged && error->isEmpty())
        *error =
            QCoreApplication::translate("qtfiletransfertransaction", "Could not merge folders: %1")
                .arg(target);
    return merged;
}

bool QtFileTransferTransaction::removeMergedSource(const QString &source,
                                                   const QStringList &writtenSources,
                                                   QString *error) {
    for (const QString &path : writtenSources) {
        if (worker_.canceled_) {
            *error =
                QCoreApplication::translate("qtfiletransfertransaction", "Operation canceled.");
            return false;
        }
        if (!worker_.adapter_.removeSource(path)) {
            *error = QCoreApplication::translate("qtfiletransfertransaction",
                                                 "Could not delete source after copying: %1")
                         .arg(path);
            return false;
        }
    }
    QStringList directories;
    QDirIterator iterator(source, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        if (worker_.canceled_) {
            *error =
                QCoreApplication::translate("qtfiletransfertransaction", "Operation canceled.");
            return false;
        }
        directories << iterator.next();
    }
    std::sort(directories.begin(), directories.end(),
              [](const QString &a, const QString &b) { return a.size() > b.size(); });
    for (const QString &directory : directories)
        QDir().rmdir(directory);
    QDir().rmdir(source); // Skipped files intentionally keep the source folder.
    if (worker_.canceled_) {
        *error = QCoreApplication::translate("qtfiletransfertransaction", "Operation canceled.");
        return false;
    }
    return true;
}

QStringList QtFileTransferTransaction::prepareTargets(QString &error) const {
    const bool directPath =
        worker_.destinationKind_ == FileTransferOperation::DestinationKind::DirectPath;
    const QString destinationDirectory =
        directPath ? QFileInfo(worker_.destination_).absolutePath() : worker_.destination_;
    const QFileInfo destinationInfo(destinationDirectory);
    const QString canonicalDestination = destinationInfo.canonicalFilePath();
    QStringList targets;
    if (!destinationInfo.isDir() || destinationInfo.isSymLink() || worker_.sources_.isEmpty() ||
        (directPath && worker_.sources_.size() != 1))
        error = QCoreApplication::translate("qtfiletransfertransaction",
                                            "The destination folder is unavailable.");
    for (const QString &source : worker_.sources_) {
        if (!error.isEmpty())
            break;
        const QFileInfo info(source);
        const QString target = directPath ? worker_.destination_
                                          : QDir(destinationDirectory).filePath(info.fileName());
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
            error = QCoreApplication::translate(
                "qtfiletransfertransaction",
                "Source or destination unavailable, or the name is already in use.");
        targets << target;
    }
    return targets;
}

void QtFileTransferTransaction::finish(QList<Completed> &completed, QString &error) {
    if (!error.isEmpty()) {
        for (qsizetype i = completed.size(); i > 0; --i) {
            const Completed &entry = completed[i - 1];
            if (worker_.move_ && !entry.copiedMove)
                QDir().rename(entry.target, entry.source);
            else
                removeEntry(entry.target);
            if (!entry.backup.isEmpty())
                QDir().rename(entry.backup, entry.target);
        }
        worker_.resultKind_ = worker_.canceled_ ? FileTransferOperation::ResultKind::Canceled
                                                : FileTransferOperation::ResultKind::Failure;
        emit worker_.result(false, error);
    } else if (worker_.move_) {
        for (Completed &entry : completed) {
            if (!entry.copiedMove)
                continue;
            if (worker_.canceled_) {
                error =
                    QCoreApplication::translate("qtfiletransfertransaction", "Operation canceled.");
                break;
            }
            if (entry.merged ? !removeMergedSource(entry.source, entry.writtenSources, &error)
                             : !worker_.adapter_.removeSource(entry.source)) {
                // Recursive deletion can fail after removing some children.
                // Retain the published copy so the remaining bytes stay available.
                entry.sourceDeleted = true;
                if (error.isEmpty())
                    error = QCoreApplication::translate("qtfiletransfertransaction",
                                                        "Could not delete source after copying: %1")
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
            worker_.resultKind_ = FileTransferOperation::ResultKind::Failure;
            emit worker_.result(
                false, error + QCoreApplication::translate("qtfiletransfertransaction",
                                                           " Some items may have been moved."));
            return;
        }
        for (const Completed &entry : completed)
            if (!entry.backup.isEmpty())
                removeEntry(entry.backup);
        const bool retainedSource =
            std::any_of(completed.cbegin(), completed.cend(), [](const Completed &entry) {
                return entry.merged && QFileInfo::exists(entry.source);
            });
        worker_.resultKind_ = FileTransferOperation::ResultKind::Success;
        emit worker_.result(
            true, retainedSource
                      ? QCoreApplication::translate("qtfiletransfertransaction",
                                                    "Some files skipped; sources retained.")
                      : QCoreApplication::translate("qtfiletransfertransaction", "Items moved."));
    } else {
        for (const Completed &entry : completed)
            if (!entry.backup.isEmpty())
                removeEntry(entry.backup);
        worker_.resultKind_ = FileTransferOperation::ResultKind::Success;
        emit worker_.result(
            true, QCoreApplication::translate("qtfiletransfertransaction", "Items copied."));
    }
}

void QtFileTransferTransaction::run() {
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
        const int answer = worker_.askOverwrite(existing, incoming);
        if (answer == NOverwriteAnswer::kNoToAll)
            overwriteMode = OverwriteMode::Skip;
        else if (answer == NOverwriteAnswer::kYesToAll)
            overwriteMode = OverwriteMode::Overwrite;
        else if (answer == NOverwriteAnswer::kAutoRename)
            overwriteMode = OverwriteMode::Rename;
        return answer;
    };
    QString error;
    QStringList targets = prepareTargets(error);
    QList<Completed> completed;
    for (qsizetype i = 0; i < worker_.sources_.size() && error.isEmpty(); ++i) {
        if (worker_.canceled_) {
            error = QCoreApplication::translate("qtfiletransfertransaction", "Operation canceled.");
            break;
        }
        const QString source = worker_.sources_[i];
        QString target = targets[i];
        bool mayReplace = false;
        const QFileInfo sourceInfo(source);
        const QFileInfo existingInfo(target);
        const bool merge =
            existingInfo.isDir() && sourceInfo.isDir() && !QtIsFileSystemLink(source);
        if (existingInfo.exists()) {
            if (existingInfo.isDir() != sourceInfo.isDir()) {
                error = QCoreApplication::translate(
                            "qtfiletransfertransaction",
                            "Cannot replace a file with a folder or a folder with a file: %1")
                            .arg(target);
                break;
            }
            if (merge) {
                mayReplace = true;
            } else if (overwriteMode == OverwriteMode::Ask) {
                const int answer = worker_.askOverwrite(target, source);
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
                    error = QCoreApplication::translate("qtfiletransfertransaction",
                                                        "Operation canceled.");
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
                    error = QCoreApplication::translate("qtfiletransfertransaction",
                                                        "Could not find an available name: %1")
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
        emit worker_.currentFile(worker_.sources_[i]);
        const bool valid = !worker_.move_ || validateEntry(source, &error);
        const bool renamed =
            valid && worker_.move_ && !merge && worker_.adapter_.trySourceRename(source, stage);
        const bool copiedMove = valid && worker_.move_ && !renamed;
        const bool staged =
            valid &&
            (merge ? (copyEntry(target, stage, &error, false) &&
                      mergeEntry(source, stage, target, nestedAnswer, &writtenSources, &error))
                   : (renamed || copyEntry(source, stage, &error)));
        if (!staged && error.isEmpty())
            error =
                QCoreApplication::translate("qtfiletransfertransaction", "Could not move item: %1")
                    .arg(worker_.sources_[i]);
        if (error.isEmpty() && worker_.canceled_)
            error = QCoreApplication::translate("qtfiletransfertransaction", "Operation canceled.");
        if (error.isEmpty() && QFileInfo(target).isSymLink())
            error = QCoreApplication::translate("qtfiletransfertransaction",
                                                "Destination changed during transfer: %1")
                        .arg(target);
        if (error.isEmpty() && QFileInfo::exists(target) && !mayReplace)
            error = QCoreApplication::translate("qtfiletransfertransaction",
                                                "Destination changed during transfer: %1")
                        .arg(target);
        if (error.isEmpty() && QFileInfo::exists(target)) {
            // Keep the old entry until every source has published successfully.
            backup = target + suffix + QStringLiteral("-backup");
            if (!QDir().rename(target, backup))
                error = QCoreApplication::translate("qtfiletransfertransaction",
                                                    "Could not preserve the previous item: %1")
                            .arg(target);
        }
        if (error.isEmpty() && !QDir().rename(stage, target))
            error = QCoreApplication::translate("qtfiletransfertransaction",
                                                "Could not finish transfer: %1")
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
        completed.append({source, target, backup, copiedMove, worker_.move_ && !copiedMove, merge,
                          writtenSources});
    }
    finish(completed, error);
}

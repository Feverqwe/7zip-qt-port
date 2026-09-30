#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/OverwriteDialog.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "mainwindow.h"

namespace {
NOverwriteDialog::CFileInfo overwriteFileInfo(const QString &path) {
    const QFileInfo source(path);
    NOverwriteDialog::CFileInfo info;
    info.Path = ToUString(path);
    info.Is_FileSystemFile = true;
    info.Is_Directory = source.isDir();
    if (source.isFile())
        info.SetSize(UInt64(source.size()));
    const qint64 ticks = (source.lastModified().toMSecsSinceEpoch() + 11644473600000LL) * 10000;
    if (ticks > 0) {
        FILETIME time{};
        time.dwLowDateTime = DWORD(quint64(ticks));
        time.dwHighDateTime = DWORD(quint64(ticks) >> 32);
        info.SetTime(time);
    }
    return info;
}
} // namespace

QString MainWindow::selectedCopyInfo() {
    CPanel &panel = app_.GetFocusedPanel();
    CSelectedState selection;
    // Synchronize Qt selection until the complete source OnNotify port is connected.
    panel.SaveSelectedState(selection);
    CRecordVector<UInt32> indices;
    panel.Get_ItemIndices_Operated(indices);
    panel.LoadFullPath();
    return panel.GetItemsInfoString(indices);
}
void MainWindow::transferFiles(const QStringList &sources, const QString &destination, bool move) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ != State::Filesystem || !QFileInfo(destination).isDir() ||
        QFileInfo(destination).isSymLink() || sources.isEmpty()) {
        QMessageBox::warning(this, "Sevenzip Desktop", "Папка назначения недоступна.");
        return;
    }
    fileTransfer_ = new FileTransferOperation(sources, destination, move, this);
    auto *operation = fileTransfer_;
    connect(operation, &FileTransferOperation::overwriteRequested, this,
            [this, operation](const QString &existing, const QString &incoming) {
                if (app_.QtOperations().closed() || operation->isCanceled()) {
                    operation->submitOverwriteAnswer(NOverwriteAnswer::kCancel);
                    return;
                }
                COverwriteDialog dialog(this);
                overwriteDialog_ = &dialog;
                dialog.OldFileInfo = overwriteFileInfo(existing);
                dialog.NewFileInfo = overwriteFileInfo(incoming);
                connect(operation, &FileTransferOperation::dismissOverwriteRequested, &dialog,
                        &QDialog::reject, Qt::QueuedConnection);
                if (operation->isCanceled()) {
                    operation->submitOverwriteAnswer(NOverwriteAnswer::kCancel);
                    return;
                }
                const int answer = NOverwriteDialog::MapButtonResult(dialog.Create(this));
                overwriteDialog_ = nullptr;
                operation->submitOverwriteAnswer(answer);
            });
    fileTransferCompletedBytes_ = 0;
    beginWorkerProgress(move ? QStringLiteral("Перемещение файлов")
                             : QStringLiteral("Копирование файлов"),
                        destination, [operation = fileTransfer_] { operation->cancel(); });
    connect(fileTransfer_, &FileTransferOperation::currentFile, this, [this](const QString &path) {
        if (app_.QtOperations().closed())
            return;
        if (progress_)
            progress_->Sync.Set_FilePath(path.toStdWString().c_str());
    });
    connect(fileTransfer_, &FileTransferOperation::advanced, this, [this](qint64 bytes) {
        fileTransferCompletedBytes_ += quint64(qMax<qint64>(0, bytes));
        if (progress_)
            progress_->Sync.Set_NumBytesCur(fileTransferCompletedBytes_);
    });
    connect(fileTransfer_, &FileTransferOperation::result, this,
            [this](bool success, const QString &message) {
                if (app_.QtOperations().closed())
                    return;
                if (!success && message != QStringLiteral("Операция отменена.") && progress_)
                    progress_->Sync.FinalMessage.ErrorMessage.Message = ToUString(message);
                closeProgressDialog(true);
                if (success)
                    statusBar()->showMessage(message);
            });
    connect(operation, &QThread::finished, this, [this, operation, sources] {
        if (app_.QtOperations().closed())
            return;
        operation->deleteLater();
        if (fileTransfer_ == operation)
            fileTransfer_ = nullptr;
        showDirectory(directory_, false);
        selectPaths(sources);
    });
    setState(State::Operating);
    fileTransfer_->start();
}

void MainWindow::copySelected() {
    if (state_ == State::Viewing) {
        extract(true);
        return;
    }
    if (state_ != State::Filesystem || selectedFilesystemPaths().isEmpty())
        return;
    transferSelected(false);
}

void MainWindow::moveSelected() {
    if (state_ == State::Viewing) {
        if (!move_->isEnabled())
            return;
        const QStringList paths = selectedArchivePaths();
        if (paths.isEmpty())
            return;
        QStringList history;
        ReadCopyHistory(history, toolbarSettingsPath());
        CopyDialog dialog(true, copyDefaultDestination(), paths, history, this, selectedCopyInfo());
        if (dialog.exec() != QDialog::Accepted)
            return;
        const QString destination = dialog.destination();
        if (!QFileInfo(destination).isDir() || QFileInfo(destination).isSymLink()) {
            QMessageBox::warning(this, "Sevenzip Desktop", "Папка назначения недоступна.");
            return;
        }
        AddUniqueStringToHeadOfList(history, destination);
        while (history.size() > 20)
            history.removeLast();
        SaveCopyHistory(history, toolbarSettingsPath());
        archiveMovePaths_ = paths;
        archiveMoveStage_ = ArchiveMoveStage::Extracting;
        previousState_ = state_;
        setState(State::Operating);
        begin("Перемещение из архива", destination);
        backend_->extract(archive_, destination, paths, archivePassword_);
        return;
    }
    if (state_ != State::Filesystem || selectedFilesystemPaths().isEmpty())
        return;
    transferSelected(true);
}

void MainWindow::transferSelected(bool move) {
    const QStringList sources = selectedFilesystemPaths();
    if (state_ != State::Filesystem || sources.isEmpty())
        return;
    QStringList history;
    ReadCopyHistory(history, toolbarSettingsPath());
    CopyDialog dialog(move, copyDefaultDestination(), sources, history, this, selectedCopyInfo());
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString destination = dialog.destination();
    transferFiles(sources, destination, move);
    if (!QFileInfo(destination).isDir())
        return;
    AddUniqueStringToHeadOfList(history, destination);
    while (history.size() > 20)
        history.removeLast();
    SaveCopyHistory(history, toolbarSettingsPath());
}

void MainWindow::deleteSelected() {
    if (state_ == State::Viewing) {
        const QStringList paths = selectedArchivePaths();
        if (info_.encrypted || paths.isEmpty() ||
            QMessageBox::question(this, "Удалить",
                                  "Удалить выбранные объекты из открытого архива?") !=
                QMessageBox::Yes)
            return;
        previousState_ = state_;
        updatingArchive_ = true;
        setState(State::Operating);
        begin("Удаление из архива", archive_);
        backend_->remove(archive_, paths);
        return;
    }
    if (state_ != State::Filesystem)
        return;
    const QStringList paths = selectedFilesystemPaths();
    if (paths.isEmpty())
        return;
    if (QMessageBox::question(this, "Удалить", "Удалить выбранные объекты из файловой системы?") !=
        QMessageBox::Yes)
        return;
    startFileDelete(paths);
}

void MainWindow::renameSelected() {
    if (state_ == State::Viewing) {
        if (!rename_->isEnabled())
            return;
        const QStringList paths = selectedArchiveItemsPaths();
        if (paths.size() != 1)
            return;
        const QString oldPath = paths.first();
        bool accepted = false;
        const QString name =
            QInputDialog::getText(this, "Переименовать", "Новое имя:", QLineEdit::Normal,
                                  oldPath.section('/', -1), &accepted);
        if (!accepted)
            return;
        if (name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
            name.contains('\\')) {
            QMessageBox::warning(this, "Sevenzip Desktop", "Недопустимое имя файла.");
            return;
        }
        const int slash = oldPath.lastIndexOf('/');
        const QString newPath = (slash < 0 ? QString() : oldPath.left(slash + 1)) + name;
        previousState_ = state_;
        updatingArchive_ = true;
        setState(State::Operating);
        begin("Переименование в архиве", archive_);
        backend_->renameEntry(archive_, oldPath, newPath);
        return;
    }
    const QStringList paths = selectedFilesystemPaths();
    if (paths.size() != 1)
        return;
    if (QFileInfo(paths.first()).isSymLink())
        return;
    const auto indices = sourceOperatedIndices();
    if (indices.Size() != 1)
        return;
    int row = -1;
    for (int i = 0; i < table_->rowCount(); ++i)
        if (table_->item(i, 0) &&
            table_->item(i, 0)->data(Qt::UserRole + 6).toUInt() == indices[0]) {
            row = i;
            break;
        }
    if (row < 0)
        return;
    auto *item = table_->item(row, 0);
    if (!item || item->data(Qt::UserRole + 6).isNull())
        return;
    renameEditingItem_ = item;
    renameOriginalPath_ = paths.first();
    renameSourceIndex_ = item->data(Qt::UserRole + 6).toUInt();
    renameActivationPending_ = true;
    if (viewStack_->currentWidget() == iconList_) {
        const QModelIndex modelIndex = table_->model()->index(row, 0);
        iconList_->scrollTo(modelIndex);
        iconList_->edit(modelIndex);
    } else {
        table_->scrollToItem(item);
        table_->editItem(item);
    }
}

void MainWindow::finishFilesystemRename(quint32 sourceIndex, const QString &originalPath,
                                        const QString &name) {
    if (state_ != State::Filesystem ||
        (QFileInfo(originalPath).absolutePath() != directory_ && !flatDisk_))
        return;
    const HRESULT result = app_.GetFocusedPanel().OnEndLabelEdit(sourceIndex, ToUString(name));
    if (result != S_OK && result != E_ABORT)
        QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось переименовать объект.");
    showDirectory(directory_, false);
    if (result == S_OK)
        selectPaths({QDir(QFileInfo(originalPath).absolutePath()).filePath(name)});
    else
        selectPaths({originalPath});
}

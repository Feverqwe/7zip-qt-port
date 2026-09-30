#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "mainwindow.h"

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
    fileTransferProgress_ = new QProgressDialog(
        move ? "Перемещение файлов…" : "Копирование файлов…", "Отмена", 0, 0, this);
    fileTransferProgress_->setObjectName("fileTransferProgress");
    fileTransferProgress_->setWindowModality(Qt::NonModal);
    fileTransferProgress_->setMinimumDuration(0);
    connect(fileTransferProgress_, &QProgressDialog::canceled, fileTransfer_,
            &FileTransferOperation::cancel);
    connect(fileTransfer_, &FileTransferOperation::currentFile, this, [this](const QString &path) {
        if (app_.QtOperations().closed())
            return;
        if (fileTransferProgress_)
            fileTransferProgress_->setLabelText(QFileInfo(path).fileName());
    });
    connect(fileTransfer_, &FileTransferOperation::result, this,
            [this](bool success, const QString &message) {
                if (app_.QtOperations().closed())
                    return;
                if (fileTransferProgress_) {
                    fileTransferProgress_->close();
                    fileTransferProgress_->deleteLater();
                    fileTransferProgress_ = nullptr;
                }
                if (success)
                    statusBar()->showMessage(message);
                else if (message != "Операция отменена.")
                    QMessageBox::warning(this, "Sevenzip Desktop", message);
            });
    auto *operation = fileTransfer_;
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
    fileTransferProgress_->show();
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
    const QFileInfo source(paths.first());
    bool accepted = false;
    const QString name = QInputDialog::getText(
        this, "Переименовать", "Новое имя:", QLineEdit::Normal, source.fileName(), &accepted);
    if (!accepted)
        return;
    const QString target = QDir(source.absolutePath()).filePath(name);
    NFsFolder::CFSFolder folder(directory_);
    folder.AddPath(paths.first());
    if (name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
        name.contains('\\') || source.isSymLink() || QFileInfo::exists(target) ||
        !folder.Rename(0, name))
        QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось переименовать объект.");
    showDirectory(directory_, false);
}

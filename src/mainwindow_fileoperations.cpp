#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "mainwindow.h"

QString MainWindow::selectedCopyInfo() const {
    CObjectVector<CPanelItem> items;
    CRecordVector<UInt32> indices;
    for (const QModelIndex &index : table_->selectionModel()->selectedRows()) {
        const int row = index.row();
        const QTableWidgetItem *name = table_->item(row, 0);
        CPanelItem item;
        item.RelPath = name->text();
        item.IsFolder = name->data(Qt::UserRole + 1).toBool();
        if (state_ == State::Viewing && item.IsFolder) {
            item.Size = proxyArchive_.Dirs[name->data(Qt::UserRole + 4).toUInt()].Size;
            item.SizeDefined = true;
        } else if (!item.IsFolder) {
            const QVariant size = table_->item(row, 2)->data(Qt::UserRole + 2);
            item.SizeDefined = size.isValid() && size.toLongLong() >= 0;
            item.Size = size.toULongLong();
        }
        indices.Add(items.Add(item));
    }
    QString prefix = state_ == State::Viewing ? archiveDisplayPath() : directory_;
    if (!prefix.endsWith('/'))
        prefix += '/';
    return CPanel(prefix, items).GetItemsInfoString(indices);
}
void MainWindow::transferFiles(const QStringList &sources, const QString &destination, bool move) {
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
        if (fileTransferProgress_)
            fileTransferProgress_->setLabelText(QFileInfo(path).fileName());
    });
    connect(fileTransfer_, &FileTransferOperation::result, this,
            [this](bool success, const QString &message) {
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
        CopyDialog dialog(true, returnDirectory_, paths, history, this, selectedCopyInfo());
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
    CopyDialog dialog(move, directory_, sources, history, this, selectedCopyInfo());
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
    const QStringList paths = selectedFilesystemPaths();
    if (paths.isEmpty())
        return;
    if (QMessageBox::question(this, "Удалить", "Удалить выбранные объекты из файловой системы?") !=
        QMessageBox::Yes)
        return;
    for (const QString &path : paths) {
        if (QFileInfo(path).isSymLink() || !QFileInfo(path).exists()) {
            QMessageBox::warning(this, "Sevenzip Desktop", "Объект недоступен.");
            break;
        }
        bool ok = QFileInfo(path).isDir() ? QDir(path).removeRecursively() : QFile::remove(path);
        if (!ok) {
            QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось удалить объект.");
            break;
        }
    }
    showDirectory(directory_, false);
}

void MainWindow::renameSelected() {
    if (state_ == State::Viewing) {
        if (!rename_->isEnabled())
            return;
        const QString oldPath =
            table_->item(table_->selectionModel()->selectedRows().first().row(), 0)
                ->data(Qt::UserRole)
                .toString();
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
    if (name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
        name.contains('\\') || source.isSymLink() || QFileInfo::exists(target) ||
        !QDir().rename(paths.first(), target))
        QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось переименовать объект.");
    showDirectory(directory_, false);
}

#include "mainwindow.h"

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
        QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
        CopyDialog dialog(true, returnDirectory_, paths,
                          settings.value("copy/history").toStringList(), this);
        if (dialog.exec() != QDialog::Accepted)
            return;
        const QString destination = dialog.destination();
        if (!QFileInfo(destination).isDir() || QFileInfo(destination).isSymLink()) {
            QMessageBox::warning(this, "Sevenzip Desktop", "Папка назначения недоступна.");
            return;
        }
        QStringList history = settings.value("copy/history").toStringList();
        history.removeAll(destination);
        history.prepend(destination);
        while (history.size() > 20)
            history.removeLast();
        settings.setValue("copy/history", history);
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
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    CopyDialog dialog(move, directory_, sources, settings.value("copy/history").toStringList(),
                      this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString destination = dialog.destination();
    transferFiles(sources, destination, move);
    if (!QFileInfo(destination).isDir())
        return;
    QStringList history = settings.value("copy/history").toStringList();
    history.removeAll(destination);
    history.prepend(destination);
    while (history.size() > 20)
        history.removeLast();
    settings.setValue("copy/history", history);
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

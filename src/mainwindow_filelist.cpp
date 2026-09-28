#include "CPP/7zip/UI/FileManager/BrowseDialog.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "mainwindow.h"

void MainWindow::showDirectory(const QString &path, bool remember) {
    const QFileInfo info(path);
    if (!info.isDir()) {
        statusBar()->showMessage("Папка не найдена: " + path);
        path_->setText(state_ == State::Viewing ? archiveDisplayPath() : directory_);
        return;
    }
    const QString next = info.canonicalFilePath();
    if (remember && !directory_.isEmpty() && directory_ != next)
        history_.append(directory_);
    filesystemScanner_->cancel();
    scanningDisk_ = false;
    scanSelection_.clear();
    scanSelectedState_.reset();
    directory_ = next;
    recordVisitedPath(directory_);
    archive_.clear();
    displayArchive_.clear();
    archiveParents_.clear();
    archivePassword_.clear();
    archiveFolder_.clear();
    {
        const QSignalBlocker blocked(flatView_);
        flatView_->setChecked(flatDisk_);
    }
    pendingArchiveFolder_.clear();
    refreshing_ = false;
    refreshFolder_.clear();
    refreshSelection_.clear();
    entries_.clear();
    table_->clearSelection();
    table_->setSortingEnabled(false);
    table_->setRowCount(0);
    path_->setText(directory_);
    const QDir::Filters filter =
        QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System;
    static_cast<FileTable *>(table_)->filesystem = true;
    table_->setColumnHidden(3, true);
    if (flatDisk_) {
        scanningDisk_ = true;
        setState(State::Filesystem);
        statusBar()->showMessage("Сканирование вложенных папок…");
        filesystemScanner_->start(directory_, filter);
    } else {
        const QFileInfoList files = QDir(directory_).entryInfoList(filter, QDir::NoSort);
        for (const QFileInfo &file : files)
            appendFilesystemEntry(file);
        table_->setSortingEnabled(true);
        applySortMode();
        setState(State::Filesystem);
        statusBar()->showMessage(QStringLiteral("%1 объектов").arg(files.size()));
    }
}

void MainWindow::appendFilesystemEntry(const QFileInfo &file) {
    const int row = table_->rowCount();
    table_->insertRow(row);
    const QString displayName =
        flatDisk_ ? QDir(directory_).relativeFilePath(file.absoluteFilePath()) : file.fileName();
    const QStringList values = {displayName,
                                file.absoluteFilePath(),
                                file.isDir() ? QString() : QString::number(file.size()),
                                QString(),
                                file.lastModified().toString(Qt::ISODate),
                                GetExtensionPtr(QStringView(file.fileName())).toString(),
                                QString::number(row)};
    for (int col = 0; col < values.size(); ++col) {
        auto *item = new ArchiveTableItem(values[col]);
        item->setData(Qt::UserRole + 1, file.isDir());
        item->setData(Qt::UserRole + 3, file.fileName());
        item->setData(Qt::UserRole + 5,
                      displayName.left(displayName.size() - file.fileName().size()));
        item->setData(Qt::UserRole + 6, row);
        if (col == 0) {
            if (fileListSettings_.ShowRealFileIcons) {
                QFileIconProvider icons;
                item->setIcon(icons.icon(file));
            } else
                item->setIcon(QApplication::style()->standardIcon(
                    file.isDir() ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
        }
        if (col == 2 && !file.isDir())
            item->setData(Qt::UserRole + 2, file.size());
        if (col == 6)
            item->setData(Qt::UserRole + 2, row);
        if (col == 2 || col == 3)
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table_->setItem(row, col, item);
    }
}

QStringList MainWindow::selectedPaths() const {
    QStringList paths;
    for (const QModelIndex &index : table_->selectionModel()->selectedRows())
        paths << table_->item(index.row(), 1)->text();
    return paths;
}

void MainWindow::selectPaths(const QStringList &paths) {
    table_->clearSelection();
    for (int row = 0; row < table_->rowCount(); ++row) {
        if (paths.contains(table_->item(row, 1)->text()))
            table_->selectionModel()->select(table_->model()->index(row, 0),
                                             QItemSelectionModel::Select |
                                                 QItemSelectionModel::Rows);
    }
}

void MainWindow::refreshCurrent() {
    if (state_ == State::Filesystem) {
        CSelectedState selection;
        app_.GetFocusedPanel().SaveSelectedState(selection);
        showDirectory(directory_, false);
        if (scanningDisk_)
            scanSelectedState_ = selection;
        else
            app_.GetFocusedPanel().RefreshListCtrl(selection);
    } else if (state_ == State::Viewing) {
        refreshing_ = true;
        refreshFolder_ = archiveFolder_;
        refreshSelection_ = selectedPaths();
        setState(State::Loading);
        begin("Обновление архива", archive_);
        backend_->list(archive_);
    }
}

void MainWindow::createFolder() {
    if (state_ != State::Filesystem)
        return;
    QString name;
    if (!Dlg_CreateFolder(this, name))
        return;
    const QString target = QDir(directory_).filePath(name);
    NFsFolder::CFSFolder folder(directory_);
    if (name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
        name.contains('\\') || name.contains(QRegularExpression("[<>:\"|?*]")) ||
        name.endsWith(' ') || name.endsWith('.') || QFileInfo::exists(target) ||
        QFileInfo(target).isSymLink() || !folder.CreateFolder(name)) {
        QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось создать папку.");
        return;
    }
    showDirectory(directory_, false);
    selectPaths({target});
}

void MainWindow::createFile() {
    if (state_ != State::Filesystem)
        return;
    ComboDialog dialog("Создать файл", "Имя файла:", "Новый файл", this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString name = dialog.value();
    const QString target = QDir(directory_).filePath(name);
    if (name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
        name.contains('\\') || name.contains(QRegularExpression("[<>:\"|?*]")) ||
        name.endsWith(' ') || name.endsWith('.') || QFileInfo::exists(target) ||
        QFileInfo(target).isSymLink()) {
        QMessageBox::warning(this, "Создать файл", "Недопустимое имя или файл уже существует.");
        return;
    }
    NFsFolder::CFSFolder folder(directory_);
    if (!folder.CreateFile(name)) {
        QMessageBox::warning(this, "Создать файл", "Не удалось создать файл.");
        return;
    }
    showDirectory(directory_, false);
    selectPaths({target});
}

void MainWindow::navigateTo(const QString &path) {
    if (state_ == State::Loading || state_ == State::Operating)
        return;
    if (state_ == State::Viewing && path == archiveDisplayPath())
        return;
    const QFileInfo info(path);
    if (info.isDir())
        showDirectory(path, true);
    else if (info.isFile() && !info.isSymLink())
        openArchive(path);
    else {
        // The Qt address presentation separates an archive from its internal
        // folder with " / ". Prefer real filesystem paths above, then locate
        // an existing archive prefix rather than splitting folder names.
        qsizetype separator = path.indexOf(QStringLiteral(" / "));
        while (separator >= 0) {
            const QFileInfo archiveInfo(path.left(separator));
            if (archiveInfo.isFile() && !archiveInfo.isSymLink()) {
                QString folder = path.mid(separator + 3);
                if (state_ == State::Viewing && archiveParents_.empty() &&
                    archiveInfo.absoluteFilePath() == archive_) {
                    while (!folder.isEmpty() && !archiveFolderExists(folder)) {
                        const qsizetype slash = folder.lastIndexOf('/');
                        folder = slash < 0 ? QString() : folder.left(slash);
                    }
                    showArchiveFolder(folder);
                } else {
                    pendingArchiveFolder_ = folder;
                    openArchive(archiveInfo.absoluteFilePath());
                }
                return;
            }
            separator = path.indexOf(QStringLiteral(" / "), separator + 3);
        }
        statusBar()->showMessage("Путь не найден или тип файла не поддерживается: " + path);
        path_->setText(state_ == State::Viewing ? archiveDisplayPath() : directory_);
    }
}

void MainWindow::activateRow(int row) {
    if (state_ == State::Loading || state_ == State::Operating)
        return;
    if (row < 0 || row >= table_->rowCount())
        return;
    if (state_ == State::Filesystem) {
        const QString path = table_->item(row, 1)->text();
        const QFileInfo info(path);
        if (info.isDir() || (info.isFile() && ArchiveFormats::canOpen(path)))
            navigateTo(path);
        else if (info.isFile() && !info.isSymLink())
            openLocalFile(path);
    } else if (state_ == State::Viewing) {
        const QTableWidgetItem *item = table_->item(row, 0);
        if (item->data(Qt::UserRole + 1).toBool())
            showArchiveFolder(item->data(Qt::UserRole).toString());
        else
            openArchiveFile(item->data(Qt::UserRole).toString());
    }
}

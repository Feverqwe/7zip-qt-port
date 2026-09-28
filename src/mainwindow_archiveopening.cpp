#include "mainwindow.h"

void MainWindow::openArchive(const QString &path, bool nested, const QString &label) {
    if (state_ == State::Loading || state_ == State::Operating)
        return;
    const QFileInfo file(path);
    if (!ArchiveFormats::canOpen(path) && (!file.isFile() || file.isSymLink())) {
        QMessageBox::warning(this, "Sevenzip Desktop",
                             "Формат архива не поддерживается для открытия.");
        return;
    }
    filesystemScanner_->cancel();
    if (!nested)
        archiveParents_.clear();
    scanningDisk_ = false;
    scanSelectedState_.reset();
    app_.GetFocusedPanel().ReleaseFolder();
    archiveRoot_.Release();
    listingFolderError_.clear();
    archive_ = QFileInfo(path).absoluteFilePath();
    displayArchive_ = nested ? label : archive_;
    archivePassword_.clear();
    if (infoDialog_)
        infoDialog_->close();
    if (state_ == State::Filesystem)
        returnDirectory_ = directory_;
    else if (returnDirectory_.isEmpty())
        returnDirectory_ = QFileInfo(archive_).absolutePath();
    entries_.clear();
    info_ = ArchiveInfo{};
    archiveFolder_.clear();
    refreshing_ = false;
    refreshFolder_.clear();
    refreshSelectedState_ = CSelectedState();
    table_->setRowCount(0);
    path_->setText(archive_);
    setState(State::Loading);
    begin("Открытие архива", archive_);
    nestedListing_ = nested;
    backend_->list(archive_);
}

void MainWindow::openSelectedInside() {
    if (!openInside_->isEnabled())
        return;
    const int row = table_->currentRow();
    if (state_ == State::Filesystem)
        navigateTo(table_->item(row, 1)->text());
    else if (table_->item(row, 0)->data(Qt::UserRole + 1).toBool())
        showArchiveFolder(table_->item(row, 0)->data(Qt::UserRole).toString());
    else
        openNestedArchive(table_->item(row, 0)->data(Qt::UserRole).toString());
}

void MainWindow::openSelectedOutside() {
    if (!openOutside_->isEnabled())
        return;
    const int row = table_->currentRow();
    if (state_ == State::Filesystem)
        openLocalFile(table_->item(row, 1)->text());
    else
        openArchiveFile(table_->item(row, 0)->data(Qt::UserRole).toString());
}

void MainWindow::openNestedArchive(const QString &entry) {
    nestedTemporary_ = std::make_unique<QTemporaryDir>();
    if (!nestedTemporary_->isValid()) {
        nestedTemporary_.reset();
        QMessageBox::warning(this, "Открыть архив", "Не удалось создать временную папку.");
        return;
    }
    pendingViewPath_ = QDir(nestedTemporary_->path()).filePath(entry);
    pendingNestedEntry_ = entry;
    pendingNested_ = true;
    previousState_ = state_;
    setState(State::Operating);
    begin("Открытие вложенного архива", entry);
    backend_->extract(archive_, nestedTemporary_->path(), {entry}, archivePassword_);
}

bool MainWindow::finishNestedExtraction(bool ok) {
    pendingNested_ = false;
    const QString file = pendingViewPath_;
    pendingViewPath_.clear();
    if (ok && QFileInfo(file).isFile()) {
        ArchiveParent parent;
        parent.path = archive_;
        parent.display = displayArchive_;
        parent.password = archivePassword_;
        parent.folder = archiveFolder_;
        parent.entry = pendingNestedEntry_;
        parent.entries = entries_;
        parent.info = info_;
        parent.root = archiveRoot_;
        parent.temporary = std::move(nestedTemporary_);
        archiveParents_.push_back(std::move(parent));
        const QString label = displayArchive_ + " / " + pendingNestedEntry_;
        pendingNestedEntry_.clear();
        openArchive(file, true, label);
        return true;
    }
    nestedTemporary_.reset();
    pendingNestedEntry_.clear();
    return false;
}

void MainWindow::restoreParentArchive() {
    ArchiveParent parent = std::move(archiveParents_.back());
    archiveParents_.pop_back();
    archive_ = parent.path;
    displayArchive_ = parent.display;
    archivePassword_ = parent.password;
    entries_ = std::move(parent.entries);
    archiveRoot_ = parent.root;
    info_ = parent.info;
    nestedListing_ = false;
    showArchiveFolder(parent.folder);
    setState(State::Viewing);
    selectPaths({parent.entry});
}

void MainWindow::goBack() {
    if (state_ == State::Loading || state_ == State::Operating)
        return;
    if (state_ == State::Viewing) {
        if (archiveFolder_.isEmpty()) {
            if (archiveParents_.empty())
                showDirectory(returnDirectory_, false);
            else
                restoreParentArchive();
        } else
            goUp();
    } else if (!history_.isEmpty()) {
        showDirectory(history_.takeLast(), false);
    }
}

void MainWindow::goUp() {
    if (state_ != State::Viewing || archiveFolder_.isEmpty())
        return;
    const int separator = archiveFolder_.lastIndexOf('/');
    showArchiveFolder(separator < 0 ? QString() : archiveFolder_.left(separator));
}

void MainWindow::goParent() {
    if (state_ == State::Viewing) {
        goBack();
    } else if (state_ == State::Filesystem) {
        QDir parent(directory_);
        if (parent.cdUp())
            showDirectory(parent.absolutePath(), true);
    }
}

QString MainWindow::archiveDisplayPath() const {
    return displayArchive_ +
           (archiveFolder_.isEmpty() ? QString() : QStringLiteral(" / ") + archiveFolder_);
}

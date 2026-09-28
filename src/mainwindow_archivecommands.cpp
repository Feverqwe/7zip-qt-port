#include "mainwindow.h"

void MainWindow::calculateHash(const QString &method) {
    if ((state_ != State::Filesystem && state_ != State::Viewing) ||
        table_->selectionModel()->selectedRows().isEmpty())
        return;
    hashMethod_ = method;
    previousState_ = state_;
    if (state_ == State::Filesystem) {
        const QStringList paths = selectedFilesystemPaths();
        if (paths.isEmpty())
            return;
        hashing_ = true;
        setState(State::Operating);
        begin(QStringLiteral("Расчёт контрольных сумм"), method);
        backend_->hash(paths, method);
        return;
    }
    hashSelection_.clear();
    for (const QModelIndex &index : table_->selectionModel()->selectedRows())
        hashSelection_ << table_->item(index.row(), 0)->data(Qt::UserRole).toString();
    const QStringList entries = selectedArchivePaths();
    if (entries.isEmpty())
        return;
    hashTemporary_ = std::make_unique<QTemporaryDir>();
    if (!hashTemporary_->isValid()) {
        hashTemporary_.reset();
        QMessageBox::warning(this, QStringLiteral("Контрольные суммы"),
                             QStringLiteral("Не удалось создать временную папку."));
        return;
    }
    pendingHashExtraction_ = true;
    setState(State::Operating);
    begin(QStringLiteral("Извлечение для расчёта контрольных сумм"), archive_);
    backend_->extract(archive_, hashTemporary_->path(), entries, archivePassword_);
}

void MainWindow::openArchiveFile(const QString &entry) {
    viewTemporary_ = std::make_unique<QTemporaryDir>();
    if (!viewTemporary_->isValid()) {
        viewTemporary_.reset();
        QMessageBox::warning(this, QStringLiteral("Открыть файл"),
                             QStringLiteral("Не удалось создать временную папку."));
        return;
    }
    pendingViewPath_ = QDir(viewTemporary_->path()).filePath(entry);
    pendingView_ = true;
    pendingExternalOpen_ = true;
    previousState_ = state_;
    setState(State::Operating);
    begin(QStringLiteral("Открытие файла"), entry);
    backend_->extract(archive_, viewTemporary_->path(), {entry}, archivePassword_);
}

void MainWindow::viewSelectedFile(bool editable) {
    if (state_ != State::Filesystem && state_ != State::Viewing)
        return;
    const auto rows = table_->selectionModel()->selectedRows();
    if (rows.size() != 1)
        return;
    const int row = rows.first().row();
    if (table_->item(row, 0)->data(Qt::UserRole + 1).toBool())
        return;
    if (state_ == State::Filesystem) {
        const QString path = table_->item(row, 1)->text();
        if (QFileInfo(path).isFile() && !QFileInfo(path).isSymLink())
            openFileViewer(path, editable);
        return;
    }
    if (editable && (info_.encrypted || !archiveParents_.empty()))
        return;
    const QString entry = table_->item(row, 0)->data(Qt::UserRole).toString();
    for (const ArchiveEntry &item : entries_) {
        if (item.path == entry && item.size > 16 * 1024 * 1024) {
            QMessageBox::information(this, "Просмотр",
                                     "Файл больше 16 МиБ. Просмотр текста недоступен.");
            return;
        }
    }
    viewTemporary_ = std::make_unique<QTemporaryDir>();
    if (!viewTemporary_->isValid()) {
        viewTemporary_.reset();
        QMessageBox::warning(this, "Просмотр", "Не удалось создать временную папку.");
        return;
    }
    pendingViewPath_ = QDir(viewTemporary_->path()).filePath(entry);
    pendingView_ = true;
    pendingExternalOpen_ = false;
    pendingViewEditable_ = editable;
    pendingEditEntry_ = editable ? entry : QString();
    previousState_ = state_;
    setState(State::Operating);
    begin(editable ? "Редактирование файла" : "Просмотр файла", entry);
    backend_->extract(archive_, viewTemporary_->path(), {entry}, archivePassword_);
}

void MainWindow::extract(bool onlySelected) {
    if (state_ != State::Viewing || entries_.isEmpty())
        return;
    if (onlySelected && table_->selectionModel()->selectedRows().isEmpty())
        return;
    ExtractDialog dialog(returnDirectory_, {}, onlySelected, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString destination = dialog.destination();
    if (destination.isEmpty())
        return;
    extractTo(destination, onlySelected, dialog.password(), dialog.options());
}

void MainWindow::createDialog() {
    if (state_ != State::Filesystem)
        return;
    QStringList selectedPaths;
    for (const QModelIndex &index : table_->selectionModel()->selectedRows())
        selectedPaths << table_->item(index.row(), 1)->text();
    CreateArchiveDialog dialog(directory_, selectedPaths, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    createArchive(dialog.target(), dialog.sources(), dialog.password(), dialog.encryptHeaders(),
                  dialog.compressionOptions());
}

void MainWindow::createArchive(const QString &target, const QStringList &sources,
                               const QString &password, bool encryptHeaders,
                               const CompressionOptions &options) {
    if (state_ != State::Filesystem || sources.isEmpty() || target.isEmpty())
        return;
    previousState_ = state_;
    creating_ = true;
    setState(State::Operating);
    begin("Создание архива", target);
    backend_->create(target, sources, password, encryptHeaders, options);
}

void MainWindow::addDialog() {
    if (state_ != State::Viewing)
        return;
    CreateArchiveDialog dialog(returnDirectory_, {}, this, archive_);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (state_ != State::Viewing || dialog.sources().isEmpty())
        return;
    addSources(dialog.sources());
}

QStringList MainWindow::selectedFilesystemPaths() const {
    QStringList paths;
    if (state_ != State::Filesystem)
        return paths;
    for (const QModelIndex &index : table_->selectionModel()->selectedRows())
        paths << table_->item(index.row(), 1)->text();
    return paths;
}

void MainWindow::extractTo(const QString &destination, bool onlySelected, const QString &password,
                           const ExtractionOptions &options) {
    if (state_ != State::Viewing || entries_.isEmpty() || destination.isEmpty())
        return;
    QStringList names;
    if (onlySelected) {
        names = selectedArchivePaths();
        if (names.isEmpty())
            return;
    }
    previousState_ = state_;
    setState(State::Operating);
    begin("Извлечение", destination);
    backend_->extract(archive_, destination, names,
                      password.isEmpty() ? archivePassword_ : password, options);
}

void MainWindow::testArchive() {
    if (state_ != State::Viewing)
        return;
    previousState_ = state_;
    setState(State::Operating);
    begin("Проверка архива", archive_);
    backend_->test(archive_, archivePassword_);
}

void MainWindow::addSources(const QStringList &sources) {
    if (state_ != State::Viewing || !ArchiveFormats::canModify(archive_) || info_.encrypted ||
        sources.isEmpty())
        return;
    previousState_ = state_;
    updatingArchive_ = true;
    setState(State::Operating);
    begin("Добавление в архив", archive_);
    backend_->add(archive_, sources);
}

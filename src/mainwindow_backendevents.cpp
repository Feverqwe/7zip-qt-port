#include "mainwindow.h"

void MainWindow::connectBackend() {
    connect(backend_, &ArchiveBackend::entriesReady, this,
            [this](const QList<ArchiveEntry> &entries) {
                if (state_ != State::Loading)
                    return;
                entries_ = entries;
                proxyArchive_.Load(entries_);
                QString folder = refreshing_ ? refreshFolder_ : pendingArchiveFolder_;
                while (!folder.isEmpty()) {
                    bool present = false;
                    for (const ArchiveEntry &entry : entries_) {
                        present = entry.path == folder || entry.path == folder + '/' ||
                                  entry.path.startsWith(folder + '/');
                        if (present)
                            break;
                    }
                    if (present)
                        break;
                    const int separator = folder.lastIndexOf('/');
                    folder = separator < 0 ? QString() : folder.left(separator);
                }
                {
                    const QSignalBlocker blocked(flatView_);
                    flatView_->setChecked(flatArchive_);
                }
                showArchiveFolder(folder);
                pendingArchiveFolder_.clear();
                if (refreshing_)
                    selectPaths(refreshSelection_);
                refreshing_ = false;
                refreshFolder_.clear();
                refreshSelection_.clear();
                setState(State::Viewing);
            });
    connect(backend_, &ArchiveBackend::infoReady, this,
            [this](const ArchiveInfo &info) { info_ = info; });
    connect(backend_, &ArchiveBackend::passwordRequired, this, [this] {
        bool accepted = false;
        const QString password = QInputDialog::getText(
            this, "Пароль архива", "Введите пароль:", QLineEdit::Password, {}, &accepted);
        if (!accepted || password.isEmpty()) {
            backend_->cancel();
            return;
        }
        archivePassword_ = password;
        backend_->providePassword(password);
    });
    connect(backend_, &ArchiveBackend::outputReceived, this,
            [this](const QString &line) { appendJournal(line); });
    connect(backend_, &ArchiveBackend::progress, this, [this](int percent, const QString &detail) {
        if (!progress_)
            return;
        if (percent >= 0)
            progress_->setValue(percent);
        if (!detail.isEmpty()) {
            currentDetail_ = detail;
            progress_->setLabelText(operationName_ + "\n" + currentDetail_);
            statusBar()->showMessage(operationName_ + ": " + currentDetail_);
        }
    });
    connect(backend_, &ArchiveBackend::finished, this, &MainWindow::backendFinished);
}

void MainWindow::backendFinished(bool ok, const QString &message) {
    if (startPendingHash(ok))
        return;
    if (pendingHashExtraction_) {
        pendingHashExtraction_ = false;
        hashSelection_.clear();
        hashTemporary_.reset();
    }
    const bool hashFinished = hashing_;
    if (hashing_) {
        hashing_ = false;
        hashTemporary_.reset();
    }
    if (startPendingArchiveMove(ok))
        return;
    QString resultMessage = message;
    if (archiveMoveStage_ == ArchiveMoveStage::Removing)
        resultMessage = ok ? QStringLiteral("Объекты перемещены из архива.")
                           : QStringLiteral("Файлы извлечены, но не удалены из архива. ") + message;
    if (archiveMoveStage_ != ArchiveMoveStage::None) {
        archiveMoveStage_ = ArchiveMoveStage::None;
        archiveMovePaths_.clear();
    }
    if (nestedListing_ && ok)
        nestedListing_ = false;
    if (!ok)
        archivePassword_.clear();
    appendJournal((ok ? QStringLiteral("Результат: ") : QStringLiteral("Ошибка: ")) +
                  resultMessage);
    if (hashFinished && ok)
        QTimer::singleShot(0, this, [this] { showJournal(); });
    closeProgressDialog();
    if (finishArchiveState(ok))
        return;
    finishViewExtraction(ok);
    if (pendingNested_) {
        if (finishNestedExtraction(ok))
            return;
    }
    statusBar()->showMessage(resultMessage);
    if (!ok && resultMessage != "Операция отменена.")
        QMessageBox::warning(this, "Sevenzip Desktop", resultMessage);
}

bool MainWindow::startPendingHash(bool ok) {
    if (pendingHashExtraction_ && ok) {
        pendingHashExtraction_ = false;
        QStringList files;
        for (const QString &entry : hashSelection_)
            files << QDir(hashTemporary_->path()).filePath(entry);
        hashSelection_.clear();
        closeProgressDialog();
        begin(QStringLiteral("Расчёт контрольных сумм"), hashMethod_, true);
        hashing_ = true;
        backend_->hash(files, hashMethod_);
        return true;
    }
    return false;
}

bool MainWindow::startPendingArchiveMove(bool ok) {
    if (archiveMoveStage_ == ArchiveMoveStage::Extracting && ok) {
        appendJournal(QStringLiteral("Извлечение завершено. Удаление исходных записей."));
        archiveMoveStage_ = ArchiveMoveStage::Removing;
        updatingArchive_ = true;
        closeProgressDialog();
        begin("Удаление перемещённых записей", archive_, true);
        backend_->remove(archive_, archiveMovePaths_);
        return true;
    }
    return false;
}

bool MainWindow::finishArchiveState(bool ok) {
    if (state_ == State::Loading && !ok) {
        if (nestedListing_ && !archiveParents_.empty())
            restoreParentArchive();
        else
            showDirectory(returnDirectory_, false);
    } else if (state_ == State::Operating) {
        if (replacingArchiveEntry_) {
            replacingArchiveEntry_ = false;
            viewTemporary_.reset();
            if (ok) {
                refreshing_ = true;
                refreshFolder_ = archiveFolder_;
                refreshSelection_ = {pendingEditEntry_};
            }
            pendingEditEntry_.clear();
        }
        if (creating_)
            showDirectory(directory_, false);
        else if (updatingArchive_ && ok) {
            updatingArchive_ = false;
            entries_.clear();
            table_->setRowCount(0);
            setState(State::Loading);
            backend_->list(archive_);
            return true;
        } else
            setState(previousState_ == State::Viewing ? State::Viewing : State::Filesystem);
        creating_ = false;
        updatingArchive_ = false;
    }
    return false;
}

void MainWindow::finishViewExtraction(bool ok) {
    if (pendingView_) {
        const QString file = pendingViewPath_;
        const bool external = pendingExternalOpen_;
        const bool editable = pendingViewEditable_;
        pendingView_ = false;
        pendingExternalOpen_ = false;
        pendingViewEditable_ = false;
        pendingViewPath_.clear();
        if (ok && QFileInfo(file).isFile()) {
            QTimer::singleShot(0, this, [this, file, external, editable] {
                if (external) {
                    if (openLocalFile(file))
                        openedTemporaries_.push_back(std::move(viewTemporary_));
                    else
                        viewTemporary_.reset();
                } else
                    openFileViewer(file, editable);
            });
        } else {
            pendingEditEntry_.clear();
            viewTemporary_.reset();
        }
    }
}

void MainWindow::closeProgressDialog() {
    if (!progress_)
        return;
    QProgressDialog *completed = progress_;
    progress_ = nullptr;
    completed->disconnect(backend_);
    completed->close();
    completed->deleteLater();
}

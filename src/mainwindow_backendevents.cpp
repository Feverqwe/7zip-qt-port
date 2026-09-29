#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "mainwindow.h"

void MainWindow::connectBackend() {
    connect(backend_, &ArchiveBackend::infoReady, this, [this](const ArchiveInfo &info) {
        if (!app_.QtOperations().closed() && !sourceArchiveProgress_)
            info_ = info;
    });
    connect(backend_, &ArchiveBackend::passwordRequired, this, [this] {
        if (app_.QtOperations().closed() || sourceArchiveProgress_)
            return;
        CPasswordDialog dialog(this);
        dialog.OnInit();
        if (dialog.exec() != QDialog::Accepted || dialog.Password.isEmpty()) {
            backend_->cancel();
            return;
        }
        archivePassword_ = dialog.Password;
        backend_->providePassword(dialog.Password);
    });
    connect(backend_, &ArchiveBackend::outputReceived, this, [this](const QString &line) {
        if (!app_.QtOperations().closed())
            appendJournal(line);
    });
    connect(backend_, &ArchiveBackend::progress, this, [this](int percent, const QString &detail) {
        if (app_.QtOperations().closed() || !progress_)
            return;
        if (percent >= 0)
            progress_->setValue(percent);
        if (!detail.isEmpty()) {
            currentDetail_ = detail;
            progress_->setLabelText(operationName_ + "\n" + currentDetail_);
            statusBar()->showMessage(operationName_ + ": " + currentDetail_);
        }
    });
    connect(backend_, &ArchiveBackend::finished, this,
            [this](bool ok, const QString &message) { backendFinished(ok, message); });
}

void MainWindow::backendFinished(bool ok, const QString &message, bool messageDisplayed) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (sourceArchiveProgress_)
        return; // Source worker owns completion until its modal lifecycle joins.
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
    if (!ok)
        archivePassword_.clear();
    appendJournal((ok ? QStringLiteral("Результат: ") : QStringLiteral("Ошибка: ")) +
                  resultMessage);
    if (hashFinished && ok)
        QTimer::singleShot(0, this, [this] {
            if (!app_.QtOperations().closed())
                showJournal();
        });
    closeProgressDialog();
    if (finishArchiveState(ok))
        return;
    finishViewExtraction(ok);
    statusBar()->showMessage(resultMessage);
    if (!ok && !messageDisplayed && resultMessage != "Операция отменена.")
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
        showDirectory(returnDirectory_, false);
    } else if (state_ == State::Operating) {
        if (replacingArchiveEntry_) {
            replacingArchiveEntry_ = false;
            viewTemporary_.reset();
            pendingEditEntry_.clear();
        }
        if (creating_)
            showDirectory(directory_, false);
        else if (updatingArchive_ && ok) {
            updatingArchive_ = false;
            // Mutation completion and native reopen are one guarded transition.
            // Change the presentation mode without enabling commands in between.
            state_ = State::Viewing;
            refreshCurrent();
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
                if (app_.QtOperations().closed())
                    return;
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

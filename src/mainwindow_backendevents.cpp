#include "7zip/UI/FileManager/resourceGui.h"
#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "CPP/7zip/UI/FileManager/OverwriteDialog.h"
#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "mainwindow.h"
#include "qtapppresentation.h"
#include "qtlangcontrols.h"
#include <QCoreApplication>

void MainWindow::connectBackend() {
    connect(backend_, &ArchiveBackend::overwriteRequired, this,
            [this](const QString &existing, const ArchiveEntry &incoming) {
                if (app_.QtOperations().closed() || sourceArchiveProgress_) {
                    backend_->provideOverwriteAnswer(NOverwriteAnswer::kCancel);
                    return;
                }
                COverwriteDialog dialog(this);
                overwriteDialog_ = &dialog;
                const QFileInfo old(existing);
                dialog.OldFileInfo.Path = ToUString(existing);
                dialog.OldFileInfo.Is_FileSystemFile = true;
                dialog.OldFileInfo.SetSize(UInt64(old.size()));
                dialog.NewFileInfo.Path = ToUString(incoming.path);
                dialog.NewFileInfo.SetSize(UInt64(qMax<qint64>(0, incoming.size)));
                const auto setTime = [](NOverwriteDialog::CFileInfo &info, const QDateTime &date) {
                    if (!date.isValid())
                        return;
                    const qint64 ticks = (date.toMSecsSinceEpoch() + 11644473600000LL) * 10000;
                    FILETIME time{};
                    time.dwLowDateTime = DWORD(quint64(ticks));
                    time.dwHighDateTime = DWORD(quint64(ticks) >> 32);
                    info.SetTime(time);
                };
                setTime(dialog.OldFileInfo, old.lastModified());
                setTime(dialog.NewFileInfo,
                        QDateTime::fromString(incoming.modified, "yyyy-MM-dd HH:mm:ss"));
                connect(backend_, &ArchiveBackend::dismissOverwriteRequested, &dialog,
                        &QDialog::reject, Qt::QueuedConnection);
                const int answer = NOverwriteDialog::MapButtonResult(dialog.Create(this));
                overwriteDialog_ = nullptr;
                backend_->provideOverwriteAnswer(answer);
            });
    connect(backend_, &ArchiveBackend::infoReady, this, [this](const ArchiveInfo &info) {
        if (!app_.QtOperations().closed() && !sourceArchiveProgress_)
            info_ = info;
    });
    connect(backend_, &ArchiveBackend::passwordRequired, this, [this] {
        if (app_.QtOperations().closed() || sourceArchiveProgress_)
            return;
        CPasswordDialog dialog(this);
        dialog.OnInit();
        if (dialog.exec() != QDialog::Accepted) {
            backend_->cancel();
            return;
        }
        archivePassword_ = ToQString(dialog.Password);
        backend_->providePassword(archivePassword_);
    });
    connect(backend_, &ArchiveBackend::outputReceived, this, [this](const QString &line) {
        if (!app_.QtOperations().closed()) {
            if (progress_ && (line.startsWith(QStringLiteral("ERROR:"), Qt::CaseInsensitive) ||
                              line.startsWith(QStringLiteral("WARNING:"), Qt::CaseInsensitive)))
                progress_->Sync.AddError_Message(line.toStdWString().c_str());
        }
    });
    connect(backend_, &ArchiveBackend::progress, this, [this](int percent, const QString &detail) {
        if (app_.QtOperations().closed() || !progress_)
            return;
        progress_->SetTransportPercent(percent);
        if (!detail.isEmpty()) {
            currentDetail_ = detail;
            progress_->Sync.Set_FilePath(detail.toStdWString().c_str());
            statusBar()->showMessage(operationName_ + ": " + currentDetail_);
        }
    });
    connect(backend_, &ArchiveBackend::finished, this, [this](bool ok, const QString &message) {
        backendFinished(ok, message, backend_->lastResult());
    });
}

void MainWindow::backendFinished(bool ok, const QString &message, HRESULT result,
                                 bool messageDisplayed) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (sourceArchiveProgress_)
        return; // Source worker owns completion until its modal lifecycle joins.
    if (startPendingArchiveMove(ok))
        return;
    const std::optional<unsigned> transferDestination = transferDestinationPanel_;
    transferDestinationPanel_.reset();
    QString resultMessage = message;
    if (archiveMoveStage_ == ArchiveMoveStage::Removing)
        resultMessage =
            ok ? QtLocalText("Items moved out of archive.",
                             QStringLiteral("Объекты перемещены из архива."))
               : QtLocalText("Files extracted but not deleted from archive. ",
                             QStringLiteral("Файлы извлечены, но не удалены из архива. ")) +
                     message;
    if (archiveMoveStage_ != ArchiveMoveStage::None) {
        archiveMoveStage_ = ArchiveMoveStage::None;
        archiveMovePaths_.clear();
    }
    if (!ok)
        archivePassword_.clear();
    const bool hadProgress = progress_;
    const bool canceled = result == E_ABORT;
    if (hadProgress && !ok && !canceled)
        progress_->Sync.FinalMessage.ErrorMessage.Message = ToUString(resultMessage);
    closeProgressDialog(true);
    const bool finishedArchiveState = finishArchiveState(ok);
    if (transferDestination)
        refreshTransferDestination(*transferDestination);
    appPresentation_->CopyFinished();
    if (finishedArchiveState)
        return;
    statusBar()->showMessage(resultMessage);
    if (!ok && !hadProgress && !messageDisplayed && !canceled)
        QMessageBox::warning(this, "Sevenzip", resultMessage);
}

bool MainWindow::startPendingArchiveMove(bool ok) {
    if (archiveMoveStage_ == ArchiveMoveStage::Extracting && ok) {
        archiveMoveStage_ = ArchiveMoveStage::Removing;
        updatingArchive_ = true;
        closeProgressDialog();
        begin(
            QtLocalText("Deleting moved entries", QStringLiteral("Удаление перемещённых записей")),
            archive_);
        backend_->remove(archive_, archiveMovePaths_);
        return true;
    }
    return false;
}

bool MainWindow::finishArchiveState(bool ok) {
    if (state_ == State::Loading && !ok) {
        showDirectory(returnDirectory_, false);
    } else if (state_ == State::Operating) {
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

void MainWindow::closeProgressDialog(bool finished) {
    if (!progress_)
        return;
    CProgressDialog *completed = progress_;
    progress_ = nullptr;
    completed->AttachTransport(nullptr);
    // Source progress is destroyed before OnCopy refreshes the title. Qt keeps
    // the completed dialog through queued close/error presentation; it must no
    // longer overwrite the application owner's restored title on destruction.
    completed->MainWindow = nullptr;
    progressTransport_.reset();
    if (finished) {
        completed->setObjectName("completedProgressDialog");
        connect(completed, &QDialog::finished, completed, &QObject::deleteLater);
        completed->ProcessWasFinished();
        // ArchiveBackend and Qt workers finish on the GUI event loop. Deliver
        // the source close/final-message transition before commands become
        // available for the next operation.
        QCoreApplication::sendPostedEvents(completed, QEvent::MetaCall);
    } else {
        // QDialog::close routes through the source reject/cancel confirmation.
        // An already completed stage must be dismissed without a new prompt.
        completed->done(0);
        completed->deleteLater();
    }
}

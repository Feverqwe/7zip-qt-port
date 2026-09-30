#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "mainwindow.h"

void MainWindow::startFileDelete(const QStringList &paths) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ != State::Filesystem || fileDelete_ || paths.isEmpty())
        return;
    auto *operation = new FileDeleteOperation(directory_, paths, this);
    fileDelete_ = operation;
    beginWorkerProgress(QStringLiteral("Удаление файлов"), directory_,
                        [operation] { operation->cancel(); });
    progress_->Sync.Set_FilesProgressMode(true);
    progress_->Sync.Set_NumFilesTotal(quint64(paths.size()));
    connect(operation, &FileDeleteOperation::currentFile, this, [this](const QString &path) {
        if (progress_)
            progress_->Sync.Set_FilePath(path.toStdWString().c_str());
    });
    connect(operation, &FileDeleteOperation::advanced, this, [this](int completed, int total) {
        if (progress_) {
            progress_->Sync.Set_NumFilesTotal(quint64(total));
            progress_->Sync.Set_NumFilesCur(quint64(completed));
        }
    });
    connect(operation, &FileDeleteOperation::result, this,
            [this](bool success, const QString &message) {
                if (app_.QtOperations().closed())
                    return;
                if (!success && message != QStringLiteral("Операция отменена.") && progress_)
                    progress_->Sync.FinalMessage.ErrorMessage.Message = ToUString(message);
                closeProgressDialog(true);
                if (success)
                    statusBar()->showMessage(message);
            });
    connect(operation, &QThread::finished, this, [this, operation] {
        if (app_.QtOperations().closed())
            return;
        operation->deleteLater();
        if (fileDelete_ == operation)
            fileDelete_ = nullptr;
        // Internal deletion, like upstream, can have completed earlier items
        // before failure/cancel. Refresh on every outcome.
        showDirectory(directory_, false);
    });
    setState(State::Operating);
    operation->start();
}

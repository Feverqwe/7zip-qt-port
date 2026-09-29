#include "mainwindow.h"

void MainWindow::startFileDelete(const QStringList &paths) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ != State::Filesystem || fileDelete_ || paths.isEmpty())
        return;
    auto *operation = new FileDeleteOperation(directory_, paths, this);
    fileDelete_ = operation;
    auto *progress = new QProgressDialog("Удаление файлов…", "Отмена", 0, int(paths.size()), this);
    progress->setObjectName("fileDeleteProgress");
    progress->setWindowModality(Qt::NonModal);
    progress->setMinimumDuration(0);
    // Reaching the top-level count precedes the worker's result/finished.
    progress->setAutoClose(false);
    progress->setAutoReset(false);
    connect(progress, &QProgressDialog::canceled, operation, &FileDeleteOperation::cancel);
    connect(
        operation, &FileDeleteOperation::currentFile, progress,
        [progress](const QString &path) { progress->setLabelText(QFileInfo(path).fileName()); });
    connect(operation, &FileDeleteOperation::advanced, progress,
            [progress](int completed, int total) {
                progress->setMaximum(total);
                progress->setValue(completed);
            });
    connect(operation, &FileDeleteOperation::result, this,
            [this, progress](bool success, const QString &message) {
                if (app_.QtOperations().closed())
                    return;
                progress->close();
                if (success)
                    statusBar()->showMessage(message);
                else if (message != "Операция отменена.")
                    QMessageBox::warning(this, "Sevenzip Desktop", message);
            });
    connect(operation, &QThread::finished, this, [this, operation, progress] {
        if (app_.QtOperations().closed())
            return;
        progress->deleteLater();
        operation->deleteLater();
        if (fileDelete_ == operation)
            fileDelete_ = nullptr;
        // Internal deletion, like upstream, can have completed earlier items
        // before failure/cancel. Refresh on every outcome.
        showDirectory(directory_, false);
    });
    setState(State::Operating);
    progress->show();
    operation->start();
}

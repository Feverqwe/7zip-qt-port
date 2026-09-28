#include "CPP/7zip/UI/FileManager/PanelSplitFile.h"
#include "mainwindow.h"

MainWindow::~MainWindow() {
    // QComboBox can emit activated from editingFinished while QWidget tears
    // down focus. Navigation must stop before MainWindow members are destroyed.
    disconnect(pathCombo_, nullptr, this, nullptr);
    disconnect(path_, nullptr, this, nullptr);
    if (fileDelete_) {
        fileDelete_->cancel();
        fileDelete_->wait();
    }
    if (fileTransfer_) {
        fileTransfer_->cancel();
        fileTransfer_->wait();
    }
    if (volumeOperation_) {
        volumeOperation_->cancel();
        volumeOperation_->wait();
    }
}

void MainWindow::startVolume(VolumeOperation::Mode mode) {
    if (state_ != State::Filesystem || volumeOperation_ ||
        !(mode == VolumeOperation::Mode::Split ? split_ : combine_)->isEnabled())
        return;
    const QStringList paths = selectedFilesystemPaths();
    if (paths.size() != 1)
        return;
    const QFileInfo source(paths.first());
    if (!source.isFile() || source.isSymLink())
        return;
    VolumeParts parts;
    if (mode == VolumeOperation::Mode::Combine) {
        QString error;
        parts = VolumeOperation::inspectParts(paths.first(), &error);
        if (parts.paths.isEmpty()) {
            QMessageBox::warning(this, "Sevenzip Desktop", error);
            return;
        }
    }
    VolumeDialog dialog(mode, paths.first(), this, parts);
    if (dialog.exec() != QDialog::Accepted)
        return;
    CVolSeqName sequence;
    if (mode == VolumeOperation::Mode::Combine && !sequence.ParseName(source.fileName()))
        return;
    const QString destination =
        mode == VolumeOperation::Mode::Split
            ? dialog.destination()
            : QDir(dialog.destination()).filePath(sequence.GetCombinedName());
    volumeOperation_ =
        new VolumeOperation(mode, paths.first(), destination, dialog.volumeSizes(), this);
    volumeProgress_ = new QProgressDialog(
        mode == VolumeOperation::Mode::Split ? "Разделение файла…" : "Объединение файлов…",
        "Отмена", 0, 0, this);
    volumeProgress_->setObjectName("volumeProgress");
    volumeProgress_->setWindowModality(Qt::NonModal);
    volumeProgress_->setMinimumDuration(0);
    connect(volumeProgress_, &QProgressDialog::canceled, volumeOperation_,
            &VolumeOperation::cancel);
    connect(volumeOperation_, &VolumeOperation::result, this,
            [this](bool success, const QString &message) {
                if (volumeProgress_) {
                    volumeProgress_->close();
                    volumeProgress_->deleteLater();
                    volumeProgress_ = nullptr;
                }
                if (success) {
                    showDirectory(directory_, false);
                    statusBar()->showMessage(message);
                } else {
                    setState(State::Filesystem);
                    if (message != "Операция отменена.")
                        QMessageBox::warning(this, "Sevenzip Desktop", message);
                }
            });
    auto *operation = volumeOperation_;
    connect(operation, &QThread::finished, this, [this, operation] {
        operation->deleteLater();
        if (volumeOperation_ == operation)
            volumeOperation_ = nullptr;
    });
    setState(State::Operating);
    volumeProgress_->show();
    volumeOperation_->start();
}

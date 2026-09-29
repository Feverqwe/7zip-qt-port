#include "CPP/7zip/UI/FileManager/PanelSplitFile.h"
#include "CPP/7zip/UI/FileManager/SplitDialog.h"
#include "CPP/7zip/UI/FileManager/SplitUtils.h"
#include "mainwindow.h"
#include "qtassociatedfileservices.h"

MainWindow::~MainWindow() {
    removeEventFilter(this);
    // QComboBox can emit activated from editingFinished while QWidget tears
    // down focus. Navigation must stop before MainWindow members are destroyed.
    disconnect(pathCombo_, nullptr, this, nullptr);
    disconnect(path_, nullptr, this, nullptr);
    // Quiesce callbacks and process writes while temporary paths and folder
    // members are still alive; QObject child destruction happens afterward.
    disconnect(backend_, nullptr, this, nullptr);
    disconnect(filesystemScanner_, nullptr, this, nullptr);
    filesystemScanner_->cancel();
    backend_->cancelAndWait();
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
    QFileInfo source(paths.first());
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
    QString path = source.absolutePath();
    const QString itemRelPath = QDir(directory_).relativeFilePath(source.absoluteFilePath());
    QList<qint64> sizes;
    if (mode == VolumeOperation::Mode::Split) {
        CSplitDialog dialog(itemRelPath, path, this);
        if (dialog.exec() != QDialog::Accepted)
            return;
        source.refresh();
        if (!source.isFile() || source.isSymLink()) {
            QMessageBox::warning(this, "Разбить файл", "Исходный файл недоступен.");
            return;
        }
        if (quint64(source.size()) <= dialog.VolumeSizes.FrontItem()) {
            QMessageBox::warning(this, "Разбить файл", "Размер тома должен быть меньше файла.");
            return;
        }
        std::vector<UInt64> volumeSizes;
        for (unsigned i = 0; i < dialog.VolumeSizes.Size(); ++i) {
            const UInt64 size = dialog.VolumeSizes[i];
            sizes.append(qint64(size));
            volumeSizes.push_back(size);
        }
        const UInt64 numVolumes = GetNumberOfVolumes(quint64(source.size()), volumeSizes);
        if (numVolumes >= 100 &&
            QMessageBox::question(
                this, "Разбить файл", QStringLiteral("Создать %1 частей?").arg(numVolumes),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel) != QMessageBox::Yes)
            return;
        path = dialog.Path;
    } else {
        CThreadCombine combiner;
        combiner.InputDirPrefix = source.absolutePath() + '/';
        combiner.TotalSize = quint64(parts.totalSize);
        for (const QString &part : parts.paths)
            combiner.Names << QFileInfo(part).fileName();
        CCopyDialog dialog("Объединить файлы " + itemRelPath, "Объединить в:", path,
                           combiner.GetInfo(), this);
        dialog.setObjectName("combineDialog");
        if (dialog.exec() != QDialog::Accepted)
            return;
        path = dialog.Value;
    }
    path = QDir::fromNativeSeparators(path);
    if (!path.isEmpty() && !path.endsWith('/'))
        path += '/';
    // Upstream creates a missing destination only after the dialog and the
    // large-part-count confirmation. Keep the adapter's symlink prohibition.
    if (QFileInfo(QDir::cleanPath(path)).isSymLink() || !QDir().mkpath(path)) {
        QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось создать папку назначения.");
        return;
    }
    CVolSeqName sequence;
    if (mode == VolumeOperation::Mode::Combine && !sequence.ParseName(source.fileName()))
        return;
    const QString destination = mode == VolumeOperation::Mode::Split
                                    ? path
                                    : QDir(path).filePath(sequence.GetCombinedName());
    volumeOperation_ = new VolumeOperation(mode, paths.first(), destination, sizes, this);
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

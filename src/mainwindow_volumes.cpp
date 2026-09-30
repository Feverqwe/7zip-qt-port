#include "CPP/7zip/UI/FileManager/PanelSplitFile.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/SplitDialog.h"
#include "CPP/7zip/UI/FileManager/SplitUtils.h"
#include "mainwindow.h"
#include "qtapppresentation.h"
#include "qtassociatedfileservices.h"
#include "qtfilemanagercommands.h"
#include <QCloseEvent>

MainWindow::~MainWindow() {
    removeEventFilter(this);
    // QComboBox can emit activated from editingFinished while QWidget tears
    // down focus. Navigation must stop before MainWindow members are destroyed.
    QCloseEvent close;
    closeEvent(&close);
    if (!app_.QtOperations().closed()) {
        // Destruction cannot be vetoed; retain the same input lifetime even
        // when a modal child prevented an ordinary user close.
        app_.QtOperations().Close();
        for (auto &panel : app_.Panels)
            panel.Disable_Processing_Timer_Notify_StatusBar();
        quiesceOperations();
        app_.Save();
        app_.ReleaseApp();
    }
}

void MainWindow::startVolume(VolumeOperation::Mode mode) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
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
    beginWorkerProgress(mode == VolumeOperation::Mode::Split ? QStringLiteral("Разделение файла")
                                                             : QStringLiteral("Объединение файлов"),
                        paths.first(), [operation = volumeOperation_] { operation->cancel(); });
    progress_->Sync.Set_NumBytesTotal(
        quint64(mode == VolumeOperation::Mode::Split ? source.size() : parts.totalSize));
    connect(volumeOperation_, &VolumeOperation::advanced, this, [this](qint64 bytes) {
        if (progress_)
            progress_->Sync.Set_NumBytesCur(quint64(qMax<qint64>(0, bytes)));
    });
    connect(volumeOperation_, &VolumeOperation::result, this,
            [this](bool success, const QString &message) {
                if (app_.QtOperations().closed())
                    return;
                if (!success && message != QStringLiteral("Операция отменена.") && progress_)
                    progress_->Sync.FinalMessage.ErrorMessage.Message = ToUString(message);
                closeProgressDialog(true);
                if (success) {
                    showDirectory(directory_, false);
                    statusBar()->showMessage(message);
                } else {
                    setState(State::Filesystem);
                }
            });
    auto *operation = volumeOperation_;
    connect(operation, &QThread::finished, this, [this, operation] {
        if (app_.QtOperations().closed())
            return;
        operation->deleteLater();
        if (volumeOperation_ == operation)
            volumeOperation_ = nullptr;
    });
    setState(State::Operating);
    volumeOperation_->start();
}

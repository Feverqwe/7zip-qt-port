#include "7zip/UI/FileManager/SplitDialogRes.h"
#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/PanelSplitFile.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/SplitDialog.h"
#include "CPP/7zip/UI/FileManager/SplitUtils.h"
#include "mainwindow.h"
#include "qtapppresentation.h"
#include "qtassociatedfileservices.h"
#include "qtfilemanagercommands.h"
#include "qtfilemanagerdrag.h"
#include "qtlangcontrols.h"
#include "qtprogramfilemenu.h"
#include "qttrashservice.h"
#include <QCloseEvent>

MainWindow::~MainWindow() {
    if (drag_)
        drag_->Revoke();
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
    // QWidget destroys its children after CApp's member lifetime ends. Stop
    // callback text before that teardown and clear published folder identities.
    for (auto &ui : panelUi_)
        if (auto *table = dynamic_cast<FileTable *>(ui.table)) {
            table->sourcePanel = nullptr;
            table->textFolder = nullptr;
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
            QMessageBox::warning(this, QtLangText(IDD_SPLIT),
                                 QtLocalText("Source file is unavailable.",
                                             QStringLiteral("Исходный файл недоступен.")));
            return;
        }
        if (quint64(source.size()) <= dialog.VolumeSizes.FrontItem()) {
            QMessageBox::warning(this, QtLangText(IDD_SPLIT),
                                 QtLangText(IDS_SPLIT_VOL_MUST_BE_SMALLER));
            return;
        }
        CRecordVector<UInt64> volumeSizes;
        for (unsigned i = 0; i < dialog.VolumeSizes.Size(); ++i) {
            const UInt64 size = dialog.VolumeSizes[i];
            sizes.append(qint64(size));
            volumeSizes.Add(size);
        }
        const UInt64 numVolumes = GetNumberOfVolumes(quint64(source.size()), volumeSizes);
        if (numVolumes >= 100 &&
            QMessageBox::question(this, QtLangText(IDS_SPLIT_CONFIRM_TITLE),
                                  QString::fromWCharArray(MyFormatNew(IDS_SPLIT_CONFIRM_MESSAGE,
                                                                      NumberToString(numVolumes))),
                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel) !=
                QMessageBox::Yes)
            return;
        path = dialog.Path;
    } else {
        CThreadCombine combiner;
        combiner.InputDirPrefix = source.absolutePath() + '/';
        combiner.TotalSize = quint64(parts.totalSize);
        for (const QString &part : parts.paths)
            combiner.Names << QFileInfo(part).fileName();
        CCopyDialog dialog(QtLangText(IDS_COMBINE) + ' ' + itemRelPath, QtLangText(IDS_COMBINE_TO),
                           path, combiner.GetInfo(), this);
        dialog.BindLanguage(IDS_COMBINE, IDS_COMBINE_TO, ' ' + itemRelPath);
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
        QMessageBox::warning(this, "Sevenzip Desktop",
                             QtLocalText("Could not create destination folder.",
                                         QStringLiteral("Не удалось создать папку назначения.")));
        return;
    }
    CVolSeqName sequence;
    if (mode == VolumeOperation::Mode::Combine && !sequence.ParseName(source.fileName()))
        return;
    const QString destination = mode == VolumeOperation::Mode::Split
                                    ? path
                                    : QDir(path).filePath(sequence.GetCombinedName());
    volumeOperation_ = new VolumeOperation(mode, paths.first(), destination, sizes, this);
    beginWorkerProgress(
        QtLangText(mode == VolumeOperation::Mode::Split ? IDS_SPLITTING : IDS_COMBINING),
        paths.first(), [operation = volumeOperation_] { operation->cancel(); });
    progress_->Sync.Set_NumBytesTotal(
        quint64(mode == VolumeOperation::Mode::Split ? source.size() : parts.totalSize));
    // Source CThreadSplit/Combine update this mutex snapshot directly. Close
    // joins the worker before releasing the dialog; its timer owns presentation.
    CProgressSync *sync = &progress_->Sync;
    connect(
        volumeOperation_, &VolumeOperation::advanced, this,
        [sync](qint64 bytes) { sync->Set_NumBytesCur(quint64(qMax<qint64>(0, bytes))); },
        Qt::DirectConnection);
    connect(volumeOperation_, &VolumeOperation::result, this,
            [this, operation = volumeOperation_](bool success, const QString &message) {
                if (app_.QtOperations().closed())
                    return;
                operation->wait();
                if (!success && !operation->isCanceled() && progress_)
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

#include "7zip/UI/FileManager/resource.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "7zip/UI/GUI/ExtractRes.h"
#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/OverwriteDialog.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "mainwindow.h"
#include "qtapppresentation.h"
#include "qtlangcontrols.h"
#include "qtlocationfolder.h"
#include "qttrashservice.h"
#include <QFile>
#include <QScopedValueRollback>

namespace {

NOverwriteDialog::CFileInfo overwriteFileInfo(const QString &path) {
    const QFileInfo source(path);
    NOverwriteDialog::CFileInfo info;
    info.Path = ToUString(path);
    info.Is_FileSystemFile = true;
    info.Is_Directory = source.isDir();
    if (source.isFile())
        info.SetSize(UInt64(source.size()));
    const qint64 ticks = (source.lastModified().toMSecsSinceEpoch() + 11644473600000LL) * 10000;
    if (ticks > 0) {
        FILETIME time{};
        time.dwLowDateTime = DWORD(quint64(ticks));
        time.dwHighDateTime = DWORD(quint64(ticks) >> 32);
        info.SetTime(time);
    }
    return info;
}
} // namespace

void MainWindow::transferFiles(const QStringList &sources, const QString &destination, bool move,
                               bool directPath, bool refreshSource) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    const QString destinationDirectory =
        directPath ? QFileInfo(destination).absolutePath() : destination;
    if (state_ != State::Filesystem || !QFileInfo(destinationDirectory).isDir() ||
        QFileInfo(destinationDirectory).isSymLink() || sources.isEmpty() ||
        (directPath && sources.size() != 1)) {
        QMessageBox::warning(
            this, "Sevenzip Desktop",
            QtLocalText("The destination folder is unavailable.", "Папка назначения недоступна."));
        return;
    }
    fileTransfer_ =
        new FileTransferOperation(sources, destination, move, this, {},
                                  directPath ? FileTransferOperation::DestinationKind::DirectPath
                                             : FileTransferOperation::DestinationKind::Directory);
    auto *operation = fileTransfer_;
    connect(operation, &FileTransferOperation::overwriteRequested, this,
            [this, operation](const QString &existing, const QString &incoming) {
                if (app_.QtOperations().closed() || operation->isCanceled()) {
                    operation->submitOverwriteAnswer(NOverwriteAnswer::kCancel);
                    return;
                }
                COverwriteDialog dialog(this);
                overwriteDialog_ = &dialog;
                dialog.OldFileInfo = overwriteFileInfo(existing);
                dialog.NewFileInfo = overwriteFileInfo(incoming);
                connect(operation, &FileTransferOperation::dismissOverwriteRequested, &dialog,
                        &QDialog::reject, Qt::QueuedConnection);
                if (operation->isCanceled()) {
                    operation->submitOverwriteAnswer(NOverwriteAnswer::kCancel);
                    return;
                }
                const int answer = NOverwriteDialog::MapButtonResult(dialog.Create(this));
                overwriteDialog_ = nullptr;
                operation->submitOverwriteAnswer(answer);
            });
    beginWorkerProgress(QtLangText(move ? IDS_MOVING : IDS_COPYING), destination,
                        [operation = fileTransfer_] { operation->cancel(); });
    // The source mutex snapshot is worker-safe. Completion or close joins the
    // worker before releasing this dialog; no QWidget/MainWindow state is read.
    CProgressSync *sync = &progress_->Sync;
    connect(
        fileTransfer_, &FileTransferOperation::currentFile, this,
        [sync](const QString &path) { sync->Set_FilePath(path.toStdWString().c_str()); },
        Qt::DirectConnection);
    connect(
        fileTransfer_, &FileTransferOperation::advanced, this,
        [sync, completed = UInt64(0)](qint64 bytes) mutable {
            completed += quint64(qMax<qint64>(0, bytes));
            sync->Set_NumBytesCur(completed);
        },
        Qt::DirectConnection);
    connect(fileTransfer_, &FileTransferOperation::result, this,
            [this, operation](bool success, const QString &message) {
                if (app_.QtOperations().closed())
                    return;
                operation->wait();
                if (!success &&
                    operation->resultKind() != FileTransferOperation::ResultKind::Canceled &&
                    progress_)
                    progress_->Sync.FinalMessage.ErrorMessage.Message = ToUString(message);
                closeProgressDialog(true);
                if (success)
                    statusBar()->showMessage(message);
            });
    const unsigned sourcePanel = app_.GetFocusedPanelIndex();
    const unsigned destinationPanel = 1 - sourcePanel;
    const bool refreshDestination = app_.NumPanels == 2 &&
                                    panelUi_[destinationPanel].state == State::Filesystem &&
                                    QDir::cleanPath(panelUi_[destinationPanel].directory) ==
                                        QDir::cleanPath(destinationDirectory);
    connect(operation, &QThread::finished, this,
            [this, operation, sources, sourcePanel, destinationPanel, refreshDestination,
             refreshSource] {
                if (app_.QtOperations().closed())
                    return;
                operation->deleteLater();
                if (fileTransfer_ == operation)
                    fileTransfer_ = nullptr;
                if (refreshSource) {
                    showDirectory(directory_, false, true);
                    selectPaths(sources);
                } else
                    setState(State::Filesystem);
                if (refreshDestination && app_.NumPanels == 2 &&
                    app_.GetFocusedPanelIndex() == sourcePanel) {
                    activatePanelUi(destinationPanel);
                    app_.Panels[destinationPanel].OnReload();
                    appPresentation_->CopyDestinationRefreshed(destinationPanel);
                    activatePanelUi(sourcePanel);
                }
                if (app_.GetFocusedPanelIndex() == sourcePanel)
                    app_.Panels[sourcePanel].SetFocusToList();
                appPresentation_->CopyFinished();
            });
    setState(State::Operating);
    fileTransfer_->start();
}

void MainWindow::executeCopyDestination(bool move, bool copyToSame, unsigned sourceIndex,
                                        const QString &destination,
                                        const CRecordVector<UInt32> &indices, bool useDestPanel,
                                        bool useTemp) {
    const bool streams = app_.GetFocusedPanel().IsAltStreamsFolder();
    const bool devices =
        app_.GetFocusedPanel().IsFSDrivesFolder() && app_.GetFocusedPanel().IsDeviceDrivesPrefix();
    if (sourceIndex != app_.GetFocusedPanelIndex() || indices.IsEmpty() ||
        (state_ != State::Viewing && state_ != State::Filesystem && !streams && !devices))
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (useDestPanel && !useTemp && transferToArchivePanel(destination, move, indices))
        return;
    if (streams || devices) {
        CCopyToOptions options;
        options.folder = ToUString(destination);
        options.moveMode = move;
        UStringVector messages;
        bool usePassword = false;
        UString password;
        const HRESULT result =
            app_.Panels[sourceIndex].CopyTo(options, indices, &messages, usePassword, password);
        renderBoundFolder();
        if (useDestPanel && app_.NumPanels == 2) {
            activatePanelUi(1 - sourceIndex);
            app_.Panels[1 - sourceIndex].OnReload();
            activatePanelUi(sourceIndex);
        }
        if (result != S_OK && result != E_ABORT)
            QMessageBox::warning(this, QtLangText(move ? IDS_MOVING : IDS_COPYING),
                                 QString::fromWCharArray(HResultToMessage(result).Ptr()));
        return;
    }
    const bool directPath = indices.Size() == 1 && !destination.endsWith('/') &&
                            !QFileInfo(destination).isDir() && !useDestPanel;
    if (state_ == State::Viewing) {
        if (useDestPanel && useTemp && transferBetweenArchivePanels(destination, move, indices))
            return;
        const QStringList paths = archivePathsForIndices(indices);
        if (paths.isEmpty())
            return;
        ExtractionOptions options;
        options.pathMode = ExtractionOptions::PathMode::CurrentPaths;
        options.currentFolder = archiveFolder_;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        rememberTransferDestination(destination);
        if (copyToSame) {
            if (move) {
                archiveMovePaths_ = paths;
                archiveMoveStage_ = ArchiveMoveStage::Extracting;
            }
            const bool handled =
                extractWithSource(destination, true, archivePassword_, options, paths, &indices);
            if (!handled || state_ != State::Operating) {
                archiveMoveStage_ = ArchiveMoveStage::None;
                archiveMovePaths_.clear();
                transferDestinationPanel_.reset();
                if (!handled)
                    QMessageBox::warning(
                        this, QtLangText(IDS_COPYING),
                        QtLocalText("Source extraction is unavailable for this archive.",
                                    "Исходное извлечение для этого архива недоступно."));
            }
            return;
        }
        if (!move) {
            if (!extractWithSource(destination, true, archivePassword_, options, paths, &indices)) {
                previousState_ = state_;
                setState(State::Operating);
                begin(QtLangText(IDS_PROGRESS_EXTRACTING), destination);
                backend_->extract(archive_, destination, paths, archivePassword_, options);
            }
            if (state_ != State::Operating)
                transferDestinationPanel_.reset();
            return;
        }
        archiveMovePaths_ = paths;
        archiveMoveStage_ = ArchiveMoveStage::Extracting;
        if (extractWithSource(destination, true, archivePassword_, options, paths, &indices)) {
            if (state_ != State::Operating) {
                archiveMoveStage_ = ArchiveMoveStage::None;
                archiveMovePaths_.clear();
                transferDestinationPanel_.reset();
            }
            return;
        }
        if (!QFileInfo(destination).isDir() || QFileInfo(destination).isSymLink()) {
            archiveMoveStage_ = ArchiveMoveStage::None;
            archiveMovePaths_.clear();
            transferDestinationPanel_.reset();
            QMessageBox::warning(this, "Sevenzip Desktop",
                                 QtLocalText("The destination folder is unavailable.",
                                             "Папка назначения недоступна."));
            return;
        }
        previousState_ = state_;
        setState(State::Operating);
        begin(QtLangText(IDS_MOVING), destination);
        backend_->extract(archive_, destination, paths, archivePassword_);
        return;
    }
    QStringList sources;
    CPanel &source = app_.Panels[sourceIndex];
    source.LoadFullPath();
    UStringVector filePaths;
    source.GetFilePaths(indices, filePaths);
    FOR_VECTOR(i, filePaths)
    sources << QtFilesystemPath(us2fs(filePaths[i]));
    if (sources.isEmpty())
        return;
    transferFiles(sources, destination, move, directPath,
                  move || copyToSame || app_.NumPanels <= 1);
}

bool MainWindow::transferToArchivePanel(const QString &destination, bool move,
                                        const CRecordVector<UInt32> &indices) {
    if (app_.NumPanels != 2 || !app_.GetFocusedPanel().Is_IO_FS_Folder())
        return false;
    const unsigned sourceIndex = app_.GetFocusedPanelIndex();
    const unsigned destinationIndex = 1 - sourceIndex;
    CPanel &source = app_.Panels[sourceIndex];
    CPanel &target = app_.Panels[destinationIndex];
    if (panelUi_[destinationIndex].state != State::Viewing ||
        QDir::cleanPath(destination) != QDir::cleanPath(ToQString(target.GetFsPath())))
        return false;
    auto *folder = dynamic_cast<CAgentFolder *>(target.GetFolder());
    if (!folder || target.GetParentFolders().Size() != 1 || target.IsThereReadOnlyFolder()) {
        QMessageBox::warning(this, QtLangText(IDS_COPYING),
                             QtLocalText("Writing to this archive folder is unavailable.",
                                         "Запись в эту папку архива недоступна."));
        return true;
    }
    UStringVector names;
    FOR_VECTOR(i, indices) names.Add(source.GetItemRelPath2(indices[i]));
    if (names.IsEmpty())
        return true;
    CSelectedState destinationSelection;
    target.SaveSelectedState(destinationSelection);
    UStringVector messages;
    HRESULT result;
    {
        QScopedValueRollback<bool> progressOwner(sourceArchiveProgress_, true);
        result = target.CopyFrom(move, source.GetFsPath(), names, true, &messages);
    }
    // The native call has completed. Release its retained source guards before
    // changing the panel projection; source RefreshListCtrl can then own a scan.
    if (app_.QtOperations().kind() == QtAppOperationState::Kind::Operation)
        app_.QtOperations().End();
    if (result == S_OK) {
        activatePanelUi(destinationIndex);
        // CopyFrom already reopens the native archive and rebuilds its proxy.
        // OnCopy reloads that provider with state saved before the update.
        target.RefreshListCtrl(destinationSelection);
        appPresentation_->CopyDestinationRefreshed(destinationIndex);
        activatePanelUi(sourceIndex);
        if (move)
            source.OnReload();
        statusBar()->showMessage(
            move ? QtLocalText("Items moved into the archive.", "Объекты перемещены в архив.")
                 : QtLocalText("Items copied into the archive.", "Объекты скопированы в архив."));
    } else if (result != E_ABORT) {
        QMessageBox::warning(this, QtLangText(IDS_COPYING),
                             QString::fromWCharArray(HResultToMessage(result).Ptr()));
    }
    return true;
}

void MainWindow::deleteSelected(bool toRecycleBin) {
    if (state_ == State::Viewing) {
        if (!remove_->isEnabled())
            return;
        CSelectedState selection;
        const HRESULT result =
            app_.GetFocusedPanel().DeleteItems(toRecycleBin, *trashService_, selection);
        if (result == S_FALSE || result == E_ABORT)
            return;
        renderBoundFolder();
        restoreListSelection(selection);
        if (result != S_OK)
            QMessageBox::warning(this, QStringLiteral("7-Zip"),
                                 QtLocalText("Could not delete items from the archive.",
                                             "Не удалось удалить объекты из архива."));
        return;
    }
    if (state_ != State::Filesystem && !app_.GetFocusedPanel().IsAltStreamsFolder())
        return;
    CPanel &panel = app_.GetFocusedPanel();
    CSelectedState selection;
    const HRESULT result = panel.DeleteItems(toRecycleBin, *trashService_, selection);
    if (result == S_FALSE)
        return;
    refreshCurrent(&selection);
    if (result != S_OK && result != E_ABORT)
        QMessageBox::warning(this, QStringLiteral("7-Zip"),
                             QtLocalText("Could not delete all selected items.",
                                         "Не удалось удалить все выбранные объекты."));
}

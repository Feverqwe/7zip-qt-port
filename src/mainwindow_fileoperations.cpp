#include "7zip/UI/FileManager/resource.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/OverwriteDialog.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "mainwindow.h"
#include "qtlangcontrols.h"
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
                               bool directPath) {
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
    fileTransferCompletedBytes_ = 0;
    beginWorkerProgress(QtLangText(move ? IDS_MOVING : IDS_COPYING), destination,
                        [operation = fileTransfer_] { operation->cancel(); });
    connect(fileTransfer_, &FileTransferOperation::currentFile, this, [this](const QString &path) {
        if (app_.QtOperations().closed())
            return;
        if (progress_)
            progress_->Sync.Set_FilePath(path.toStdWString().c_str());
    });
    connect(fileTransfer_, &FileTransferOperation::advanced, this, [this](qint64 bytes) {
        fileTransferCompletedBytes_ += quint64(qMax<qint64>(0, bytes));
        if (progress_)
            progress_->Sync.Set_NumBytesCur(fileTransferCompletedBytes_);
    });
    connect(fileTransfer_, &FileTransferOperation::result, this,
            [this, operation](bool success, const QString &message) {
                if (app_.QtOperations().closed())
                    return;
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
            [this, operation, sources, sourcePanel, destinationPanel, refreshDestination] {
                if (app_.QtOperations().closed())
                    return;
                operation->deleteLater();
                if (fileTransfer_ == operation)
                    fileTransfer_ = nullptr;
                showDirectory(directory_, false);
                selectPaths(sources);
                if (refreshDestination && app_.NumPanels == 2 &&
                    app_.GetFocusedPanelIndex() == sourcePanel) {
                    activatePanelUi(destinationPanel);
                    refreshCurrent();
                    activatePanelUi(sourcePanel);
                }
                if (app_.GetFocusedPanelIndex() == sourcePanel)
                    app_.Panels[sourcePanel].SetFocusToList();
            });
    setState(State::Operating);
    fileTransfer_->start();
}

void MainWindow::executeCopyDestination(bool move, bool copyToSame, unsigned sourceIndex,
                                        const QString &destination,
                                        const CRecordVector<UInt32> &indices, bool useDestPanel,
                                        bool useTemp) {
    if (sourceIndex != app_.GetFocusedPanelIndex() || indices.IsEmpty() ||
        (state_ != State::Viewing && state_ != State::Filesystem))
        return;
    const bool directPath = indices.Size() == 1 && !destination.endsWith('/') &&
                            !QFileInfo(destination).isDir() && !useDestPanel;
    if (state_ == State::Viewing) {
        if (useDestPanel && useTemp && transferBetweenArchivePanels(destination, move))
            return;
        const QStringList paths = copyToSame ? archivePathsForIndices(indices)
                                  : move     ? selectedArchivePaths()
                                             : selectedArchiveItemsPaths();
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
            extractTo(destination, true, archivePassword_, options);
            if (state_ != State::Operating)
                transferDestinationPanel_.reset();
            return;
        }
        archiveMovePaths_ = paths;
        archiveMoveStage_ = ArchiveMoveStage::Extracting;
        if (extractWithSource(destination, true, archivePassword_, options, paths)) {
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
    const QStringList sources =
        copyToSame ? QStringList{ToQString(app_.Panels[sourceIndex].GetItemFullPath(indices[0]))}
                   : selectedFilesystemPaths();
    if (sources.isEmpty())
        return;
    if (useDestPanel && transferToArchivePanel(destination, move))
        return;
    transferFiles(sources, destination, move, directPath);
}

bool MainWindow::transferToArchivePanel(const QString &destination, bool move) {
    if (app_.NumPanels != 2 || state_ != State::Filesystem)
        return false;
    const unsigned sourceIndex = app_.GetFocusedPanelIndex();
    const unsigned destinationIndex = 1 - sourceIndex;
    CPanel &source = app_.Panels[sourceIndex];
    CPanel &target = app_.Panels[destinationIndex];
    if (panelUi_[destinationIndex].state != State::Viewing ||
        QDir::cleanPath(destination) != QDir::cleanPath(ToQString(target.GetFsPath())))
        return false;
    auto *folder = dynamic_cast<CAgentFolder *>(target.GetFolder());
    if (!folder || target.GetParentFolders().Size() != 1 ||
        !ArchiveFormats::canModify(panelUi_[destinationIndex].archive)) {
        QMessageBox::warning(this, QtLangText(IDS_COPYING),
                             QtLocalText("Writing to this archive folder is unavailable.",
                                         "Запись в эту папку архива недоступна."));
        return true;
    }
    CRecordVector<UInt32> indices = sourceOperatedIndices();
    UStringVector names;
    FOR_VECTOR(i, indices) names.Add(source.GetItemRelPath(indices[i]));
    if (names.IsEmpty())
        return true;
    UStringVector messages;
    HRESULT result;
    {
        QScopedValueRollback<bool> progressOwner(sourceArchiveProgress_, true);
        result = target.CopyFrom(move, source.GetFsPath(), names, true, &messages);
    }
    if (result == S_OK) {
        activatePanelUi(destinationIndex);
        refreshCurrent();
        activatePanelUi(sourceIndex);
        if (move)
            refreshCurrent();
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
        app_.GetFocusedPanel().RefreshListCtrl(selection);
        if (result != S_OK)
            QMessageBox::warning(this, QStringLiteral("7-Zip"),
                                 QtLocalText("Could not delete items from the archive.",
                                             "Не удалось удалить объекты из архива."));
        return;
    }
    if (state_ != State::Filesystem)
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

void MainWindow::renameSelected() {
    if (state_ != State::Filesystem && state_ != State::Viewing)
        return;
    const bool archive = state_ == State::Viewing;
    if (!rename_->isEnabled())
        return;
    const QStringList paths = archive ? selectedArchiveItemsPaths() : selectedFilesystemPaths();
    if (paths.size() != 1)
        return;
    if (!archive && QFileInfo(paths.first()).isSymLink())
        return;
    const auto indices = sourceOperatedIndices();
    if (indices.Size() != 1)
        return;
    int row = -1;
    for (int i = 0; i < table_->rowCount(); ++i)
        if (table_->item(i, 0) &&
            table_->item(i, 0)->data(Qt::UserRole + 6).toUInt() == indices[0]) {
            row = i;
            break;
        }
    if (row < 0)
        return;
    auto *item = table_->item(row, 0);
    if (!item || item->data(Qt::UserRole + 6).isNull())
        return;
    renameEditingItem_ = item;
    renameOriginalPath_ = paths.first();
    renameSourceIndex_ = item->data(Qt::UserRole + 6).toUInt();
    renameActivationPending_ = true;
    if (viewStack_->currentWidget() == iconList_) {
        const QModelIndex modelIndex = table_->model()->index(row, 0);
        iconList_->scrollTo(modelIndex);
        iconList_->edit(modelIndex);
        if (archive)
            if (auto *editor = iconList_->findChild<QLineEdit *>()) {
                editor->setText(paths.first().section('/', -1));
                editor->selectAll();
            }
    } else {
        table_->scrollToItem(item);
        table_->editItem(item);
        if (archive)
            if (auto *editor = table_->findChild<QLineEdit *>()) {
                editor->setText(paths.first().section('/', -1));
                editor->selectAll();
            }
    }
}

void MainWindow::finishFilesystemRename(quint32 sourceIndex, const QString &originalPath,
                                        const QString &name) {
    const bool archive = state_ == State::Viewing;
    if (state_ != State::Filesystem && !archive)
        return;
    if (!archive && QFileInfo(originalPath).absolutePath() != directory_ && !flatDisk_)
        return;
    if (archive && (name.isEmpty() || name == "." || name == ".." || name.contains('/') ||
                    name.contains('\\'))) {
        renderBoundFolder();
        selectPaths({originalPath});
        QMessageBox::warning(this, QStringLiteral("7-Zip"),
                             QtLocalText("Invalid file name.", "Недопустимое имя файла."));
        return;
    }
    const HRESULT result = app_.GetFocusedPanel().OnEndLabelEdit(sourceIndex, ToUString(name));
    if (result != S_OK && result != E_ABORT)
        QMessageBox::warning(
            this, "Sevenzip Desktop",
            QtLocalText("Could not rename the item.", "Не удалось переименовать объект."));
    if (archive) {
        renderBoundFolder();
        const QString prefix = originalPath.section('/', 0, -2);
        selectPaths(
            {result == S_OK ? (prefix.isEmpty() ? name : prefix + '/' + name) : originalPath});
        return;
    }
    showDirectory(directory_, false);
    if (result == S_OK)
        selectPaths({QDir(QFileInfo(originalPath).absolutePath()).filePath(name)});
    else
        selectPaths({originalPath});
}

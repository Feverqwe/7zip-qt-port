#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/OverwriteDialog.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "mainwindow.h"
#include "qttrashservice.h"

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

QString MainWindow::selectedCopyInfo() {
    CPanel &panel = app_.GetFocusedPanel();
    CSelectedState selection;
    // Synchronize Qt selection until the complete source OnNotify port is connected.
    panel.SaveSelectedState(selection);
    CRecordVector<UInt32> indices;
    panel.Get_ItemIndices_OperSmart(indices);
    panel.LoadFullPath();
    return panel.GetItemsInfoString(indices);
}
void MainWindow::transferFiles(const QStringList &sources, const QString &destination, bool move) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ != State::Filesystem || !QFileInfo(destination).isDir() ||
        QFileInfo(destination).isSymLink() || sources.isEmpty()) {
        QMessageBox::warning(this, "Sevenzip Desktop", "Папка назначения недоступна.");
        return;
    }
    fileTransfer_ = new FileTransferOperation(sources, destination, move, this);
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
    beginWorkerProgress(move ? QStringLiteral("Перемещение файлов")
                             : QStringLiteral("Копирование файлов"),
                        destination, [operation = fileTransfer_] { operation->cancel(); });
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
            [this](bool success, const QString &message) {
                if (app_.QtOperations().closed())
                    return;
                if (!success && message != QStringLiteral("Операция отменена.") && progress_)
                    progress_->Sync.FinalMessage.ErrorMessage.Message = ToUString(message);
                closeProgressDialog(true);
                if (success)
                    statusBar()->showMessage(message);
            });
    const unsigned sourcePanel = app_.GetFocusedPanelIndex();
    const unsigned destinationPanel = 1 - sourcePanel;
    const bool refreshDestination =
        app_.NumPanels == 2 && panelUi_[destinationPanel].state == State::Filesystem &&
        QDir::cleanPath(panelUi_[destinationPanel].directory) == QDir::cleanPath(destination);
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
            });
    setState(State::Operating);
    fileTransfer_->start();
}

void MainWindow::copySelected() {
    if (state_ == State::Viewing) {
        transferSelected(false);
        return;
    }
    if (state_ != State::Filesystem || selectedFilesystemPaths().isEmpty())
        return;
    transferSelected(false);
}

void MainWindow::moveSelected() {
    if (state_ == State::Viewing) {
        if (!move_->isEnabled())
            return;
        const QStringList paths = selectedArchivePaths();
        if (paths.isEmpty())
            return;
        QStringList history;
        ReadCopyHistory(history, toolbarSettingsPath());
        CopyDialog dialog(true, copyDefaultDestination(), paths, history, this, selectedCopyInfo());
        if (dialog.exec() != QDialog::Accepted)
            return;
        const QString destination = dialog.destination();
        AddUniqueStringToHeadOfList(history, destination);
        while (history.size() > 20)
            history.removeLast();
        SaveCopyHistory(history, toolbarSettingsPath());
        archiveMovePaths_ = paths;
        archiveMoveStage_ = ArchiveMoveStage::Extracting;
        rememberTransferDestination(destination);
        ExtractionOptions options;
        options.pathMode = ExtractionOptions::PathMode::CurrentPaths;
        options.currentFolder = archiveFolder_;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
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
            QMessageBox::warning(this, "Sevenzip Desktop", "Папка назначения недоступна.");
            return;
        }
        previousState_ = state_;
        setState(State::Operating);
        begin("Перемещение из архива", destination);
        backend_->extract(archive_, destination, paths, archivePassword_);
        return;
    }
    if (state_ != State::Filesystem || selectedFilesystemPaths().isEmpty())
        return;
    transferSelected(true);
}

void MainWindow::transferSelected(bool move) {
    if (state_ == State::Viewing && !move) {
        const QStringList sources = selectedArchiveItemsPaths();
        if (sources.isEmpty())
            return;
        QStringList history;
        ReadCopyHistory(history, toolbarSettingsPath());
        CopyDialog dialog(false, copyDefaultDestination(), sources, history, this,
                          selectedCopyInfo());
        if (dialog.exec() != QDialog::Accepted)
            return;
        const QString destination = dialog.destination();
        ExtractionOptions options;
        options.pathMode = ExtractionOptions::PathMode::CurrentPaths;
        options.currentFolder = archiveFolder_;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        rememberTransferDestination(destination);
        extractTo(destination, true, archivePassword_, options);
        if (state_ != State::Operating)
            transferDestinationPanel_.reset();
        AddUniqueStringToHeadOfList(history, destination);
        while (history.size() > 20)
            history.removeLast();
        SaveCopyHistory(history, toolbarSettingsPath());
        return;
    }
    const QStringList sources = selectedFilesystemPaths();
    if (state_ != State::Filesystem || sources.isEmpty())
        return;
    QStringList history;
    ReadCopyHistory(history, toolbarSettingsPath());
    CopyDialog dialog(move, copyDefaultDestination(), sources, history, this, selectedCopyInfo());
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString destination = dialog.destination();
    transferFiles(sources, destination, move);
    if (!QFileInfo(destination).isDir())
        return;
    AddUniqueStringToHeadOfList(history, destination);
    while (history.size() > 20)
        history.removeLast();
    SaveCopyHistory(history, toolbarSettingsPath());
}

void MainWindow::deleteSelected(bool toRecycleBin) {
    if (state_ == State::Viewing) {
        if (isNestedArchive() || !remove_->isEnabled())
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
                                 QStringLiteral("Не удалось удалить объекты из архива."));
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
                             QStringLiteral("Не удалось удалить все выбранные объекты."));
}

void MainWindow::renameSelected() {
    if (state_ != State::Filesystem && state_ != State::Viewing)
        return;
    const bool archive = state_ == State::Viewing;
    if (!rename_->isEnabled() || (archive && isNestedArchive()))
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
                             QStringLiteral("Недопустимое имя файла."));
        return;
    }
    const HRESULT result = app_.GetFocusedPanel().OnEndLabelEdit(sourceIndex, ToUString(name));
    if (result != S_OK && result != E_ABORT)
        QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось переименовать объект.");
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

#include "7zip/PropID.h"
#include "7zip/UI/FileManager/resource.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "CPP/7zip/UI/FileManager/BrowseDialog.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "CPP/7zip/UI/FileManager/SysIconUtils.h"
#include "Windows/PropVariant.h"
#include "mainwindow.h"
#include "qtarchivereopen.h"
#include "qtlangcontrols.h"
#include "qtlanguageservices.h"
#include "qtlocationfolder.h"
#include <QScopeGuard>
#include <QScopedValueRollback>
#include <QScrollBar>

void MainWindow::showDirectory(const QString &path, bool remember, bool sourceAlreadyBound) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    const QFileInfo info(path);
    if (!info.isDir()) {
        statusBar()->showMessage(QtLocalText("Folder not found: ", "Папка не найдена: ") + path);
        path_->setText(state_ == State::Viewing ? archiveDisplayPath() : directory_);
        return;
    }
    const QString next = info.canonicalFilePath();
    if (remember && state_ == State::Namespace)
        history_.append(QString());
    else if (remember && !directory_.isEmpty() && directory_ != next)
        history_.append(directory_);
    filesystemScanner_->cancel();
    listProjection_->cancel();
    scanningArchive_ = false;
    scanningDisk_ = false;
    scanSelectedState_.reset();
    scanSelectedPaths_.reset();
    if (!sourceAlreadyBound)
        app_.GetFocusedPanel().CloseOpenFolders();
    directory_ = next;
    autoRefreshSnapshot_.clear();
    autoRefreshSnapshot_.insert(directory_, filesystemSignature(directory_));
    recordVisitedPath(directory_);
    archive_.clear();
    displayArchive_.clear();
    archivePassword_.clear();
    archiveFolder_.clear();
    {
        const QSignalBlocker blocked(flatView_);
        flatView_->setChecked(flatDisk_);
    }
    entries_.clear();
    renameEditingItem_ = nullptr;
    renameOriginalPath_.clear();
    table_->clearSelection();
    table_->setSortingEnabled(false);
    table_->setRowCount(0);
    path_->setText(directory_);
    static_cast<FileTable *>(table_)->filesystem = true;
    startFilesystemListing(sourceAlreadyBound);
}

void MainWindow::startFilesystemListing(bool sourceAlreadyBound) {
    if (flatDisk_) {
        scanningDisk_ = true;
        setState(State::Filesystem);
        statusBar()->showMessage(QtLangText(IDS_SCANNING));
        filesystemScanner_->start(directory_);
    } else {
        COpenResult opened;
        auto &panel = app_.GetFocusedPanel();
        HRESULT result =
            sourceAlreadyBound ? S_OK : panel.BindToPath(ToUString(directory_), UString(), opened);
        CMyComPtr<IFolderFolder> folder = panel.GetFolder();
        if (result == S_OK) {
            panel.LoadFullPath();
            auto *spec = static_cast<NFsFolder::CFSFolder *>((IFolderFolder *)folder);
            bool defer = false;
            {
                auto previous = std::move(spec->SetCompleted);
                const auto restore = qScopeGuard([&] { spec->SetCompleted = std::move(previous); });
                // Preserve immediate small-folder callers. Stop the original
                // load at its callback boundary, then rerun it in the existing
                // worker for a larger list instead of walking all files here.
                spec->SetCompleted = [&](quint64 count) {
                    defer = count > 256;
                    return !defer;
                };
                result = folder->LoadItems();
            }
            if (result == E_ABORT && defer) {
                scanningDisk_ = true;
                setState(State::Filesystem);
                statusBar()->showMessage(QtLangText(IDS_SCANNING));
                filesystemScanner_->start(directory_, false);
                return;
            }
        }
        if (result == S_OK) {
            configureColumns();
            appendParentEntry();
            auto *spec = static_cast<NFsFolder::CFSFolder *>((IFolderFolder *)folder);
            for (UInt32 i = 0; i < spec->Files.Size(); ++i)
                appendFilesystemEntry(
                    QFileInfo(QDir(directory_)
                                  .filePath(QtFilesystemPath(spec->GetRelPath(spec->Files[i])))),
                    i);
        }
        applySortMode(true);
        setState(State::Filesystem);
        statusBar()->showMessage(
            result == S_OK ? QtLocalText("%1 items", "%1 объектов").arg(table_->rowCount())
                           : QtLocalText("Could not read folder.", "Не удалось прочитать папку."));
    }
}

void MainWindow::appendFilesystemEntry(const QFileInfo &file, quint32 sourceIndex) {
#ifdef Q_OS_WIN
    // Normal Windows folders already use source FindChangeNotification. Keep
    // the portable fallback for flat descendants, outside its nonrecursive watch.
    if (flatDisk_)
#endif
        autoRefreshSnapshot_.insert(file.absoluteFilePath(),
                                    filesystemSignature(file.absoluteFilePath()));
    IFolderFolder *folder = app_.GetFocusedPanel().GetFolder();
    NWindows::NCOM::CPropVariant isDir, size, name, prefix;
    folder->GetProperty(sourceIndex, kpidIsDir, &isDir);
    folder->GetProperty(sourceIndex, kpidSize, &size);
    folder->GetProperty(sourceIndex, kpidName, &name);
    folder->GetProperty(sourceIndex, kpidPrefix, &prefix);
    const bool directory = isDir.vt == VT_BOOL && isDir.boolVal != VARIANT_FALSE;
    const QString nativeName =
        name.vt == VT_BSTR ? QString::fromWCharArray(name.bstrVal) : file.fileName();
    const QString nativePrefix =
        prefix.vt == VT_BSTR ? QDir::fromNativeSeparators(QString::fromWCharArray(prefix.bstrVal))
                             : QString();
    const QVariant sizeValue =
        size.vt == VT_UI8 ? QVariant::fromValue(quint64(size.uhVal.QuadPart)) : QVariant();
    const int row = table_->rowCount();
    table_->insertRow(row);
    const QString displayName = ((flatDisk_ ? nativePrefix : QString()) + nativeName)
                                    .normalized(QString::NormalizationForm_C);
    QStringList values = {displayName,
                          file.absoluteFilePath(),
                          directory ? QString() : sizeValue.toString(),
                          QString(),
                          file.lastModified().toString(Qt::ISODate),
                          GetExtensionPtr(QStringView(nativeName)).toString(),
                          QString::number(sourceIndex)};
    for (int col : {2, 3, 4})
        if (!table_->isColumnHidden(col))
            values[col] = sourceColumnText(sourceIndex, col);
    for (int col = 7; col < table_->columnCount(); ++col)
        values << (table_->isColumnHidden(col) ? QString() : sourceColumnText(sourceIndex, col));
    for (int col = 0; col < values.size(); ++col) {
        auto *item = new ArchiveTableItem(values[col]);
        item->setData(Qt::UserRole + 1, directory);
        item->setData(Qt::UserRole + 3, nativeName);
        item->setData(Qt::UserRole + 5, nativePrefix);
        item->setData(Qt::UserRole + 6, sourceIndex);
        if (col == 0) {
            item->setToolTip(file.absoluteFilePath());
            if (fileListSettings_.ShowRealFileIcons) {
                item->setIcon(g_Ext_to_Icon_Map.GetIcon(
                    directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE, nativeName));
            } else
                item->setIcon(QApplication::style()->standardIcon(directory ? QStyle::SP_DirIcon
                                                                            : QStyle::SP_FileIcon));
        }
        if (col == 2 && !directory)
            item->setData(Qt::UserRole + 2, sizeValue);
        if (col == 6)
            item->setData(Qt::UserRole + 2, sourceIndex);
        if (col != 1 && col != 5 && col != 6)
            item->setTextAlignment(sourceColumnAlignment(col));
        table_->setItem(row, col, item);
    }
}

QStringList MainWindow::selectedPaths() const {
    QStringList paths;
    for (const QModelIndex &index : table_->selectionModel()->selectedRows())
        paths << table_->item(index.row(), 1)->text();
    return paths;
}

void MainWindow::selectPaths(const QStringList &paths) {
    if (scanningDisk_ || scanningArchive_) {
        scanSelectedPaths_ = paths;
        return;
    }
    table_->clearSelection();
    const auto &panel = app_.GetFocusedPanel();
    const bool streams = panel.GetFolder() && panel.IsAltStreamsFolder();
    for (int row = 0; row < table_->rowCount(); ++row) {
        const QString path = streams ? table_->item(row, 0)->data(Qt::UserRole).toString()
                                     : table_->item(row, 1)->text();
        if (paths.contains(path))
            table_->selectionModel()->select(table_->model()->index(row, 0),
                                             QItemSelectionModel::Select |
                                                 QItemSelectionModel::Rows);
    }
}

void MainWindow::refreshCurrent(const CSelectedState *preserved) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ == State::Namespace) {
        auto &panel = app_.GetFocusedPanel();
        CSelectedState selection;
        if (preserved)
            selection = *preserved;
        else
            panel.SaveSelectedState(selection);
        renderRootFolder();
        panel.RefreshListCtrl(selection);
        refreshActions();
        return;
    }
    if (state_ == State::Filesystem) {
        CSelectedState selection;
        if (preserved)
            selection = *preserved;
        else
            app_.GetFocusedPanel().SaveSelectedState(selection);
        showDirectory(directory_, false);
        restoreListSelection(selection);
        if (!scanningDisk_) {
            restoreAutoRefreshViewport();
            refreshActions();
        }
    } else if (state_ == State::Viewing) {
        auto &panel = app_.GetFocusedPanel();
        CSelectedState selection;
        if (preserved)
            selection = *preserved;
        else
            panel.SaveSelectedState(selection);
        CMyComPtr<IFolderFolder> oldFolder = panel.GetFolder();
        auto *folder = dynamic_cast<CAgentFolder *>((IFolderFolder *)oldFolder);
        if (!folder)
            return;
        const UString previousFolderPath = GetFolderPath(oldFolder);
        setState(State::Loading);
        CMyComPtr<IFolderFolder> refreshed;
        HRESULT result;
        {
            QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
            const auto &parents = panel.GetParentFolders();
            result = ReOpenArchiveFolder(*folder, toolbarSettingsPath(), this, &refreshed,
                                         !parents.IsEmpty() && parents.Back().UsePassword,
                                         parents.IsEmpty() ? UString() : parents.Back().Password);
        }
        if (result == S_OK) {
            panel.SetNewFolder(refreshed);
            renderBoundFolder();
            restoreListSelection(GetFolderPath(refreshed) == previousFolderPath ? selection
                                                                                : CSelectedState());
            refreshActions();
        } else {
            // Native ReOpen releases the old proxy; never render stale folders.
            showDirectory(returnDirectory_, false);
            if (result != E_ABORT)
                QMessageBox::warning(
                    this, QStringLiteral("7-Zip"),
                    QtLocalText("Could not update archive (%1).", "Не удалось обновить архив (%1).")
                        .arg(quint32(result), 0, 16));
        }
    }
}

MainWindow::FilesystemSignature MainWindow::filesystemSignature(const QString &path) const {
    const QFileInfo info(path);
    return {info.lastModified(), info.metadataChangeTime(), info.size(), info.exists(),
            info.isDir()};
}

void MainWindow::pollAutoRefresh() {
    // Native modal commands hold panel timer/notification guards while their
    // workers can rebuild archive proxies. Do not change the Qt projection
    // (and query the other panel) before CPanel::OnTimer sees those guards.
    if (scanningDisk_ || app_.QtOperations().active() || app_.QtOperations().closed() ||
        app_.QtOperations().callActive() || sourceArchiveProgress_)
        return;
    const unsigned original = projectedPanel_;
    for (unsigned index = 0; index < kNumPanelsMax; ++index) {
        if (!app_.IsPanelVisible(index) || !panelUi_[index].container)
            continue;
        if (index != projectedPanel_)
            activatePanelUi(index);
        if (state_ == State::Filesystem) {
            CPanel &panel = app_.GetFocusedPanel();
            if (panel.AutoRefresh_Mode && !autoRefreshSnapshot_.isEmpty()) {
                for (auto it = autoRefreshSnapshot_.cbegin(); it != autoRefreshSnapshot_.cend();
                     ++it) {
                    if (!(filesystemSignature(it.key()) == it.value())) {
                        panel.MarkPortableFolderChanged();
                        break;
                    }
                }
            }
            panel.OnTimer();
        }
        if (app_.QtOperations().active())
            break;
    }
    if (original != projectedPanel_)
        activatePanelUi(original);
}

void MainWindow::restoreAutoRefreshViewport() {
    if (autoRefreshScrollValue_ < 0)
        return;
    int row = -1;
    for (int i = 0; i < table_->rowCount(); ++i) {
        if (table_->item(i, 1) && table_->item(i, 1)->text() == autoRefreshTopPath_) {
            row = i;
            break;
        }
    }
    if (row >= 0)
        table_->scrollToItem(table_->item(row, 0), QAbstractItemView::PositionAtTop);
    else
        table_->verticalScrollBar()->setValue(autoRefreshScrollValue_);
    autoRefreshTopPath_.clear();
    autoRefreshScrollValue_ = -1;
}

void MainWindow::createFolder() {
    if (state_ != State::Filesystem && state_ != State::Viewing)
        return;
    const bool archive = state_ == State::Viewing;
    if (archive && !createFolder_->isEnabled())
        return;
    QString name;
    if (!Dlg_CreateFolder(this, name))
        return;
    UString createdName;
    const HRESULT result = app_.GetFocusedPanel().CreateFolder(ToUString(name), createdName);
    if (result == E_ABORT)
        return;
    if (result != S_OK) {
        QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось создать папку.");
        return;
    }
    if (archive) {
        renderBoundFolder();
        return;
    }
    showDirectory(directory_, false);
    selectPaths({QDir(directory_).filePath(QDir::fromNativeSeparators(name).section('/', 0, 0))});
}

void MainWindow::createFile() {
    const auto &panel = app_.GetFocusedPanel();
    const bool streams = panel.GetFolder() && panel.IsAltStreamsFolder();
    if (state_ != State::Filesystem && !streams)
        return;
    ComboDialog dialog("Создать файл", "Имя файла:", "Новый файл", this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString name = dialog.value();
    UString createdName;
    const HRESULT result = app_.GetFocusedPanel().CreateFile(ToUString(name), createdName);
    if (result == E_ABORT)
        return;
    if (result != S_OK) {
        QMessageBox::warning(this, "Создать файл", "Не удалось создать файл.");
        return;
    }
    if (streams) {
        const QString target =
            ToQString(app_.GetFocusedPanel().GetCurrentFolderPrefix()) + ToQString(createdName);
        renderBoundFolder();
        selectPaths({target});
        return;
    }
    const QString target = QDir(directory_).filePath(ToQString(createdName));
    showDirectory(directory_, false);
    selectPaths({target});
}

void MainWindow::openPath(const QString &path) { navigateTo(QFileInfo(path).absoluteFilePath()); }

void MainWindow::navigateTo(const QString &path) {
    if (state_ == State::Loading || state_ == State::Operating)
        return;
    if (state_ == State::Viewing && path == archiveDisplayPath())
        return;
    // A typed relative address belongs to the displayed filesystem folder.
    // CPanel::BindToPath still resolves the resulting native path.
    const QString nativeAddress = state_ == State::Filesystem && QDir::isRelativePath(path)
                                      ? QDir(directory_).filePath(path)
                                      : path;
    const QFileInfo info(nativeAddress);
    if (info.isDir())
        showDirectory(nativeAddress, true);
    else if (info.isFile() && !info.isSymLink())
        openArchive(nativeAddress);
    else {
        QString nativePath = nativeAddress;
        nativePath.replace(QStringLiteral(" / "), QStringLiteral("/"));
        auto &panel = app_.GetFocusedPanel();
        HRESULT result;
        {
            QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
            COpenResult opened;
            result = panel.BindToPath(ToUString(nativePath), UString(), opened);
        }
        renderBoundFolder();
        if (result != S_OK && result != E_ABORT)
            statusBar()->showMessage(QtLocalText("Path not found: ", "Путь не найден: ") + path);
    }
}

void MainWindow::activateRow(int row) {
    if (renameActivationPending_)
        return;
    if (row < 0 || row >= table_->rowCount())
        return;
    if (table_->item(row, 0)->data(Qt::UserRole + 6).toUInt() == UInt32(-1)) {
        goParent();
        return;
    }
    if (!table_->selectionModel()->isRowSelected(row, QModelIndex()))
        table_->selectRow(row);
    table_->setCurrentCell(row, 0, QItemSelectionModel::NoUpdate);
    app_.ExecuteCommand(IDM_OPEN);
}

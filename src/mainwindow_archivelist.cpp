#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "CPP/7zip/UI/FileManager/PanelSplitFile.h"
#include "CPP/7zip/UI/FileManager/SysIconUtils.h"
#include "Common/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "mainwindow.h"
#include "qtfilemanagercommands.h"
#include "qtlanguageservices.h"
#include "qtlocationfolder.h"

void MainWindow::renderArchiveFolder() {
    listProjection_->cancel();
    scanningArchive_ = false;
    scanSelectedState_.reset();
    scanSelectedPaths_.reset();
    auto &panel = app_.GetFocusedPanel();
    std::optional<CPanel::CDisableTimerProcessing> disableTimerProcessing;
    std::optional<CPanel::CDisableNotify> disableNotify;
    disableTimerProcessing.emplace(panel);
    disableNotify.emplace(panel);
    CMyComPtr<IFolderFolder> folderOwner = panel.GetFolder();
    if (!folderOwner)
        return;
    auto *archiveFolder = static_cast<CAgentFolder *>((IFolderFolder *)folderOwner);
    const bool flat = flatView_->isChecked();
    archiveFolder->SetFlatMode(BoolToInt(flat));
    if (archiveFolder->LoadItems() != S_OK)
        return;
    static_cast<FileTable *>(table_)->filesystem = false;
    autoRefreshSnapshot_.clear();
    if (!isNestedArchive())
        recordVisitedPath(archiveDisplayPath());
    const QString prefix = archiveFolder_.isEmpty() ? QString() : archiveFolder_ + '/';
    table_->setSortingEnabled(false);
    app_.GetFocusedPanel().SetNewFolder(folderOwner);
    app_.GetFocusedPanel().LoadFullPath();
    configureColumns();
    appendParentEntry();
    if (archiveFolder->GetNumberOfItems() > 256) {
        // Finish the synchronous source guards before retaining operation
        // guards. Otherwise an idle flat-view change saves disabled flags
        // and leaves notifications disabled when asynchronous loading ends.
        disableNotify.reset();
        disableTimerProcessing.reset();
        scanningArchive_ = true;
        setState(State::Viewing);
        path_->setText(archiveDisplayPath());
        listProjection_->start(
            folderOwner, archiveFolder->GetNumberOfItems(),
            [this, archiveFolder, prefix, flat](quint32 index) {
                appendArchiveEntry(*archiveFolder, index, prefix, flat);
            },
            [this] {
                scanningArchive_ = false;
                applySortMode(true);
                completeListSelection();
                setState(State::Viewing);
                statusBar()->showMessage(
                    QtLocalText("%1 items", "%1 объектов").arg(table_->rowCount()));
            });
        return;
    }
    for (unsigned index = 0; index < archiveFolder->GetNumberOfItems(); ++index)
        appendArchiveEntry(*archiveFolder, index, prefix, flat);
    applySortMode(true);
    path_->setText(archiveDisplayPath());
    statusBar()->showMessage(
        QtLocalText("%1 items", "%1 объектов").arg(archiveFolder->GetNumberOfItems()));
    refreshActions();
}

void MainWindow::appendArchiveEntry(CAgentFolder &folder, unsigned index, const QString &prefix,
                                    bool flat) {
    NWindows::NCOM::CPropVariant directory;
    folder.GetProperty(index, kpidIsDir, &directory);
    const bool isDir = directory.vt == VT_BOOL && directory.boolVal != VARIANT_FALSE;
    const int realIndex = folder.GetRealIndex(index);
    ArchiveEntry entry;
    entry.directory = isDir;
    const QString name = ToQString(folder.GetName(index));
    const QString panelPrefix = QDir::fromNativeSeparators(ToQString(folder.GetFullPrefix(index)));
    entry.path = panelPrefix + name;
    const QString displayName = flat ? panelPrefix.mid(prefix.size()) + name : name;
    const int row = table_->rowCount();
    table_->insertRow(row);
    const int sourceOrder = int(index);
    NWindows::NCOM::CPropVariant size, packedSize, modified;
    folder.GetProperty(index, kpidSize, &size);
    folder.GetProperty(index, kpidPackSize, &packedSize);
    folder.GetProperty(index, kpidMTime, &modified);
    if (modified.vt == VT_FILETIME) {
        char text[64];
        if (ConvertUtcFileTimeToString(modified.filetime, text))
            entry.modified = QString::fromLatin1(text);
    }
    const QVariant sizeValue =
        size.vt == VT_UI8 ? QVariant::fromValue(quint64(size.uhVal.QuadPart)) : QVariant();
    const QVariant packedValue = packedSize.vt == VT_UI8
                                     ? QVariant::fromValue(quint64(packedSize.uhVal.QuadPart))
                                     : QVariant();
    QStringList values = {displayName,
                          entry.path,
                          entry.directory ? QString() : sizeValue.toString(),
                          entry.directory ? QString() : packedValue.toString(),
                          entry.modified,
                          GetExtensionPtr(QStringView(name)).toString(),
                          QString::number(sourceOrder)};
    for (int col : {2, 3, 4})
        if (!table_->isColumnHidden(col))
            values[col] = sourceColumnText(index, col);
    for (int col = 7; col < table_->columnCount(); ++col)
        values << (table_->isColumnHidden(col) ? QString() : sourceColumnText(index, col));
    for (int col = 0; col < values.size(); ++col) {
        auto *item = new ArchiveTableItem(values[col]);
        item->setData(Qt::UserRole, entry.path);
        item->setData(Qt::UserRole + 1, entry.directory);
        item->setData(Qt::UserRole + 3, name);
        item->setData(Qt::UserRole + 5, flat ? panelPrefix.mid(prefix.size()) : QString());
        item->setData(Qt::UserRole + 6, sourceOrder);
        item->setToolTip(entry.path);
        if (col == 0)
            item->setIcon(
                fileListSettings_.ShowRealFileIcons
                    ? g_Ext_to_Icon_Map.GetIcon(
                          entry.directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE, name)
                    : QApplication::style()->standardIcon(entry.directory ? QStyle::SP_DirIcon
                                                                          : QStyle::SP_FileIcon));
        if (!entry.directory && (col == 2 || col == 3))
            item->setData(Qt::UserRole + 2, col == 2 ? sizeValue : packedValue);
        if (col == 6)
            item->setData(Qt::UserRole + 2, sourceOrder);
        if (col != 1 && col != 5 && col != 6)
            item->setTextAlignment(sourceColumnAlignment(col));
        table_->setItem(row, col, item);
    }
}

void MainWindow::setState(State state) {
    const auto kind = state == State::Loading             ? QtAppOperationState::Kind::Open
                      : state == State::Operating         ? QtAppOperationState::Kind::Operation
                      : scanningDisk_ || scanningArchive_ ? QtAppOperationState::Kind::Scan
                                                          : QtAppOperationState::Kind::Idle;
    if (!app_.QtOperations().Begin(kind))
        return;
    state_ = state;
    refreshActions();
}

void MainWindow::refreshActions() {
    app_.RefreshTitle();
    const bool navigating = !app_.QtOperations().closed() && !app_.QtOperations().blocking();
    const bool idle = navigating && !scanningDisk_ && !scanningArchive_;
    refreshBookmarks();
    if (commands_)
        commands_->RefreshActions();
    if (commands_ && !scanningDisk_ && !scanningArchive_)
        refreshPanelStatus();
    for (unsigned index = 0; index < kNumPanelsMax; ++index) {
        const auto &ui = panelUi_[index];
        if (!ui.container || !ui.table || !ui.iconList || !ui.path)
            continue;
        const bool projecting = index == projectedPanel_ && (scanningDisk_ || scanningArchive_);
        const bool panelIdle =
            navigating && (index != projectedPanel_ || (!scanningDisk_ && !scanningArchive_));
        // RefreshListCtrl disables redraw until native sorting and selection
        // restoration finish. Keep that policy across Qt's asynchronous batches.
        ui.table->setUpdatesEnabled(!projecting);
        ui.iconList->setUpdatesEnabled(!projecting);
        ui.table->setEnabled(panelIdle);
        ui.iconList->setEnabled(panelIdle);
        // Native archive progress is modal to the file manager. Qt disables
        // its lists during the operation; disable the address controls too so
        // focus cannot move to an inactive pane while its source guards block
        // the corresponding focus notification.
        ui.pathCombo->setEnabled(navigating);
        ui.path->setReadOnly(!navigating);
    }
    const bool dragAvailable = idle && app_.GetFocusedPanel().DoesItSupportOperations() &&
                               (state_ == State::Filesystem || state_ == State::Viewing);
    table_->setDragEnabled(dragAvailable);
    iconList_->setDragEnabled(dragAvailable);
}

void MainWindow::refreshPanelStatus() {
    const CPanelStatus status = app_.GetFocusedPanel().Refresh_StatusBar();
    const auto &labels = panelUi_[projectedPanel_].statusLabels;
    if (!labels[0])
        return;
    labels[0]->setText(
        ToQString(MyFormatNew(app_.LangString_N_SELECTED_ITEMS, ToUString(status.selectedItems))));
    labels[1]->setText(status.selectedSize);
    labels[2]->setText(status.focusedSize);
    labels[3]->setText(status.focusedDate);
}

#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "CPP/7zip/UI/FileManager/PanelSplitFile.h"
#include "Common/Defs.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "mainwindow.h"
#include "qtlocationfolder.h"

void MainWindow::renderArchiveFolder() {
    auto &panel = app_.GetFocusedPanel();
    CPanel::CDisableTimerProcessing disableTimerProcessing(panel);
    CPanel::CDisableNotify disableNotify(panel);
    CMyComPtr<IFolderFolder> folderOwner = panel.GetFolder();
    if (!folderOwner)
        return;
    auto *archiveFolder = static_cast<CAgentFolder *>((IFolderFolder *)folderOwner);
    const bool flat = flatView_->isChecked();
    archiveFolder->SetFlatMode(BoolToInt(flat));
    if (archiveFolder->LoadItems() != S_OK)
        return;
    static_cast<FileTable *>(table_)->filesystem = false;
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    table_->setColumnHidden(3, !settings.value("columns/3", true).toBool());
    if (!isNestedArchive())
        recordVisitedPath(archiveDisplayPath());
    const QString prefix = archiveFolder_.isEmpty() ? QString() : archiveFolder_ + '/';
    table_->setSortingEnabled(false);
    app_.GetFocusedPanel().SetNewFolder(folderOwner);
    app_.GetFocusedPanel().LoadFullPath();
    for (unsigned index = 0; index < archiveFolder->GetNumberOfItems(); ++index)
        appendArchiveEntry(*archiveFolder, index, prefix, flat);
    table_->setSortingEnabled(true);
    applySortMode();
    path_->setText(archiveDisplayPath());
    statusBar()->showMessage(QStringLiteral("%1 объектов").arg(archiveFolder->GetNumberOfItems()));
    refreshActions();
}

void MainWindow::appendArchiveEntry(CAgentFolder &folder, unsigned index, const QString &prefix,
                                    bool flat) {
    const CProxyArc &proxy = folder.GetProxy();
    const CProxyItem proxyItem = folder.GetItem(index);
    const CProxyDir &parent = proxy.Dirs[proxyItem.DirIndex];
    const bool isDir = proxyItem.Index < parent.SubDirs.Size();
    const int realIndex = proxy.GetRealIndex(proxyItem.DirIndex, proxyItem.Index);
    const unsigned childDir = isDir ? parent.SubDirs[proxyItem.Index] : 0;
    ArchiveEntry entry;
    entry.directory = isDir;
    const QString name = QString::fromWCharArray(isDir ? proxy.Dirs[childDir].Name
                                                       : proxy.Files[unsigned(realIndex)].Name);
    const QString panelPrefix =
        QDir::fromNativeSeparators(ToQString(proxy.GetDirPath_as_Prefix(proxyItem.DirIndex)));
    if (isDir) {
        entry.path = QDir::fromNativeSeparators(ToQString(proxy.GetDirPath_as_Prefix(childDir)));
        entry.path.chop(1);
    } else
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
    const QStringList values = {displayName,
                                entry.path,
                                entry.directory ? QString() : sizeValue.toString(),
                                entry.directory ? QString() : packedValue.toString(),
                                entry.modified,
                                GetExtensionPtr(QStringView(name)).toString(),
                                QString::number(sourceOrder)};
    for (int col = 0; col < values.size(); ++col) {
        auto *item = new ArchiveTableItem(values[col]);
        item->setData(Qt::UserRole, entry.path);
        item->setData(Qt::UserRole + 1, entry.directory);
        item->setData(Qt::UserRole + 3, name);
        item->setData(Qt::UserRole + 5, flat ? panelPrefix.mid(prefix.size()) : QString());
        item->setData(Qt::UserRole + 6, sourceOrder);
        if (isDir)
            item->setData(Qt::UserRole + 4, childDir);
        item->setToolTip(entry.path);
        if (col == 0)
            item->setIcon(QApplication::style()->standardIcon(
                entry.directory ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
        if (!entry.directory && (col == 2 || col == 3))
            item->setData(Qt::UserRole + 2, col == 2 ? sizeValue : packedValue);
        if (col == 6)
            item->setData(Qt::UserRole + 2, sourceOrder);
        if (col == 2 || col == 3)
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table_->setItem(row, col, item);
    }
}

void MainWindow::setState(State state) {
    const auto kind = state == State::Loading     ? QtAppOperationState::Kind::Open
                      : state == State::Operating ? QtAppOperationState::Kind::Operation
                      : scanningDisk_             ? QtAppOperationState::Kind::Scan
                                                  : QtAppOperationState::Kind::Idle;
    if (!app_.QtOperations().Begin(kind))
        return;
    state_ = state;
    refreshActions();
}

void MainWindow::refreshActions() {
    const bool navigating = !app_.QtOperations().closed() && !app_.QtOperations().blocking();
    const bool idle = navigating && !scanningDisk_;
    for (QAction *action : sortActions_)
        action->setEnabled(idle);
    for (QAction *action : viewActions_)
        action->setEnabled(idle);
    flatView_->setEnabled(navigating);
    open_->setEnabled(idle);
    create_->setEnabled(idle && state_ == State::Filesystem);
    add_->setEnabled(idle && state_ == State::Viewing && ArchiveFormats::canModify(archive_) &&
                     !info_.encrypted && !isNestedArchive());
    toolbarAdd_->setEnabled(create_->isEnabled() || add_->isEnabled());
    extract_->setEnabled(idle && state_ == State::Viewing && !entries_.isEmpty());
    test_->setEnabled(idle && state_ == State::Viewing);
    infoAction_->setEnabled(idle && state_ == State::Viewing);
    selected_->setEnabled(idle && state_ == State::Viewing && !entries_.isEmpty() &&
                          !table_->selectionModel()->selectedRows().isEmpty());
    back_->setEnabled(navigating && (state_ == State::Viewing || !history_.isEmpty()));
    up_->setEnabled(navigating && state_ == State::Viewing && !archiveFolder_.isEmpty());
    bool canOpenItem = false;
    const int row = table_->currentRow();
    if (idle && row >= 0 && row < table_->rowCount()) {
        if (state_ == State::Viewing)
            canOpenItem = true;
        else {
            const QFileInfo info(table_->item(row, 1)->text());
            canOpenItem = info.isDir() || (info.isFile() && !info.isSymLink());
        }
    }
    openItem_->setEnabled(canOpenItem);
    bool inside = false;
    bool outside = false;
    if (canOpenItem) {
        const bool folder = table_->item(row, 0)->data(Qt::UserRole + 1).toBool();
        const QString path = state_ == State::Filesystem
                                 ? table_->item(row, 1)->text()
                                 : table_->item(row, 0)->data(Qt::UserRole).toString();
        inside = folder || (state_ == State::Viewing || !QFileInfo(path).isSymLink());
        outside = !folder;
    }
    openInside_->setEnabled(inside);
    openOutside_->setEnabled(outside);
    parent_->setEnabled(navigating && (state_ == State::Viewing ||
                                       (state_ == State::Filesystem && QDir(directory_).cdUp())));
    selectAll_->setEnabled(idle && table_->rowCount() > 0);
    const bool selected = !table_->selectionModel()->selectedRows().isEmpty();
    deselectAll_->setEnabled(idle && selected);
    invertSelection_->setEnabled(idle && table_->rowCount() > 0);
    refresh_->setEnabled(idle);
    settingsAction_->setEnabled(idle);
    foldersHistory_->setEnabled(idle && pathCombo_->count() > 0);
    refreshBookmarks();
    createFolder_->setEnabled(idle && state_ == State::Filesystem);
    createFile_->setEnabled(idle && state_ == State::Filesystem);
    selectMask_->setEnabled(idle && table_->rowCount() > 0);
    deselectMask_->setEnabled(idle && table_->rowCount() > 0);
    selectType_->setEnabled(idle && table_->currentRow() >= 0);
    deselectType_->setEnabled(idle && table_->currentRow() >= 0);
    copy_->setEnabled(idle && selected);
    const bool writableArchive = state_ == State::Viewing && ArchiveFormats::canModify(archive_) &&
                                 !info_.encrypted && !isNestedArchive();
    move_->setEnabled(idle && selected && (state_ == State::Filesystem || writableArchive));
    remove_->setEnabled(idle && selected && (state_ == State::Filesystem || writableArchive));
    const bool one = table_->selectionModel()->selectedRows().size() == 1;
    bool oneFile = false;
    if (idle && one) {
        const auto indices = sourceOperatedIndices();
        oneFile = indices.Size() == 1 && !app_.GetFocusedPanel().IsItem_Folder(indices[0]);
    }
    viewFile_->setEnabled(idle && oneFile);
    editFile_->setEnabled(idle && oneFile && (state_ == State::Filesystem || writableArchive));
    split_->setEnabled(idle && state_ == State::Filesystem && oneFile &&
                       !QFileInfo(selectedFilesystemPaths().value(0)).isSymLink());
    CVolSeqName sequence;
    combine_->setEnabled(
        split_->isEnabled() &&
        sequence.ParseName(QFileInfo(selectedFilesystemPaths().value(0)).fileName()));
    rename_->setEnabled(idle && one && (state_ == State::Filesystem || writableArchive));
    properties_->setEnabled(idle && selected);
    for (QAction *action : hashActions_)
        action->setEnabled(idle && selected);
    table_->setEnabled(idle);
    iconList_->setEnabled(idle);
    table_->setDragEnabled(idle && state_ == State::Filesystem);
    iconList_->setDragEnabled(idle && state_ == State::Filesystem);
    path_->setReadOnly(!navigating);
}

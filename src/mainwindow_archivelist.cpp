#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "mainwindow.h"

void MainWindow::showArchiveFolder(const QString &folder) {
    archiveFolder_ = folder;
    static_cast<FileTable *>(table_)->filesystem = false;
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    table_->setColumnHidden(3, !settings.value("columns/3", true).toBool());
    const int dirIndex = proxyArchive_.FindDir(folder);
    if (dirIndex < 0)
        return;
    if (archiveParents_.empty())
        recordVisitedPath(archiveDisplayPath());
    const QString prefix = folder.isEmpty() ? QString() : folder + '/';
    const bool flat = flatView_->isChecked();
    CAgentFolder archiveFolder(proxyArchive_, unsigned(dirIndex), flat);
    archiveFolder.LoadItems();
    table_->setSortingEnabled(false);
    table_->clearSelection();
    table_->setRowCount(0);
    for (unsigned index = 0; index < archiveFolder.GetNumberOfItems(); ++index) {
        const CProxyItem proxyItem = archiveFolder.GetItem(index);
        const CProxyDir &parent = proxyArchive_.Dirs[proxyItem.DirIndex];
        const bool isDir = proxyItem.Index < parent.SubDirs.size();
        const int realIndex = proxyArchive_.GetRealIndex(proxyItem.DirIndex, proxyItem.Index);
        const unsigned childDir = isDir ? parent.SubDirs[proxyItem.Index] : 0;
        ArchiveEntry entry = realIndex < 0 ? ArchiveEntry{} : entries_[realIndex];
        entry.directory = isDir;
        const QString name = isDir ? proxyArchive_.Dirs[childDir].Name
                                   : proxyArchive_.Files[unsigned(realIndex)].Name;
        const QString panelPrefix = proxyArchive_.GetDirPath_as_Prefix(proxyItem.DirIndex);
        if (isDir) {
            entry.path = proxyArchive_.GetDirPath_as_Prefix(childDir);
            entry.path.chop(1);
        }
        const QString displayName = flat ? panelPrefix.mid(prefix.size()) + name : name;
        const int row = table_->rowCount();
        table_->insertRow(row);
        const int sourceOrder = int(index);
        const QStringList values = {displayName,
                                    entry.path,
                                    entry.directory ? QString() : QString::number(entry.size),
                                    entry.directory ? QString() : QString::number(entry.packedSize),
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
                item->setData(Qt::UserRole + 2, col == 2 ? entry.size : entry.packedSize);
            if (col == 6)
                item->setData(Qt::UserRole + 2, sourceOrder);
            if (col == 2 || col == 3)
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table_->setItem(row, col, item);
        }
    }
    table_->setSortingEnabled(true);
    applySortMode();
    path_->setText(archiveDisplayPath());
    statusBar()->showMessage(QStringLiteral("%1 объектов").arg(archiveFolder.GetNumberOfItems()));
    refreshActions();
}

void MainWindow::setState(State state) {
    state_ = state;
    refreshActions();
}

void MainWindow::refreshActions() {
    const bool navigating = state_ != State::Loading && state_ != State::Operating;
    const bool idle = navigating && !scanningDisk_;
    for (QAction *action : sortActions_)
        action->setEnabled(idle);
    for (QAction *action : viewActions_)
        action->setEnabled(idle);
    flatView_->setEnabled(navigating);
    open_->setEnabled(idle);
    create_->setEnabled(idle && state_ == State::Filesystem);
    add_->setEnabled(idle && state_ == State::Viewing && ArchiveFormats::canModify(archive_) &&
                     !info_.encrypted && archiveParents_.empty());
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
                                 !info_.encrypted && archiveParents_.empty();
    move_->setEnabled(idle && selected && (state_ == State::Filesystem || writableArchive));
    remove_->setEnabled(idle && selected && (state_ == State::Filesystem || writableArchive));
    const bool one = table_->selectionModel()->selectedRows().size() == 1;
    const bool oneFile =
        one && !table_->item(table_->selectionModel()->selectedRows().first().row(), 0)
                    ->data(Qt::UserRole + 1)
                    .toBool();
    viewFile_->setEnabled(idle && oneFile);
    editFile_->setEnabled(idle && oneFile && (state_ == State::Filesystem || writableArchive));
    split_->setEnabled(idle && state_ == State::Filesystem && oneFile &&
                       !QFileInfo(selectedFilesystemPaths().value(0)).isSymLink());
    combine_->setEnabled(split_->isEnabled() &&
                         QRegularExpression(R"(\.0{2,}1$)")
                             .match(QFileInfo(selectedFilesystemPaths().value(0)).fileName())
                             .hasMatch());
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

QStringList MainWindow::selectedArchivePaths() const {
    QStringList paths;
    if (state_ != State::Viewing)
        return paths;
    for (const QModelIndex &index : table_->selectionModel()->selectedRows()) {
        const auto *item = table_->item(index.row(), 0);
        const QString path = item->data(Qt::UserRole).toString();
        if (!item->data(Qt::UserRole + 1).toBool()) {
            paths << path;
            continue;
        }
        const unsigned dirIndex = item->data(Qt::UserRole + 4).toUInt();
        std::vector<unsigned> realIndices;
        proxyArchive_.AddRealIndices(dirIndex, realIndices);
        for (unsigned realIndex : realIndices)
            paths << entries_[realIndex].path;
    }
    paths.removeDuplicates();
    return paths;
}

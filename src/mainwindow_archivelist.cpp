#include "mainwindow.h"

void MainWindow::showArchiveFolder(const QString &folder) {
    archiveFolder_ = folder;
    if (archiveParents_.empty())
        recordVisitedPath(archive_);
    static_cast<FileTable *>(table_)->filesystem = false;
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    table_->setColumnHidden(3, !settings.value("columns/3", true).toBool());
    QMap<QString, ArchiveEntry> children;
    QMap<QString, int> sourceOrder;
    const QString prefix = folder.isEmpty() ? QString() : folder + '/';
    const bool flat = flatView_->isChecked();
    for (const ArchiveEntry &entry : entries_) {
        QString path = entry.path;
        if (path.endsWith('/'))
            path.chop(1);
        if (!path.startsWith(prefix) || path == folder)
            continue;
        const QString relative = path.mid(prefix.size());
        const int slash = relative.indexOf('/');
        const QString name = flat || slash < 0 ? relative : relative.left(slash);
        if (name.isEmpty())
            continue;
        if (!sourceOrder.contains(name))
            sourceOrder.insert(name, sourceOrder.size());
        const QString childPath = prefix + name;
        if (!flat && slash >= 0) {
            if (!children.contains(name)) {
                ArchiveEntry implicit;
                implicit.path = childPath;
                implicit.directory = true;
                children.insert(name, implicit);
            }
        } else {
            ArchiveEntry visible = entry;
            visible.path = childPath;
            children.insert(name, visible);
        }
        if (flat) {
            int separator = relative.indexOf('/');
            while (separator >= 0) {
                const QString parent = relative.left(separator);
                if (!children.contains(parent)) {
                    ArchiveEntry implicit;
                    implicit.path = prefix + parent;
                    implicit.directory = true;
                    children.insert(parent, implicit);
                    sourceOrder.insert(parent, sourceOrder.size());
                }
                separator = relative.indexOf('/', separator + 1);
            }
        }
    }
    table_->setSortingEnabled(false);
    table_->clearSelection();
    table_->setRowCount(0);
    for (auto it = children.cbegin(); it != children.cend(); ++it) {
        const ArchiveEntry &entry = it.value();
        const int row = table_->rowCount();
        table_->insertRow(row);
        const QStringList values = {it.key(),
                                    entry.path,
                                    entry.directory ? QString() : QString::number(entry.size),
                                    entry.directory ? QString() : QString::number(entry.packedSize),
                                    entry.modified,
                                    entry.directory ? QString()
                                                    : QFileInfo(it.key()).suffix().toCaseFolded(),
                                    QString::number(sourceOrder.value(it.key()))};
        for (int col = 0; col < values.size(); ++col) {
            auto *item = new ArchiveTableItem(values[col]);
            item->setData(Qt::UserRole, entry.path);
            item->setData(Qt::UserRole + 1, entry.directory);
            item->setData(Qt::UserRole + 3, it.key());
            item->setToolTip(entry.path);
            if (col == 0)
                item->setIcon(QApplication::style()->standardIcon(
                    entry.directory ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
            if (!entry.directory && (col == 2 || col == 3))
                item->setData(Qt::UserRole + 2, col == 2 ? entry.size : entry.packedSize);
            if (col == 6)
                item->setData(Qt::UserRole + 2, sourceOrder.value(it.key()));
            if (col == 2 || col == 3)
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            table_->setItem(row, col, item);
        }
    }
    table_->setSortingEnabled(true);
    applySortMode();
    path_->setText(archiveDisplayPath());
    statusBar()->showMessage(QStringLiteral("%1 объектов").arg(children.size()));
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
    add_->setEnabled(idle && state_ == State::Viewing && !info_.encrypted &&
                     archiveParents_.empty());
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
        const QString suffix = QFileInfo(path).suffix().toLower();
        inside = folder || suffix == "7z" || suffix == "zip";
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
    move_->setEnabled(idle && state_ == State::Filesystem && selected);
    remove_->setEnabled(
        idle && selected &&
        (state_ == State::Filesystem ||
         (state_ == State::Viewing && !info_.encrypted && archiveParents_.empty())));
    const bool one = table_->selectionModel()->selectedRows().size() == 1;
    const bool oneFile =
        one && !table_->item(table_->selectionModel()->selectedRows().first().row(), 0)
                    ->data(Qt::UserRole + 1)
                    .toBool();
    viewFile_->setEnabled(idle && oneFile);
    editFile_->setEnabled(
        idle && oneFile &&
        (state_ == State::Filesystem ||
         (state_ == State::Viewing && !info_.encrypted && archiveParents_.empty())));
    split_->setEnabled(idle && state_ == State::Filesystem && oneFile &&
                       !QFileInfo(selectedFilesystemPaths().value(0)).isSymLink());
    combine_->setEnabled(split_->isEnabled() &&
                         QRegularExpression(R"(\.0{2,}1$)")
                             .match(QFileInfo(selectedFilesystemPaths().value(0)).fileName())
                             .hasMatch());
    rename_->setEnabled(idle && one &&
                        (state_ == State::Filesystem ||
                         (state_ == State::Viewing && !info_.encrypted && archiveParents_.empty() &&
                          !table_->item(table_->selectionModel()->selectedRows().first().row(), 0)
                               ->data(Qt::UserRole + 1)
                               .toBool())));
    properties_->setEnabled(idle && selected);
    table_->setEnabled(idle);
    iconList_->setEnabled(idle);
    table_->setDragEnabled(idle && state_ == State::Filesystem);
    iconList_->setDragEnabled(idle && state_ == State::Filesystem);
    path_->setReadOnly(!navigating);
}

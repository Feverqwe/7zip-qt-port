#include "CPP/7zip/UI/FileManager/Panel.h"
#include "mainwindow.h"
#include "qtlocationfolder.h"

void MainWindow::showItemContextMenu(const QPoint &globalPos) {
    QMenu menu(this);
    const bool hasSelection = !table_->selectionModel()->selectedRows().isEmpty();
    if (hasSelection) {
        for (QAction *action : {openItem_, openInside_, openOutside_, viewFile_, editFile_, rename_,
                                copy_, move_, remove_, split_, combine_})
            menu.addAction(action);
        menu.addSeparator();
        menu.addAction(properties_);
        menu.addSeparator();
    }
    if (state_ == State::Filesystem) {
        menu.addAction(create_);
        menu.addAction(createFolder_);
        menu.addAction(createFile_);
    } else {
        menu.addAction(add_);
        if (hasSelection)
            menu.addAction(selected_);
        menu.addAction(extract_);
        menu.addAction(test_);
        menu.addAction(infoAction_);
    }
    menu.addSeparator();
    menu.addAction(refresh_);
    menu.exec(globalPos);
}

void MainWindow::setSortMode(SortMode mode) {
    const int columns[] = {0, 5, 4, 2, 6};
    const int column = columns[static_cast<int>(mode)];
    if (column == sortColumn_)
        sortOrder_ = sortOrder_ == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder;
    else
        sortOrder_ = column == 2 || column == 4 ? Qt::DescendingOrder : Qt::AscendingOrder;
    sortColumn_ = column;
    sortMode_ = mode;
    sortActions_[static_cast<int>(mode)]->setChecked(true);
    applySortMode();
}

void MainWindow::applySortMode() {
    if (!table_)
        return;
    table_->sortItems(sortColumn_, sortOrder_);
}

QIcon MainWindow::toolbarIcon(int kind) {
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::black, 1));
    if (kind == 0) {
        painter.setBrush(QColor("#28dc11"));
        painter.drawPolygon(QPolygonF{{10, 3},
                                      {14, 3},
                                      {14, 10},
                                      {21, 10},
                                      {21, 14},
                                      {14, 14},
                                      {14, 21},
                                      {10, 21},
                                      {10, 14},
                                      {3, 14},
                                      {3, 10},
                                      {10, 10}});
    } else if (kind == 1) {
        painter.setBrush(QColor("#0099ff"));
        painter.drawRect(3, 10, 18, 5);
    } else if (kind == 2) {
        painter.setBrush(QColor("#22e5ce"));
        painter.drawPolygon(QPolygonF{{2, 11}, {7, 8}, {11, 13}, {19, 4}, {22, 7}, {11, 20}});
    } else if (kind == 3 || kind == 4) {
        painter.setBrush(kind == 3 ? QColor("#21d426") : QColor("#2367e6"));
        painter.drawPolygon(
            QPolygonF{{5, 9}, {13, 9}, {13, 4}, {22, 12}, {13, 20}, {13, 15}, {5, 15}});
        if (kind == 3) {
            painter.setBrush(QColor("#ae42df"));
            painter.drawRect(2, 10, 3, 4);
        }
    } else if (kind == 5) {
        painter.setBrush(QColor("#ef2834"));
        painter.drawPolygon(QPolygonF{{5, 3},
                                      {12, 9},
                                      {19, 3},
                                      {22, 6},
                                      {15, 12},
                                      {22, 18},
                                      {19, 21},
                                      {12, 15},
                                      {5, 21},
                                      {2, 18},
                                      {9, 12},
                                      {2, 6}});
    } else {
        painter.setBrush(QColor("#f7e51e"));
        painter.drawEllipse(QRectF(10, 2, 4, 4));
        painter.drawRect(QRectF(10, 9, 4, 12));
    }
    return QIcon(pixmap);
}

QString MainWindow::toolbarSettingsPath() { return FmSettingsPath(); }

void MainWindow::recordVisitedPath(const QString &path) {
    if (path.isEmpty())
        return;
    app_.AppState.FolderHistory.AddString(ToUString(path));
    app_.AppState.FolderHistory.Save(toolbarSettingsPath());
    refreshPathHistory();
}

void MainWindow::refreshPathHistory() {
    const QString current = path_->text();
    UStringVector paths;
    app_.AppState.FolderHistory.GetList(paths);
    const QSignalBlocker blocker(pathCombo_);
    pathCombo_->clear();
    FOR_VECTOR(i, paths)
    pathCombo_->addItem(ToQString(paths[i]));
    path_->setText(current);
}

void MainWindow::showFoldersHistory() {
    if (state_ == State::Loading || state_ == State::Operating)
        return;
    const QString selected =
        CPanel::FoldersHistory(app_.AppState.FolderHistory, toolbarSettingsPath(), this);
    refreshPathHistory();
    if (!selected.isNull())
        navigateTo(selected);
    else
        refreshActions();
}
void MainWindow::refreshBookmarks() {
    for (int index = 0; index < 10; ++index) {
        const QString path = ToQString(app_.AppState.FastFolders.GetString((unsigned)index));
        const QString label = path.isEmpty()
                                  ? QString::number(index) + QStringLiteral(" — пусто")
                                  : QString::number(index) + QStringLiteral(" — ") + path;
        openBookmarks_[index]->setText(label);
        openBookmarks_[index]->setToolTip(label);
        openBookmarks_[index]->setEnabled(!path.isEmpty() && state_ != State::Loading &&
                                          state_ != State::Operating);
        saveBookmarks_[index]->setEnabled(state_ != State::Loading && state_ != State::Operating &&
                                          !isNestedArchive());
    }
}

void MainWindow::saveBookmark(int index) {
    if (index < 0 || index >= 10 || (state_ != State::Filesystem && state_ != State::Viewing) ||
        isNestedArchive())
        return;
    CPanel::SetBookmark(app_.AppState.FastFolders, (unsigned)index,
                        ToUString(state_ == State::Filesystem ? directory_ : archiveDisplayPath()));
    app_.AppState.FastFolders.Save(toolbarSettingsPath());
    refreshBookmarks();
}

void MainWindow::openBookmark(int index) {
    if (index < 0 || index >= 10 || state_ == State::Loading || state_ == State::Operating)
        return;
    const QString path =
        ToQString(CPanel::OpenBookmark(app_.AppState.FastFolders, (unsigned)index));
    if (path.isEmpty())
        return;
    navigateTo(path);
}

void MainWindow::updateToolbar() {
    auto *toolbar = findChild<QToolBar *>("archiveToolbar");
    const bool archive = archiveToolbarOption_->isChecked();
    const bool standard = standardToolbarOption_->isChecked();
    for (QAction *action : {toolbarAdd_, extract_, test_})
        toolbar->widgetForAction(action)->setVisible(archive);
    for (QAction *action : {copy_, move_, remove_, infoAction_})
        toolbar->widgetForAction(action)->setVisible(standard);
    toolbar->widgetForAction(toolbarGroupSeparator_)->setVisible(archive && standard);
    toolbar->setVisible(archive || standard);
    toolbar->setIconSize(largeButtonsOption_->isChecked() ? QSize(48, 36) : QSize(24, 24));
    toolbar->setToolButtonStyle(buttonTextOption_->isChecked() ? Qt::ToolButtonTextUnderIcon
                                                               : Qt::ToolButtonIconOnly);
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    settings.setValue("toolbar/archive", archive);
    settings.setValue("toolbar/standard", standard);
    settings.setValue("toolbar/large", largeButtonsOption_->isChecked());
    settings.setValue("toolbar/text", buttonTextOption_->isChecked());
}

void MainWindow::showSettings() {
    SettingsDialog dialog(toolbarSettingsPath(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    fileListSettings_ = dialog.settings();
    app_.SetListSettings();
    if (state_ == State::Filesystem)
        updateFilesystemIcons();
}

void MainWindow::updateFilesystemIcons() {
    QFileIconProvider icons;
    for (int row = 0; row < table_->rowCount(); ++row) {
        const QFileInfo file(table_->item(row, 1)->text());
        table_->item(row, 0)->setIcon(
            fileListSettings_.ShowRealFileIcons
                ? icons.icon(file)
                : QApplication::style()->standardIcon(file.isDir() ? QStyle::SP_DirIcon
                                                                   : QStyle::SP_FileIcon));
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    // FM.cpp WM_SETFOCUS delegates to the application owner.
    if (watched == this && event->type() == QEvent::FocusIn) {
        app_.SetFocusToLastItem();
        return true;
    }
    if ((watched == table_->viewport() || (iconList_ && watched == iconList_->viewport())) &&
        (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove)) {
        auto *drag = static_cast<QDragMoveEvent *>(event);
        if ((state_ == State::Filesystem || (state_ == State::Viewing && !info_.encrypted)) &&
            !drag->mimeData()->urls().isEmpty() && !backend_->busy()) {
            drag->setDropAction(Qt::CopyAction);
            drag->accept();
            return true;
        }
    }
    if ((watched == table_->viewport() || (iconList_ && watched == iconList_->viewport())) &&
        event->type() == QEvent::Drop) {
        auto *drop = static_cast<QDropEvent *>(event);
        QStringList paths;
        for (const QUrl &url : drop->mimeData()->urls()) {
            if (!url.isLocalFile())
                return true;
            paths << url.toLocalFile();
        }
        if (paths.isEmpty())
            return true;
        if (state_ == State::Filesystem)
            transferFiles(paths, directory_, false);
        else if (state_ == State::Viewing && !info_.encrypted)
            addSources(paths);
        drop->setDropAction(Qt::CopyAction);
        drop->accept();
        return true;
    }
    if (event->type() == QEvent::KeyPress &&
        handlePanelKey(watched, static_cast<QKeyEvent *>(event)))
        return true;
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setViewMode(int mode) {
    if (mode < 0 || mode > 3 || !viewStack_)
        return;
    viewMode_ = mode;
    app_.GetFocusedPanel()._listViewMode = static_cast<UInt32>(mode);
    viewActions_[mode]->setChecked(true);
    if (mode != 3) {
        iconList_->setViewMode(mode == 2 ? QListView::ListMode : QListView::IconMode);
        iconList_->setIconSize(mode == 0 ? QSize(48, 48) : QSize(16, 16));
        iconList_->setGridSize(mode == 0 ? QSize(140, 82) : mode == 1 ? QSize(180, 26) : QSize());
        iconList_->setFlow(mode == 2 ? QListView::TopToBottom : QListView::LeftToRight);
        iconList_->setWrapping(true);
        iconList_->setMovement(QListView::Static);
    }
    viewStack_->setCurrentIndex(mode == 3 ? 0 : 1);
    app_.GetFocusedPanel().AttachPresentation(viewStack_->currentWidget(), pathCombo_);
    CListMode modes;
    for (unsigned i = 0; i < kNumPanelsMax; ++i)
        modes.Panels[i] = app_.Panels[i].GetListViewMode();
    modes.Save(toolbarSettingsPath());
}

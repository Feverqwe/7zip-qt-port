#include "mainwindow.h"

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
    sortMode_ = mode;
    sortActions_[static_cast<int>(mode)]->setChecked(true);
    applySortMode();
}

void MainWindow::applySortMode() {
    if (!table_)
        return;
    const int columns[] = {0, 5, 4, 2, 6};
    table_->sortItems(columns[static_cast<int>(sortMode_)], Qt::AscendingOrder);
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

QString MainWindow::toolbarSettingsPath() {
    const QString overridden = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
    if (!overridden.isEmpty())
        return overridden;
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(directory);
    return QDir(directory).filePath("toolbar.ini");
}

void MainWindow::recordVisitedPath(const QString &path) {
    if (path.isEmpty())
        return;
    const int existing = pathCombo_->findText(path, Qt::MatchExactly);
    if (existing >= 0)
        pathCombo_->removeItem(existing);
    pathCombo_->insertItem(0, path);
    while (pathCombo_->count() > 20)
        pathCombo_->removeItem(pathCombo_->count() - 1);
    QStringList paths;
    for (int index = 0; index < pathCombo_->count(); ++index)
        paths << pathCombo_->itemText(index);
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    settings.setValue("history/paths", paths);
}

void MainWindow::showFoldersHistory() {
    if (state_ == State::Loading || state_ == State::Operating)
        return;
    QDialog dialog(this);
    dialog.setObjectName("foldersHistoryDialog");
    dialog.setWindowTitle("История папок");
    dialog.resize(540, 360);
    auto *layout = new QVBoxLayout(&dialog);
    auto *list = new QListWidget(&dialog);
    list->setObjectName("foldersHistoryList");
    for (int index = 0; index < pathCombo_->count(); ++index)
        list->addItem(pathCombo_->itemText(index));
    if (list->count() > 0)
        list->setCurrentRow(0);
    layout->addWidget(list);
    auto *remove = new QPushButton("Удалить", &dialog);
    remove->setObjectName("removeHistoryEntryButton");
    layout->addWidget(remove);
    connect(remove, &QPushButton::clicked, &dialog,
            [list] { delete list->takeItem(list->currentRow()); });
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(list, &QListWidget::itemDoubleClicked, &dialog, &QDialog::accept);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString selected = list->currentItem() ? list->currentItem()->text() : QString();
    const QString current = path_->text();
    pathCombo_->clear();
    for (int index = 0; index < list->count(); ++index)
        pathCombo_->addItem(list->item(index)->text());
    path_->setText(current);
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    QStringList paths;
    for (int index = 0; index < pathCombo_->count(); ++index)
        paths << pathCombo_->itemText(index);
    settings.setValue("history/paths", paths);
    if (!selected.isEmpty())
        navigateTo(selected);
    else
        refreshActions();
}
void MainWindow::refreshBookmarks() {
    for (int index = 0; index < 10; ++index) {
        const Bookmark &bookmark = bookmarks_[index];
        const QString label =
            bookmark.path.isEmpty()
                ? QString::number(index) + QStringLiteral(" — пусто")
                : QString::number(index) + QStringLiteral(" — ") + bookmark.path +
                      (bookmark.folder.isEmpty() ? QString()
                                                 : QStringLiteral(" / ") + bookmark.folder);
        openBookmarks_[index]->setText(label);
        openBookmarks_[index]->setToolTip(label);
        openBookmarks_[index]->setEnabled(!bookmark.path.isEmpty() && state_ != State::Loading &&
                                          state_ != State::Operating);
        saveBookmarks_[index]->setEnabled(state_ != State::Loading && state_ != State::Operating &&
                                          archiveParents_.empty());
    }
}

void MainWindow::saveBookmark(int index) {
    if ((state_ != State::Filesystem && state_ != State::Viewing) || !archiveParents_.empty())
        return;
    Bookmark &bookmark = bookmarks_[index];
    bookmark.path = state_ == State::Filesystem ? directory_ : archive_;
    bookmark.folder = state_ == State::Viewing ? archiveFolder_ : QString();
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    const QString key = QStringLiteral("favorites/%1/").arg(index);
    settings.setValue(key + "path", bookmark.path);
    settings.setValue(key + "folder", bookmark.folder);
    refreshBookmarks();
}

void MainWindow::openBookmark(int index) {
    if (state_ == State::Loading || state_ == State::Operating)
        return;
    const Bookmark bookmark = bookmarks_[index];
    if (bookmark.path.isEmpty())
        return;
    const QFileInfo info(bookmark.path);
    if (bookmark.folder.isEmpty() && info.isDir()) {
        showDirectory(bookmark.path, true);
        return;
    }
    if (!info.isFile() || info.isSymLink()) {
        statusBar()->showMessage("Путь закладки недоступен: " + bookmark.path);
        return;
    }
    if (state_ == State::Viewing && archive_ == info.absoluteFilePath()) {
        if (bookmark.folder.isEmpty() || archiveFolderExists(bookmark.folder))
            showArchiveFolder(bookmark.folder);
        else
            statusBar()->showMessage("Папка закладки отсутствует: " + bookmark.folder);
        return;
    }
    pendingBookmarkFolder_ = bookmark.folder;
    openArchive(bookmark.path);
}

bool MainWindow::archiveFolderExists(const QString &folder) const {
    for (const ArchiveEntry &entry : entries_) {
        QString path = entry.path;
        if (path.endsWith('/'))
            path.chop(1);
        if ((path == folder && entry.directory) || path.startsWith(folder + '/'))
            return true;
    }
    return false;
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
    SettingsDialog dialog(fileListSettings_, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    fileListSettings_ = dialog.settings();
    QSettings settings(toolbarSettingsPath(), QSettings::IniFormat);
    settings.setValue("list/realFileIcons", fileListSettings_.realFileIcons);
    settings.setValue("list/grid", fileListSettings_.grid);
    settings.setValue("list/singleClick", fileListSettings_.singleClick);
    table_->setShowGrid(fileListSettings_.grid);
    if (state_ == State::Filesystem)
        updateFilesystemIcons();
}

void MainWindow::updateFilesystemIcons() {
    QFileIconProvider icons;
    for (int row = 0; row < table_->rowCount(); ++row) {
        const QFileInfo file(table_->item(row, 1)->text());
        table_->item(row, 0)->setIcon(
            fileListSettings_.realFileIcons
                ? icons.icon(file)
                : QApplication::style()->standardIcon(file.isDir() ? QStyle::SP_DirIcon
                                                                   : QStyle::SP_FileIcon));
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
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
    if ((watched == table_ || watched == table_->viewport() || watched == iconList_ ||
         (iconList_ && watched == iconList_->viewport())) &&
        event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() >= Qt::Key_0 && key->key() <= Qt::Key_9 &&
            key->modifiers() == Qt::AltModifier) {
            openBookmark(key->key() - Qt::Key_0);
            return true;
        }
        if (key->key() >= Qt::Key_0 && key->key() <= Qt::Key_9 &&
            key->modifiers() == (Qt::AltModifier | Qt::ShiftModifier)) {
            saveBookmark(key->key() - Qt::Key_0);
            return true;
        }
        if (!(key->modifiers() &
              (Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier))) {
            if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) &&
                openItem_->isEnabled()) {
                openItem_->trigger();
                return true;
            }
            if (key->key() == Qt::Key_Backspace && parent_->isEnabled()) {
                parent_->trigger();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

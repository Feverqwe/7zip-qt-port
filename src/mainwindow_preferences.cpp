#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/MyLoadMenu.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "CPP/7zip/UI/FileManager/SysIconUtils.h"
#include "mainwindow.h"
#include "qtdialogscreenservices.h"
#include "qtfilemanagershellmenu.h"
#include "qtfilemenuprojection.h"
#include "qtlanguageservices.h"
#include "qtlocationfolder.h"
#include "qtprogramfilemenu.h"
#include <QBitmap>
#include <QMouseEvent>

void MainWindow::showItemContextMenu(const QPoint &globalPos) {
    QMenu menu(this);
    QtFileManagerShellMenu shell(*this, menu);
    menu.setObjectName("panelContextMenu");
    QtFileMenuProjection projection;
    ReadRegDiff(projection.diffPath, toolbarSettingsPath());
    ReadReg_VerCtrlPath(projection.versionControlPath, toolbarSettingsPath());
    QtDialogScreenServices screen(this);
    projection.screenServices = &screen;
    QHash<unsigned, QAction *> actions;
    for (QAction *action : programFileMenu_->SourceActions()) {
        const bool separator = action->isSeparator();
        const unsigned id = action->menu() ? 0u : action->property("nativeCommandID").toUInt();
        if (id < IDM_VER_EDIT || id > IDM_VER_DIFF)
            projection.sourceItems.append({id, ToUString(action->text()), separator});
        if (!separator && !action->menu())
            actions.insert(id, action);
    }
    for (QAction *action : programFileMenu_->SourceActions()) {
        const unsigned id = action->property("nativeCommandID").toUInt();
        if (id >= IDM_VER_EDIT && id <= IDM_VER_DIFF)
            actions.insert(id, action);
    }
    sourceOperatedIndices(); // Synchronize the Qt selection before source menu construction.
    app_.GetFocusedPanel().CreateFileMenu(projection, false, &shell);
    for (const auto &entry : projection.items) {
        if (entry.source.IsSeparator()) {
            menu.addSeparator();
            continue;
        }
        if (entry.source.wID == 0) {
            auto *hash = menu.addMenu(QStringLiteral("CRC"));
            hash->setObjectName("contextHashMenu");
            for (QAction *action : findChild<QMenu *>("hashMenu")->actions())
                hash->addAction(action);
            continue;
        }
        QAction *action = actions.value(entry.source.wID, nullptr);
        if (!action)
            continue;
        menu.addAction(action);
        action->setEnabled(action->isEnabled() && entry.enabled);
    }
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
    auto &panel = app_.GetFocusedPanel();
    panel._sortID = static_cast<FileTable *>(table_)->propertyId(sortColumn_);
    panel._ascending = sortOrder_ == Qt::AscendingOrder;
    table_->sortItems(sortColumn_, sortOrder_);
    saveColumns();
}

QIcon MainWindow::toolbarIcon(int kind) {
    // App.cpp's g_ArchiveButtons/g_StandardButtons use the large bitmap first
    // and the small *2 bitmap second. Magenta is the source image-list mask.
    static const char *const names[] = {"Add", "Extract", "Test", "Copy", "Move", "Delete", "Info"};
    if (kind < 0 || kind >= int(std::size(names)))
        return {};
    QIcon icon;
    for (const QString &suffix : {QStringLiteral("2"), QString()}) {
        QPixmap pixmap(QStringLiteral(":/upstream/7zip/UI/FileManager/%1%2.bmp")
                           .arg(QString::fromLatin1(names[kind]), suffix));
        if (pixmap.isNull())
            continue;
        pixmap.setMask(pixmap.createMaskFromColor(Qt::magenta, Qt::MaskInColor));
        icon.addPixmap(pixmap);
    }
    return icon;
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
void MainWindow::refreshBookmarks() { MyLoadBookmarks(openBookmarks_, saveBookmarks_, app_); }

void MainWindow::saveBookmark(int index) {
    if (index < 0 || index >= 10 || (state_ != State::Filesystem && state_ != State::Viewing) ||
        isNestedArchive())
        return;
    CPanel::SetBookmark(app_.AppState.FastFolders, (unsigned)index,
                        ToUString(state_ == State::Filesystem ? directory_ : archiveDisplayPath()));
    app_.AppState.FastFolders.Save(toolbarSettingsPath());
    refreshActions();
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
    const bool archive = app_.ShowArchiveToolbar;
    const bool standard = app_.ShowStandardToolbar;
    const QSignalBlocker archiveSignals(archiveToolbarOption_);
    const QSignalBlocker standardSignals(standardToolbarOption_);
    const QSignalBlocker largeSignals(largeButtonsOption_);
    const QSignalBlocker textSignals(buttonTextOption_);
    archiveToolbarOption_->setChecked(archive);
    standardToolbarOption_->setChecked(standard);
    largeButtonsOption_->setChecked(app_.LargeButtons);
    buttonTextOption_->setChecked(app_.ShowButtonsLables);
    for (QAction *action : {toolbarAdd_, extract_, test_})
        toolbar->widgetForAction(action)->setVisible(archive);
    for (QAction *action : {copy_, move_, remove_, infoAction_})
        toolbar->widgetForAction(action)->setVisible(standard);
    toolbar->widgetForAction(toolbarGroupSeparator_)->setVisible(archive && standard);
    toolbar->setVisible(archive || standard);
    toolbar->setIconSize(largeButtonsOption_->isChecked() ? QSize(48, 36) : QSize(24, 24));
    toolbar->setToolButtonStyle(buttonTextOption_->isChecked() ? Qt::ToolButtonTextUnderIcon
                                                               : Qt::ToolButtonIconOnly);
}

void MainWindow::showSettings() {
    SettingsDialog dialog(toolbarSettingsPath(), this, folderManager_);
    dialog.Applied = [this](const CFmSettings &settings) { applyFileListSettings(settings); };
    dialog.exec();
}

void MainWindow::applyFileListSettings(const CFmSettings &settings) {
    const bool showDotsChanged = fileListSettings_.ShowDots != settings.ShowDots;
    fileListSettings_ = settings;
    app_.SetListSettings();
    app_.RefreshAllPanels();
    refreshActions();
    if (showDotsChanged)
        updateParentEntry();
    if (state_ == State::Filesystem)
        updateFilesystemIcons();
}

void MainWindow::updateFilesystemIcons() {
    for (int row = 0; row < table_->rowCount(); ++row) {
        const QFileInfo file(table_->item(row, 1)->text());
        table_->item(row, 0)->setIcon(
            fileListSettings_.ShowRealFileIcons
                ? g_Ext_to_Icon_Map.GetIcon(file.isDir() ? FILE_ATTRIBUTE_DIRECTORY
                                                         : FILE_ATTRIBUTE_ARCHIVE,
                                            file.fileName())
                : QApplication::style()->standardIcon(file.isDir() ? QStyle::SP_DirIcon
                                                                   : QStyle::SP_FileIcon));
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::FocusIn || event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::KeyPress) {
        const unsigned panelIndex = panelIndexFor(watched);
        if (panelIndex < kNumPanelsMax && panelIndex != projectedPanel_ && app_.NumPanels == 2)
            activatePanelUi(panelIndex);
    }
    // FM.cpp WM_SETFOCUS delegates to the application owner.
    if (watched == this && event->type() == QEvent::FocusIn) {
        app_.SetFocusToLastItem();
        return true;
    }
    const bool listViewport = (table_ && watched == table_->viewport()) ||
                              (iconList_ && watched == iconList_->viewport());
    if (listViewport && event->type() == QEvent::MouseButtonPress) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        selectionClickModifiers_ = mouse->modifiers();
        selectionClickLeft_ = mouse->button() == Qt::LeftButton;
    }
    if (event->type() == QEvent::KeyPress &&
        handlePanelKey(watched, static_cast<QKeyEvent *>(event)))
        return true;
    if (event->type() == QEvent::KeyRelease &&
        static_cast<QKeyEvent *>(event)->key() == Qt::Key_Shift)
        selectionShiftSequence_ = false;
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
        // Qt's Static movement resets dragEnabled. The shared file-list policy
        // still permits exporting entries through the native drag controller.
        iconList_->setDragEnabled(table_->dragEnabled());
    }
    viewStack_->setCurrentIndex(mode == 3 ? 0 : 1);
    app_.GetFocusedPanel().AttachPresentation(viewStack_->currentWidget(), pathCombo_);
    CListMode modes;
    for (unsigned i = 0; i < kNumPanelsMax; ++i)
        modes.Panels[i] = app_.Panels[i].GetListViewMode();
    modes.Save(toolbarSettingsPath());
}

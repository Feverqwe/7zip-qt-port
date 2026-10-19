#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/MyLoadMenu.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "CPP/7zip/UI/FileManager/SysIconUtils.h"
#include "mainwindow.h"
#include "qtapppresentation.h"
#include "qtcommandicons.h"
#include "qtlanguageservices.h"
#include "qtlocationfolder.h"
#include "qtmainwindowchrome.h"
#include "qtpanelcontextmenu.h"
#include <QMouseEvent>
#include <QSignalBlocker>
#include <QToolButton>

namespace {
QString SavedSourcePath(QString path) {
    // Earlier Qt settings used a display separator after the real archive.
    // Convert only that boundary; spaces inside source path components survive.
    const QString separator = QStringLiteral(" / ");
    for (int pos = path.indexOf(separator); pos >= 0;
         pos = path.indexOf(separator, pos + separator.size()))
        if (QFileInfo(path.left(pos)).isFile()) {
            path.replace(pos, separator.size(), QStringLiteral("/"));
            break;
        }
    return path;
}
} // namespace

void MainWindow::showItemContextMenu(unsigned owner, QAbstractItemView *view, const QPoint &pos) {
    withPanelProjection(owner, [this, view, pos] {
        const bool keyboard = pos == QPoint(-1, -1);
        if (!keyboard) {
            const QModelIndex index = view->indexAt(pos);
            if (index.isValid() &&
                !view->selectionModel()->isRowSelected(index.row(), QModelIndex())) {
                view->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect |
                                                          QItemSelectionModel::Rows);
                view->setCurrentIndex(index);
            }
        }
        sourceOperatedIndices();
        refreshActions();
        const QPoint point = keyboard ? pos : view->viewport()->mapToGlobal(pos);
        QtPanelContextMenu menu(*this, *view);
        app_.GetFocusedPanel().OnContextMenu(view, point.x(), point.y(), menu);
    });
}

void MainWindow::applySortMode(bool enableSorting) {
    if (!table_)
        return;
    auto &panel = app_.GetFocusedPanel();
    panel._sortID = static_cast<FileTable *>(table_)->propertyId(sortColumn_);
    panel._ascending = sortOrder_ == Qt::AscendingOrder;
    panel.SetSortRawStatus();
    if (enableSorting && !table_->isSortingEnabled()) {
        // Enabling Qt sorting already sorts; configure the source order first.
        const QSignalBlocker blocker(table_->horizontalHeader());
        table_->horizontalHeader()->setSortIndicator(sortColumn_, sortOrder_);
        table_->setSortingEnabled(true);
    } else
        table_->sortItems(sortColumn_, sortOrder_);
    saveColumns();
}

void MainWindow::sortIndicatorChanged(unsigned index, int column, Qt::SortOrder order) {
    const bool active = index == projectedPanel_;
    withPanelProjection(index, [this, index, column, order, active] {
        auto *view = static_cast<FileTable *>(table_);
        if (view->deferSourceColumnClick(
                [window = QPointer<MainWindow>(this), index, column, order] {
                    if (window)
                        window->sortIndicatorChanged(index, column, order);
                }))
            return;
        sortColumn_ = column;
        sortOrder_ = order;
        auto &panel = app_.GetFocusedPanel();
        {
            const QSignalBlocker blocked(table_->horizontalHeader());
            if (view->takeSourceColumnClick()) {
                panel.OnColumnClick(view->propertyId(column));
                sortOrder_ = panel._ascending ? Qt::AscendingOrder : Qt::DescendingOrder;
            } else {
                // Explicit Qt sort requests retain their requested direction.
                panel._sortID = view->propertyId(column);
                panel._ascending = order == Qt::AscendingOrder;
                panel.SetSortRawStatus();
            }
            // Qt's initial automatic sort precedes this signal consumer. Project
            // the authoritative source flag/order onto the clicked section.
            table_->sortItems(column, sortOrder_);
        }
        if (!configuringColumns_) {
            const PROPID property = view->propertyId(column);
            const int mode = property == kpidExtension    ? 1
                             : property == kpidMTime      ? 2
                             : property == kpidSize       ? 3
                             : property == kpidNoProperty ? 4
                                                          : 0;
            sortMode_ = static_cast<SortMode>(mode);
            if (active && (property == kpidName || property == kpidSize || property == kpidMTime ||
                           property == kpidExtension || property == kpidNoProperty))
                sortActions_[mode]->setChecked(true);
            saveColumns();
        }
    });
}

QIcon MainWindow::toolbarIcon(int kind) {
    static const QtCommandIcon symbols[] = {
        QtCommandIcon::Add,  QtCommandIcon::Extract, QtCommandIcon::Test, QtCommandIcon::Copy,
        QtCommandIcon::Move, QtCommandIcon::Delete,  QtCommandIcon::Info};
    if (kind < 0 || kind >= int(std::size(symbols)))
        return {};
    return QtCommandSymbol(symbols[kind]);
}

QString MainWindow::toolbarSettingsPath() { return FmSettingsPath(); }

void MainWindow::recordVisitedPath(const QString &path) {
    app_.AppState.FolderHistory.AddString(ToUString(path));
    app_.AppState.FolderHistory.Save(toolbarSettingsPath());
}

void MainWindow::showFoldersHistory() {
    if (state_ == State::Loading || state_ == State::Operating)
        return;
    const QString selected =
        CPanel::FoldersHistory(app_.AppState.FolderHistory, toolbarSettingsPath(), this);
    if (!selected.isNull())
        navigateTo(SavedSourcePath(selected), true);
    else
        refreshActions();
}
void MainWindow::refreshBookmarks() { MyLoadBookmarks(openBookmarks_, saveBookmarks_, app_); }

void MainWindow::saveBookmark(int index) {
    if (index < 0 || index >= 10 || state_ == State::Loading || state_ == State::Operating)
        return;
    auto &panel = app_.GetFocusedPanel();
    panel.LoadFullPath();
    CPanel::SetBookmark(app_.AppState.FastFolders, (unsigned)index, panel.GetCurrentFolderPrefix());
    app_.AppState.FastFolders.Save(toolbarSettingsPath());
    refreshActions();
}

void MainWindow::openBookmark(int index) {
    if (index < 0 || index >= 10 || state_ == State::Loading || state_ == State::Operating)
        return;
    const QString path =
        ToQString(CPanel::OpenBookmark(app_.AppState.FastFolders, (unsigned)index));
    navigateTo(SavedSourcePath(path), true);
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
    toolbar->widgetForAction(toolbarGroupSeparator_)->setStyle(toolbar->style());
    toolbar->setVisible(archive || standard);
    toolbar->setIconSize(largeButtonsOption_->isChecked() ? QSize(24, 24) : QSize(16, 16));
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    for (QAction *action : {toolbarAdd_, extract_, test_, copy_, move_, remove_, infoAction_}) {
        if (auto *button = qobject_cast<QToolButton *>(toolbar->widgetForAction(action))) {
            QtConfigureMainToolbarButton(*button);
            const bool primary = action == toolbarAdd_ || action == extract_;
            button->setToolButtonStyle(primary && app_.ShowButtonsLables
                                           ? Qt::ToolButtonTextBesideIcon
                                           : Qt::ToolButtonIconOnly);
            button->setAutoRaise(true);
            button->setFocusPolicy(Qt::StrongFocus);
        }
    }
    refreshToolbarTexts();
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
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
        updatePanelModifier(*static_cast<QKeyEvent *>(event));
    if (event->type() == QEvent::WindowDeactivate)
        rightControlPressed_ = false;
    if (appPresentation_ &&
        (event->type() == QEvent::KeyPress || event->type() == QEvent::MouseButtonPress ||
         event->type() == QEvent::MouseButtonDblClick))
        appPresentation_->CancelInitialFocus();
    if (!panelProjectionDepth_ &&
        (event->type() == QEvent::FocusIn || event->type() == QEvent::MouseButtonPress ||
         event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::KeyPress)) {
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
    if (listViewport && (event->type() == QEvent::MouseButtonPress ||
                         event->type() == QEvent::MouseButtonDblClick)) {
        const auto *mouse = static_cast<QMouseEvent *>(event);
        selectionClickModifiers_ = mouse->modifiers();
        selectionClickLeft_ = mouse->button() == Qt::LeftButton;
    }
    const bool listControl = watched == table_ || watched == iconList_ || listViewport;
    if ((watched == path_ || watched == pathCombo_) && event->type() == QEvent::ShortcutOverride) {
        const auto *key = static_cast<QKeyEvent *>(event);
        const auto mods = key->modifiers();
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter ||
            key->key() == Qt::Key_Escape) {
            event->accept();
            return true;
        }
        // Deliver the edit subclass's keys before application QAction shortcuts.
        if (!(mods & (Qt::MetaModifier | Qt::AltModifier)) &&
            ((key->key() == Qt::Key_W && mods.testFlag(Qt::ControlModifier)) ||
             (key->key() == Qt::Key_F9 && mods == Qt::NoModifier))) {
            event->accept();
            return true;
        }
    }
    if (listControl && !isPanelEditorFocused() && event->type() == QEvent::ShortcutOverride) {
        const auto *key = static_cast<QKeyEvent *>(event);
        if (overridePanelKey(*key)) {
            event->accept();
            return true;
        }
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
    auto &panel = app_.GetFocusedPanel();
    auto *stack = viewStack_;
    auto *address = pathCombo_;
    const QWidget *focused = QApplication::focusWidget();
    const QWidget *previousView = stack->currentWidget();
    const bool restoreFocus =
        previousView && focused && (focused == previousView || previousView->isAncestorOf(focused));
    viewMode_ = mode;
    viewActions_[mode]->setChecked(true);
    if (mode != 3) {
        static_cast<FileIconList *>(iconList_)->SetListViewMode(mode);
        // Qt's Static movement resets dragEnabled. The shared file-list policy
        // still permits exporting entries through the native drag controller.
        iconList_->setDragEnabled(table_->dragEnabled());
    }
    // Upstream changes the style of the same list control. Qt replaces the
    // visible view, which can synchronously focus another panel while hiding
    // the old widget. Bind the captured source owner and retain its list focus.
    stack->setCurrentIndex(mode == 3 ? 0 : 1);
    panel.AttachPresentation(stack->currentWidget(), address);
    if (restoreFocus)
        stack->currentWidget()->setFocus();
    CListMode modes;
    for (unsigned i = 0; i < kNumPanelsMax; ++i)
        modes.Panels[i] = app_.Panels[i].GetListViewMode();
    modes.Save(toolbarSettingsPath());
}

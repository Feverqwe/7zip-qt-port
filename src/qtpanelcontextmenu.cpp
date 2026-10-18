#include "qtpanelcontextmenu.h"
#include "7zip/UI/FileManager/resource.h"
#include "mainwindow.h"
#include "qtdialogscreenservices.h"
#include "qtfilemanagershellmenu.h"
#include "qtfilemenuprojection.h"
#include "qtlangcontrols.h"
#include "qtlanguageservices.h"
#include "qtprogramfilemenu.h"
#include <QScopedValueRollback>

struct QtPanelContextMenu::Impl {
    MainWindow &window;
    QAbstractItemView &view;
    QtAppOperationState::CallScope call;
    QMenu popup;
    QtDialogScreenServices screen;
    QtFileMenuProjection projection;
    QtFileManagerShellMenu shell;
    QHash<unsigned, QAction *> actions;
    unsigned pluginID = 0;
    Impl(MainWindow &owner, QAbstractItemView &control)
        : window(owner), view(control), call(owner.app_.QtOperations()), popup(&owner),
          screen(&owner), shell(owner, popup, [this](unsigned id) { pluginID = id; }) {
        popup.setObjectName("panelContextMenu");
    }
};

QtPanelContextMenu::QtPanelContextMenu(MainWindow &window, QAbstractItemView &view)
    : impl_(std::make_unique<Impl>(window, view)) {}
QtPanelContextMenu::~QtPanelContextMenu() {
    auto &window = impl_->window;
    // Native popup focus events can project disabled actions while source
    // notification guards are held. Recompute after popup and CallScope end.
    impl_.reset();
    window.refreshActions();
}
bool QtPanelContextMenu::IsHeader(QWidget *handle) const {
    auto *table = qobject_cast<QTableWidget *>(&impl_->view);
    return table && handle == table->horizontalHeader();
}
bool QtPanelContextMenu::IsList(QWidget *handle) const { return handle == &impl_->view; }
void QtPanelContextMenu::ShowColumnsContextMenu(int x, int y) {
    auto *table = qobject_cast<QTableWidget *>(&impl_->view);
    // Some Qt platforms restore peer focus before QAction::toggled. Keep
    // the source column mutation on the header's captured owner until return.
    CPanel::CDisableNotify left(impl_->window.app_.Panels[0]);
    CPanel::CDisableNotify right(impl_->window.app_.Panels[1]);
    impl_->window.showColumnsMenu(table->horizontalHeader()->mapFromGlobal(QPoint(x, y)));
}
bool QtPanelContextMenu::GetItemRect(int index, QRect &rect) const {
    const QModelIndex item = impl_->view.model()->index(index, 0);
    if (!item.isValid())
        return false;
    rect = impl_->view.visualRect(item);
    return rect.isValid();
}
void QtPanelContextMenu::ClientToScreen(int &x, int &y) const {
    const QPoint point = impl_->view.viewport()->mapToGlobal(QPoint(x, y));
    x = point.x();
    y = point.y();
}
void QtPanelContextMenu::CreatePopup() {
    auto &state = *impl_;
    ReadRegDiff(state.projection.diffPath, MainWindow::toolbarSettingsPath());
    ReadReg_VerCtrlPath(state.projection.versionControlPath, MainWindow::toolbarSettingsPath());
    state.projection.screenServices = &state.screen;
    for (QAction *action : state.window.programFileMenu_->SourceActions()) {
        const bool separator = action->isSeparator();
        const unsigned id = action->menu() ? 0u : action->property("nativeCommandID").toUInt();
        if (id < IDM_VER_EDIT || id > IDM_VER_DIFF)
            state.projection.sourceItems.append({id, ToUString(action->text()), separator});
        if (!separator && !action->menu())
            state.actions.insert(id, action);
    }
}
IQtFileMenuServices &QtPanelContextMenu::Menu() { return impl_->projection; }
IQtPanelPluginMenuServices &QtPanelContextMenu::Plugins() { return impl_->shell; }

static QAction *AppendCommand(QMenu &menu, const QAction &source, bool enabled) {
    auto *action = menu.addAction(source.icon(), source.text());
    action->setObjectName(source.objectName());
    action->setProperty("nativeCommandID", source.property("nativeCommandID"));
    action->setShortcuts(source.shortcuts());
    action->setShortcutContext(Qt::WidgetShortcut);
    action->setCheckable(source.isCheckable());
    action->setChecked(source.isChecked());
    action->setVisible(source.isVisible());
    action->setEnabled(enabled);
    return action;
}

unsigned QtPanelContextMenu::Track(int x, int y) {
    auto &state = *impl_;
    // Native TrackPopupMenu owns separate rows and returns only a command ID.
    // Keep their state independent of the main-menu projection during popup
    // focus events, while source notification guards are held.
    for (const auto &entry : state.projection.items) {
        if (entry.source.IsSeparator()) {
            state.popup.addSeparator();
            continue;
        }
        if (entry.source.wID == 0) {
            auto *hash = state.popup.addMenu(QStringLiteral("CRC"));
            hash->setObjectName("contextHashMenu");
            QtBindLocalText(hash->menuAction(), "CommandMenus",
                            QT_TRANSLATE_NOOP("CommandMenus", "Checksums"));
            for (QAction *action : state.window.findChild<QMenu *>("hashMenu")->actions())
                AppendCommand(*hash, *action, action->isEnabled());
            continue;
        }
        if (QAction *action = state.actions.value(entry.source.wID)) {
            AppendCommand(state.popup, *action, action->isEnabled() && entry.enabled);
        }
    }
    QtClarifyOpenCommands(state.popup);
    QtGroupFileCommands(state.popup, true);
    // OnContextMenu dispatches the chosen ID after the popup guards restore.
    QAction *chosen;
    const unsigned owner = state.window.app_.GetFocusedPanelIndex();
    {
        // Closing a Qt popup restores the previously focused peer widget.
        // Its synchronous FocusIn must not retarget this source command.
        CPanel::CDisableNotify left(state.window.app_.Panels[0]);
        CPanel::CDisableNotify right(state.window.app_.Panels[1]);
        chosen = state.popup.exec(QPoint(x, y));
    }
    state.window.app_.SetFocusedPanel(owner);
    return state.pluginID ? state.pluginID
                          : (chosen ? chosen->property("nativeCommandID").toUInt() : 0u);
}
void QtPanelContextMenu::InvokePluginCommand(unsigned id) {
    auto &state = *impl_;
    state.shell.ResetResult();
    QScopedValueRollback<bool> progress(state.window.sourceArchiveProgress_, true);
    state.window.app_.GetFocusedPanel().InvokePluginCommand(id, state.shell);
    state.window.refreshActions();
}
bool QtPanelContextMenu::ExecuteFileCommand(unsigned id) {
    return impl_->window.app_.ExecuteCommand(id);
}

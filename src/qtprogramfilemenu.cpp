#include "qtprogramfilemenu.h"
#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/MyLoadMenu.h"
#include "mainwindow.h"
#include "qtdialogscreenservices.h"
#include "qtfilemanagershellmenu.h"
#include "qtfilemenuprojection.h"
#include "qtlangcontrols.h"
#include <QScopedValueRollback>

void QtClarifyOpenCommands(QMenu &menu) {
    for (QAction *action : menu.actions()) {
        const unsigned id = action->property("nativeCommandID").toUInt();
        switch (id) {
        case IDM_OPEN_INSIDE:
            QtBindLocalText(action, "qtprogramfilemenu",
                            QT_TRANSLATE_NOOP("qtprogramfilemenu", "Open as archive"));
            break;
        case IDM_OPEN_OUTSIDE:
            QtBindLocalText(action, "qtprogramfilemenu",
                            QT_TRANSLATE_NOOP("qtprogramfilemenu", "Open in external application"));
            break;
        case IDM_OPEN_INSIDE_ONE:
        case IDM_OPEN_INSIDE_PARSER:
            QtBindLocalText(
                action, "qtprogramfilemenu",
                id == IDM_OPEN_INSIDE_ONE
                    ? QT_TRANSLATE_NOOP("qtprogramfilemenu", "Single archive layer (*)")
                    : QT_TRANSLATE_NOOP("qtprogramfilemenu", "Parse file as archive (#)"));
            break;
        }
    }
}

QtProgramFileMenu::QtProgramFileMenu(MainWindow &window)
    : _window(window), _menu(*window.findChild<QMenu *>("fileMenu")) {
    _source = _menu.actions();
    _shell = std::make_unique<QtFileManagerShellMenu>(window, _menu, [this](unsigned id) {
        _window.app_.ExecuteCommand(id);
        _window.refreshActions();
    });
    QObject::connect(&_menu, &QMenu::aboutToShow, &_window, [this] { Activate(); });
}

QtProgramFileMenu::~QtProgramFileMenu() = default;

void QtProgramFileMenu::ClearDynamicRows() {
    for (QAction *action : _dynamic) {
        _menu.removeAction(action);
        if (action->menu())
            action->menu()->deleteLater();
        else
            action->deleteLater();
    }
    _dynamic.clear();
}

void QtProgramFileMenu::Activate() {
    auto &panel = _window.app_.GetFocusedPanel();
    // No folder/property access during source worker replacement or scanning.
    if (_window.app_.QtOperations().callActive() || _window.sourceArchiveProgress_ ||
        _window.scanningDisk_ || !panel.ShouldProcessNotifications()) {
        ReleasePluginMenus();
        return;
    }
    ClearDynamicRows();
    _window.sourceOperatedIndices();
    QtFileMenuProjection projection;
    ReadRegDiff(projection.diffPath, MainWindow::toolbarSettingsPath());
    ReadReg_VerCtrlPath(projection.versionControlPath, MainWindow::toolbarSettingsPath());
    QtDialogScreenServices screen(&_window);
    projection.screenServices = &screen;
    QHash<unsigned, QAction *> actions;
    for (QAction *action : _source) {
        const unsigned id = action->menu() ? 0 : action->property("nativeCommandID").toUInt();
        if (id < IDM_VER_EDIT || id > IDM_VER_DIFF)
            projection.sourceItems.append({id, ToUString(action->text()), action->isSeparator()});
        if (!action->isSeparator())
            actions.insert(id, action);
    }
    _panel = &panel;
    OnMenuActivating(*_window.menuBar(), _menu, 0, _window.app_, projection, *_shell);
    _dynamic = _menu.actions(); // Source plugin prefix only; static rows follow.
    for (const auto &item : projection.items) {
        if (item.source.IsSeparator()) {
            _dynamic.append(_menu.addSeparator());
            continue;
        }
        if (QAction *action = actions.value(item.source.wID)) {
            _menu.addAction(action);
            action->setEnabled(action->isEnabled() && item.enabled);
        }
    }
    QtClarifyOpenCommands(_menu);
}

bool QtProgramFileMenu::CanInvoke(unsigned id) const {
    return _panel && _panel == &_window.app_.GetFocusedPanel() &&
           _window.state_ == MainWindow::State::Filesystem && !_window.backend_->busy() &&
           !_window.sourceArchiveProgress_ && !_window.scanningDisk_ &&
           _panel->ShouldProcessNotifications() && _shell->CanInvoke(id);
}

void QtProgramFileMenu::InvokePluginCommand(unsigned id) {
    _shell->ResetResult();
    QScopedValueRollback<bool> progress(_window.sourceArchiveProgress_, true);
    _window.app_.GetFocusedPanel().InvokePluginCommand(id, *_shell);
}

void QtProgramFileMenu::ReleasePluginMenus() {
    _shell->ReleasePluginMenus();
    _panel = nullptr;
}

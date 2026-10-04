#include "qtnativeshellmenu.h"
#ifdef _WIN32
#include "CPP/7zip/UI/FileManager/App.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "Windows/Menu.h"
#include <QMenu>
#include <QSet>
#include <shlobj.h>

namespace {
constexpr unsigned kSystemStart = kMenuCmdID_Plugin_Start + 400;
}

struct QtNativeShellMenu::State {
    // Balance only successful initialization; Qt may already own the STA.
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    CMyComPtr<IContextMenu> context;
    CMyComPtr<IContextMenu2> messages;
    NWindows::CMenu nativeMenu;
    CPanel *panel = nullptr;
    QSet<unsigned> commands;
    ~State() {
        nativeMenu.Destroy();
        messages.Release();
        context.Release();
        if (SUCCEEDED(initialized))
            CoUninitialize();
    }

    void Render(HMENU handle, QMenu &target, const std::function<void(unsigned)> &dispatch) {
        NWindows::CMenu menu;
        menu.Attach(handle);
        for (int position = 0; position < menu.GetItemCount(); ++position) {
            NWindows::CMenuItem item;
            item.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_SUBMENU;
            if (!menu.GetItem(unsigned(position), true, item))
                continue;
            if (item.IsSeparator()) {
                target.addSeparator();
                continue;
            }
            QAction *action;
            if (item.hSubMenu) {
                auto *child = target.addMenu(ToQString(item.StringValue));
                // Native handlers can populate their submenu lazily.
                if (messages)
                    messages->HandleMenuMsg(WM_INITMENUPOPUP, WPARAM(item.hSubMenu),
                                            MAKELPARAM(position, false));
                Render(item.hSubMenu, *child, dispatch);
                action = child->menuAction();
            } else {
                action = target.addAction(ToQString(item.StringValue));
                if (item.wID >= kSystemStart && item.wID <= 0x7fff) {
                    wchar_t verb[256] = {};
                    if (SUCCEEDED(context->GetCommandString(item.wID - kSystemStart, GCS_VERBW,
                                                            nullptr, reinterpret_cast<LPSTR>(verb),
                                                            256)))
                        action->setProperty("shellVerb", QString::fromWCharArray(verb));
                }
                if (item.wID >= kSystemStart && item.wID <= 0x7fff &&
                    !(item.fState & (MFS_DISABLED | MFS_GRAYED))) {
                    commands.insert(item.wID);
                    QObject::connect(action, &QAction::triggered, &target,
                                     [dispatch, id = item.wID] { dispatch(id); });
                }
            }
            action->setEnabled(!(item.fState & (MFS_DISABLED | MFS_GRAYED)));
            action->setProperty("shellCommandID", item.wID);
            action->setProperty("nativeShellMenu", true);
            if (item.fState & MFS_CHECKED) {
                action->setCheckable(true);
                action->setChecked(true);
            }
            if (item.fState & MFS_DEFAULT)
                target.setDefaultAction(action);
        }
    }
};

QtNativeShellMenu::QtNativeShellMenu() : _state(std::make_unique<State>()) {}
QtNativeShellMenu::~QtNativeShellMenu() = default;

bool QtNativeShellMenu::Create(CPanel &panel, bool extended, const CRecordVector<UInt32> &indices,
                               QMenu &menu, std::function<void(unsigned)> dispatch) {
    if (!_state->nativeMenu.CreatePopup())
        return false;
    panel.CreateSystemMenu(_state->nativeMenu, extended, indices, _state->context);
    if (!_state->context || _state->nativeMenu.GetItemCount() == 0)
        return false;
    _state->panel = &panel;
    _state->context.QueryInterface(IID_IContextMenu2, &_state->messages);
    _state->Render(_state->nativeMenu, menu, dispatch);
    return true;
}

bool QtNativeShellMenu::CanInvoke(unsigned id) const { return _state->commands.contains(id); }

HRESULT QtNativeShellMenu::Invoke(unsigned offset) {
    if (!_state->panel || !CanInvoke(kSystemStart + offset))
        return E_INVALIDARG;
    return _state->panel->InvokeShellContextMenu(offset, _state->context);
}
#endif

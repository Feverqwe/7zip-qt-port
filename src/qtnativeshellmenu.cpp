#include "qtnativeshellmenu.h"
#ifdef _WIN32
#include "CPP/7zip/UI/FileManager/App.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "Windows/Menu.h"
#include "qtnativeshellmenupresentation.h"
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
    std::unique_ptr<QtNativeShellMenuPresentation> presentation;
    CPanel *panel = nullptr;
    QSet<unsigned> commands;
    ~State() {
        presentation.reset();
        context.Release();
        if (SUCCEEDED(initialized))
            CoUninitialize();
    }
};

QtNativeShellMenu::QtNativeShellMenu() : _state(std::make_unique<State>()) {}
QtNativeShellMenu::~QtNativeShellMenu() = default;

bool QtNativeShellMenu::Create(CPanel &panel, bool extended, const CRecordVector<UInt32> &indices,
                               QMenu &menu, std::function<void(unsigned)> dispatch) {
    NWindows::CMenu nativeMenu;
    NWindows::CMenuDestroyer release(nativeMenu);
    if (!nativeMenu.CreatePopup())
        return false;
    panel.CreateSystemMenu(nativeMenu, extended, indices, _state->context);
    if (!_state->context || nativeMenu.GetItemCount() == 0)
        return false;
    _state->panel = &panel;
    _state->presentation =
        std::make_unique<QtNativeShellMenuPresentation>(nativeMenu, _state->context, kSystemStart);
    nativeMenu.Detach();
    _state->commands = _state->presentation->Render(menu, dispatch);
    return true;
}

bool QtNativeShellMenu::CanInvoke(unsigned id) const { return _state->commands.contains(id); }

HRESULT QtNativeShellMenu::Invoke(unsigned offset) {
    if (!_state->panel || !CanInvoke(kSystemStart + offset))
        return E_INVALIDARG;
    return _state->panel->InvokeShellContextMenu(offset, _state->context);
}
#endif

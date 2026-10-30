#include "platform/shell/qtshellmenupresentation.h"

bool NQtShellMenu::CMenu::CreatePopup() {
    _handle = new CMenuHandle;
    return true;
}

bool NQtShellMenu::CMenu::InsertItem(unsigned pos, bool byPosition, const CMenuItem &item) {
    if (!_handle || !byPosition || pos > _handle->Items.size())
        return false;
    CMenuHandle::CEntry entry{item.fType, item.wID, item.hbmpUnchecked, item.StringValue, nullptr};
    const auto inserted = _handle->Items.insert(_handle->Items.begin() + pos, std::move(entry));
    // Match native InsertMenuItem: refusal/exception leaves the submenu with
    // its caller; only an accepted parent entry adopts it.
    inserted->SubMenu.reset(item.hSubMenu);
    return true;
}

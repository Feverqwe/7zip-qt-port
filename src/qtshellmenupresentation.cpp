#include "qtshellmenupresentation.h"

bool NQtShellMenu::CMenu::CreatePopup() {
    _handle = new CMenuHandle;
    return true;
}

bool NQtShellMenu::CMenu::InsertItem(unsigned pos, bool byPosition, const CMenuItem &item) {
    if (!_handle || !byPosition || pos > _handle->Items.size())
        return false;
    CMenuHandle::CEntry entry{item.fType, item.wID, item.hbmpUnchecked, item.StringValue,
                              std::unique_ptr<CMenuHandle>(item.hSubMenu)};
    _handle->Items.insert(_handle->Items.begin() + pos, std::move(entry));
    return true;
}

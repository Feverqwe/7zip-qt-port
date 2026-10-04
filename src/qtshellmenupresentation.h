// Narrow menu-handle adapter for the copied Explorer QueryContextMenu body.
// Native/Qt consumers render this tree; it performs no archive operations.
#pragma once
#include "Common/MyString.h"
#include <memory>
#include <vector>

namespace NQtShellMenu {
struct CMenuHandle;
struct CMenuItem {
    unsigned fType = 0;
    unsigned fMask = 0;
    unsigned wID = 0;
    unsigned hbmpUnchecked = 0;
    CMenuHandle *hSubMenu = nullptr;
    UString StringValue;
};
struct CMenuHandle {
    struct CEntry {
        unsigned Type;
        unsigned ID;
        unsigned Bitmap;
        UString Text;
        std::unique_ptr<CMenuHandle> SubMenu;
    };
    std::vector<CEntry> Items;
};
class CMenu {
    CMenuHandle *_handle = nullptr;

  public:
    operator CMenuHandle *() const { return _handle; }
    explicit operator bool() const { return _handle != nullptr; }
    void Attach(CMenuHandle *handle) { _handle = handle; }
    CMenuHandle *Detach() {
        auto *handle = _handle;
        _handle = nullptr;
        return handle;
    }
    bool CreatePopup();
    bool InsertItem(unsigned pos, bool byPosition, const CMenuItem &item);
};
class CMenuDestroyer {
    CMenuHandle *_handle = nullptr;

  public:
    ~CMenuDestroyer() { delete _handle; }
    void Attach(CMenuHandle *handle) { _handle = handle; }
    void Disable() { _handle = nullptr; }
};
// Presentation-only tags; no Win32 handles or registry APIs cross this boundary.
enum { kString = 0, kSeparator = 1, kType = 1, kId = 2, kCheckmarks = 4, kSubmenu = 8 };
} // namespace NQtShellMenu

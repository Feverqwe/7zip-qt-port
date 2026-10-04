#include "winshellmenupresentation.h"
#include "7zip/UI/Explorer/resource.h"
#include "Windows/Menu.h"

HBITMAP LoadNativeShellMenuBitmap() {
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&LoadNativeShellMenuBitmap), &module))
        return nullptr;
    return LoadBitmapW(module, MAKEINTRESOURCEW(IDB_MENU_LOGO));
}

HRESULT InsertNativeShellMenu(HMENU menu, UINT position, const NQtShellMenu::CMenuHandle &tree,
                              HBITMAP bitmap) {
    NWindows::CMenu native;
    native.Attach(menu);
    for (const auto &entry : tree.Items) {
        NWindows::CMenuItem item;
        item.fMask = MIIM_TYPE | MIIM_ID;
        item.fType = entry.Type == NQtShellMenu::kSeparator ? MFT_SEPARATOR : MFT_STRING;
        item.wID = entry.ID;
        item.StringValue = entry.Text;
        if (entry.Bitmap && bitmap) {
            // Original MyInsertMenu/MyAddSubMenu use unchecked menu bitmaps.
            item.fMask |= MIIM_CHECKMARKS;
            item.hbmpUnchecked = bitmap;
        }
        if (entry.SubMenu) {
            NWindows::CMenu submenu;
            if (!submenu.CreatePopup())
                return HRESULT_FROM_WIN32(GetLastError());
            NWindows::CMenuDestroyer destroy(submenu);
            const HRESULT result = InsertNativeShellMenu(submenu, 0, *entry.SubMenu, bitmap);
            if (FAILED(result)) {
                return result;
            }
            item.hSubMenu = submenu.Detach();
            item.fMask |= MIIM_SUBMENU;
        }
        if (!native.InsertItem(position++, true, item)) {
            const DWORD error = GetLastError();
            if (item.hSubMenu)
                DestroyMenu(item.hSubMenu);
            return HRESULT_FROM_WIN32(error);
        }
        // The receiving HMENU now owns the inserted submenu. Its bitmaps stay
        // owned by the context object, as in upstream CZipContextMenu.
    }
    return S_OK;
}

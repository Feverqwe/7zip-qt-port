#pragma once
#include "platform/shell/Shell.h"
#include "platform/shell/qtshellmenupresentation.h"

// The shared source generator owns commands. This boundary only inserts its
// presentation tree into the shell's existing menu, retaining native ownership.
HRESULT InsertNativeShellMenu(HMENU menu, UINT position, const NQtShellMenu::CMenuHandle &tree,
                              HBITMAP bitmap);
HBITMAP LoadNativeShellMenuBitmap();

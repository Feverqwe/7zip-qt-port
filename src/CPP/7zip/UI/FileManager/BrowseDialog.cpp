// Adapted from 7-Zip CPP/7zip/UI/FileManager/BrowseDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Only Dlg_CreateFolder is ported. Qt/localized strings replace HWND/LangString;
// the rest of browsing remains with the native Qt filesystem dialogs.
#include "BrowseDialog.h"
#include "ComboDialog.h"

bool Dlg_CreateFolder(QWidget *wnd, QString &destName) {
    destName.clear();
    CComboDialog dlg("Создать папку", "Имя папки:", "Новая папка", wnd);
    if (dlg.exec() != QDialog::Accepted)
        return false;
    destName = dlg.Value;
    return true;
}

// Adapted from 7-Zip CPP/Windows/CommonDialog.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// QWidget replaces HWND; portable state and source title lifetime remain.
#ifndef ZIP7_INC_WINDOWS_COMMON_DIALOG_H
#define ZIP7_INC_WINDOWS_COMMON_DIALOG_H

#include "Common/MyString.h"
class QWidget;

namespace NWindows {
struct CCommonDialogInfo {
    int FilterIndex;
    bool SaveMode;
    QWidget *hwndOwner;
    const wchar_t *lpstrTitle;
    UString FilePath;

    CCommonDialogInfo()
        : FilterIndex(-1), SaveMode(false), hwndOwner(nullptr), lpstrTitle(nullptr) {}

    // Two sequential strings (description, masks) represent each filter.
    bool CommonDlg_BrowseForFile(const wchar_t *lpstrInitialDir, const UStringVector &filters);
};
} // namespace NWindows
#endif

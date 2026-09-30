// Adapted from 7-Zip CPP/7zip/UI/FileManager/BrowseDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Dlg_CreateFolder and CorrectFsPath are ported. Qt/localized strings replace
// HWND/LangString; other browsing remains with native Qt filesystem dialogs.
#include "BrowseDialog.h"
#include "ComboDialog.h"
#include "Common/StringConvert.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"

using namespace NWindows;
using namespace NFile;
using namespace NName;

bool Dlg_CreateFolder(QWidget *wnd, QString &destName) {
    destName.clear();
    CComboDialog dlg("Создать папку", "Имя папки:", "Новая папка", wnd);
    if (dlg.exec() != QDialog::Accepted)
        return false;
    destName = dlg.Value;
    return true;
}

#ifdef _WIN32
static void RemoveDotsAndSpaces(UString &path) {
    while (!path.IsEmpty()) {
        wchar_t c = path.Back();
        if (c != ' ' && c != '.')
            return;
        path.DeleteBack();
    }
}

bool CorrectFsPath(const UString &relBase, const UString &path2, UString &result) {
    result.Empty();

    UString path = path2;
    path.Replace(L'/', WCHAR_PATH_SEPARATOR);
    unsigned start = 0;
    UString base;

    if (IsAbsolutePath(path)) {
#ifndef UNDER_CE
        if (IsSuperOrDevicePath(path)) {
            result = path;
            return true;
        }
#endif
        start = GetRootPrefixSize(path);
    } else {
#ifndef UNDER_CE
        if (IsSuperOrDevicePath(relBase)) {
            result = path;
            return true;
        }
#endif
        base = relBase;
    }

    result += path.Left(start);
    bool checkExist = true;
    UString cur;

    for (;;) {
        if (start == path.Len())
            break;
        const int slashPos = path.Find(WCHAR_PATH_SEPARATOR, start);
        cur.SetFrom(path.Ptr(start), (slashPos < 0 ? path.Len() : (unsigned)slashPos) - start);
        if (checkExist) {
            CFileInfo fi;
            if (fi.Find(us2fs(base + result + cur))) {
                if (!fi.IsDir()) {
                    result = path;
                    break;
                }
            } else
                checkExist = false;
        }
        if (!checkExist)
            RemoveDotsAndSpaces(cur);
        result += cur;
        if (slashPos < 0)
            break;
        start = (unsigned)(slashPos + 1);
        result.Add_PathSepar();
    }

    return true;
}
#else
bool CorrectFsPath(const UString & /* relBase */, const UString &path, UString &result) {
    result = path;
    return true;
}
#endif

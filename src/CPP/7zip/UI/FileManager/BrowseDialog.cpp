// Adapted from 7-Zip CPP/7zip/UI/FileManager/BrowseDialog.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Dlg_CreateFolder, CorrectFsPath, MyBrowseForFolder and BrowseForFile are ported.
// Qt/localized strings replace HWND/LangString and native platform pickers.
#include "BrowseDialog.h"
#include "7zip/UI/FileManager/resource.h"
#include "ComboDialog.h"
#include "Common/IntToString.h"
#include "Common/StringConvert.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "qtlangcontrols.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>

using namespace NWindows;
using namespace NFile;
using namespace NName;

// Original shared list-size formatter used by BrowseDialog2.
wchar_t *Browse_ConvertSizeToString(UInt64 v, wchar_t *s) {
    char c = 0;
    if (v >= ((UInt64)10000 << 20)) {
        v >>= 30;
        c = 'G';
    } else if (v >= ((UInt64)10000 << 10)) {
        v >>= 20;
        c = 'M';
    } else if (v >= ((UInt64)10000 << 0)) {
        v >>= 10;
        c = 'K';
    }
    s = ConvertUInt64ToString(v, s);
    if (c != 0) {
        *s++ = ' ';
        *s++ = (wchar_t)c;
        *s++ = 'B';
        *s = 0;
    }
    return s;
}

bool Dlg_CreateFolder(QWidget *wnd, QString &destName) {
    destName.clear();
    CComboDialog dlg(QtLangText(IDS_CREATE_FOLDER), QtLangText(IDS_CREATE_FOLDER_NAME),
                     QtLangText(IDS_CREATE_FOLDER_DEFAULT_NAME), wnd);
    dlg.BindLanguage(IDS_CREATE_FOLDER, IDS_CREATE_FOLDER_NAME);
    if (dlg.exec() != QDialog::Accepted)
        return false;
    destName = dlg.Value;
    return true;
}

bool MyBrowseForFolder(QWidget *owner, const QString &title, const QString &path,
                       QString &resultPath) {
    resultPath.clear();
    const QString selected = QFileDialog::getExistingDirectory(owner, title, path);
    if (selected.isEmpty())
        return false;
    resultPath = QDir::cleanPath(selected);
    return true;
}

bool CBrowseInfo::BrowseForFile() {
    const QString storedPath = FilePath;
    const QFileInfo initial(FilePath);
    QFileDialog dialog(Owner, Title, initial.absolutePath(), Filters);
    dialog.setFileMode(SaveMode ? QFileDialog::AnyFile : QFileDialog::ExistingFile);
    dialog.setAcceptMode(SaveMode ? QFileDialog::AcceptSave : QFileDialog::AcceptOpen);
    if (FilterIndex >= 0 && FilterIndex < dialog.nameFilters().size())
        dialog.selectNameFilter(dialog.nameFilters()[FilterIndex]);
    if (!FilePath.isEmpty())
        dialog.selectFile(initial.fileName());
    if (dialog.exec() != QDialog::Accepted) {
        FilePath = storedPath;
        return false;
    }
    const QStringList selected = dialog.selectedFiles();
    if (selected.isEmpty()) {
        FilePath = storedPath;
        return false;
    }
    FilePath = selected.front();
    FilterIndex = dialog.nameFilters().indexOf(dialog.selectedNameFilter());
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

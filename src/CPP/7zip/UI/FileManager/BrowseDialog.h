// Adapted from 7-Zip CPP/7zip/UI/FileManager/BrowseDialog.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "Common/MyString.h"
#include <QString>
class QWidget;
bool Dlg_CreateFolder(QWidget *wnd, QString &destName);
bool CorrectFsPath(const UString &relBase, const UString &path, UString &result);

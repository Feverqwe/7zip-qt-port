// Adapted from 7-Zip CPP/Windows/FileDir.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "Common/MyString.h"
#include <QString>
#include <functional>

namespace NWindows::NFile::NDir {
bool GetCurrentDir(FString &path);
bool DeleteFileAlways(const QString &path);
bool RemoveDirWithSubItems(const QString &path,
                           const std::function<bool(const QString &)> &checkBreak = {});
} // namespace NWindows::NFile::NDir

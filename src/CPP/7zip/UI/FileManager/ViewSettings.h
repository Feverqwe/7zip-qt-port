// Adapted from 7-Zip CPP/7zip/UI/FileManager/ViewSettings.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once

#include "Common/MyString.h"
#include <QStringList>

void SaveFolderHistory(const UStringVector &folders, const QString &settingsPath);
void ReadFolderHistory(UStringVector &folders, const QString &settingsPath);
void SaveFastFolders(const UStringVector &folders, const QString &settingsPath);
void ReadFastFolders(UStringVector &folders, const QString &settingsPath);

// Conversion is confined to the Qt presentation/settings boundary.
inline UString ToUString(const QString &s) { return UString(s.toStdWString().c_str()); }
inline QString ToQString(const UString &s) { return QString::fromWCharArray(s.Ptr(), s.Len()); }

void SaveCopyHistory(const QStringList &folders, const QString &settingsPath);
void ReadCopyHistory(QStringList &folders, const QString &settingsPath);
void AddUniqueStringToHeadOfList(QStringList &list, const QString &s);

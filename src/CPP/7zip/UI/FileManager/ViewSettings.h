// Adapted from 7-Zip CPP/7zip/UI/FileManager/ViewSettings.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once

#include <QStringList>

void SaveCopyHistory(const QStringList &folders, const QString &settingsPath);
void ReadCopyHistory(QStringList &folders, const QString &settingsPath);
void AddUniqueStringToHeadOfList(QStringList &list, const QString &s);

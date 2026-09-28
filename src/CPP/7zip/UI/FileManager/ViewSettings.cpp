// Adapted from 7-Zip CPP/7zip/UI/FileManager/ViewSettings.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// QSettings replaces Registry. Callers supply the INI path; GUI-thread access
// needs no Win32 critical section. The old Qt key is a read-only fallback.
#include "ViewSettings.h"

#include <QSettings>

static const char *const kCopyHistoryValueName = "CopyHistory";

static void SaveStringList(const char *valueName, const QStringList &folders,
                           const QString &settingsPath) {
    QSettings key(settingsPath, QSettings::IniFormat);
    key.setValue(valueName, folders);
}

static void ReadStringList(const char *valueName, QStringList &folders,
                           const QString &settingsPath) {
    folders.clear();
    QSettings key(settingsPath, QSettings::IniFormat);
    if (key.contains(valueName))
        folders = key.value(valueName).toStringList();
    else
        folders = key.value("copy/history").toStringList();
}

void SaveCopyHistory(const QStringList &folders, const QString &settingsPath) {
    SaveStringList(kCopyHistoryValueName, folders, settingsPath);
}

void ReadCopyHistory(QStringList &folders, const QString &settingsPath) {
    ReadStringList(kCopyHistoryValueName, folders, settingsPath);
}

void AddUniqueStringToHeadOfList(QStringList &list, const QString &s) {
    for (qsizetype i = 0; i < list.size();) {
        if (s.compare(list[i], Qt::CaseInsensitive) == 0)
            list.removeAt(i);
        else
            i++;
    }
    list.insert(0, s);
}

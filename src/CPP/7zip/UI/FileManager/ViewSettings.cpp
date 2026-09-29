// Adapted from 7-Zip CPP/7zip/UI/FileManager/ViewSettings.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// QSettings replaces Registry. Callers supply the INI path; GUI-thread access
// needs no Win32 critical section. Only upstream-named keys are read.
#include "ViewSettings.h"

#include <QSettings>

static const char *const kCopyHistoryValueName = "CopyHistory";
static const char *const kFolderHistoryValueName = "FolderHistory";
static const char *const kFastFoldersValueName = "FolderShortcuts";

void CListMode::Save(const QString &settingsPath) const {
    UInt32 t = 0;
    for (int i = 0; i < 2; i++)
        t |= (Panels[i] & 0xFF) << (i * 8);
    QSettings key(settingsPath, QSettings::IniFormat);
    key.setValue("ListMode", t);
}

void CListMode::Read(const QString &settingsPath) {
    Init();
    QSettings key(settingsPath, QSettings::IniFormat);
    bool ok = false;
    const qulonglong value = key.value("ListMode").toULongLong(&ok);
    if (!ok || value > 0xFFFFFFFFu)
        return;
    UInt32 t = static_cast<UInt32>(value);
    for (int i = 0; i < 2; i++) {
        Panels[i] = t & 0xFF;
        t >>= 8;
    }
}

static UString GetPanelPathName(UInt32 panelIndex) {
    UString s(L"PanelPath");
    s.Add_UInt32(panelIndex);
    return s;
}

void SavePanelPath(UInt32 panel, const UString &path, const QString &settingsPath) {
    QSettings key(settingsPath, QSettings::IniFormat);
    key.setValue(ToQString(GetPanelPathName(panel)), ToQString(path));
}

bool ReadPanelPath(UInt32 panel, UString &path, const QString &settingsPath) {
    QSettings key(settingsPath, QSettings::IniFormat);
    const QString name = ToQString(GetPanelPathName(panel));
    if (!key.contains(name))
        return false;
    path = ToUString(key.value(name).toString());
    return true;
}

static void SaveStringList(const char *valueName, const UStringVector &folders,
                           const QString &settingsPath) {
    QStringList strings;
    FOR_VECTOR(i, folders)
    strings.append(ToQString(folders[i]));
    QSettings key(settingsPath, QSettings::IniFormat);
    key.setValue(valueName, strings);
}

static void ReadStringList(const char *valueName, UStringVector &folders,
                           const QString &settingsPath) {
    folders.Clear();
    QSettings key(settingsPath, QSettings::IniFormat);
    const QStringList strings = key.value(valueName).toStringList();
    for (const QString &s : strings)
        folders.Add(ToUString(s));
}

void SaveFolderHistory(const UStringVector &folders, const QString &settingsPath) {
    SaveStringList(kFolderHistoryValueName, folders, settingsPath);
}

void ReadFolderHistory(UStringVector &folders, const QString &settingsPath) {
    ReadStringList(kFolderHistoryValueName, folders, settingsPath);
}

void SaveFastFolders(const UStringVector &folders, const QString &settingsPath) {
    SaveStringList(kFastFoldersValueName, folders, settingsPath);
}

void ReadFastFolders(UStringVector &folders, const QString &settingsPath) {
    ReadStringList(kFastFoldersValueName, folders, settingsPath);
}

static void SaveStringList(const char *valueName, const QStringList &folders,
                           const QString &settingsPath) {
    QSettings key(settingsPath, QSettings::IniFormat);
    key.setValue(valueName, folders);
}

static void ReadStringList(const char *valueName, QStringList &folders,
                           const QString &settingsPath) {
    folders.clear();
    QSettings key(settingsPath, QSettings::IniFormat);
    folders = key.value(valueName).toStringList();
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

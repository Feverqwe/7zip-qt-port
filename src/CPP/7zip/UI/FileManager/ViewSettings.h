// Adapted from 7-Zip CPP/7zip/UI/FileManager/ViewSettings.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once

#include "Common/MyString.h"
#include <QStringList>

const UInt32 kListMode_Report = 3;
void SaveToolbarsMask(UInt32 toolbarMask, const QString &settingsPath);
UInt32 ReadToolbarsMask(const QString &settingsPath);

struct CListMode {
    UInt32 Panels[2];
    void Init() { Panels[0] = Panels[1] = kListMode_Report; }
    CListMode() { Init(); }
    void Save(const QString &settingsPath) const;
    void Read(const QString &settingsPath);
};

void SavePanelPath(UInt32 panel, const UString &path, const QString &settingsPath);
bool ReadPanelPath(UInt32 panel, UString &path, const QString &settingsPath);

// RECT's four signed 32-bit coordinates, without the Win32 header dependency.
struct CWindowRect {
    Int32 left;
    Int32 top;
    Int32 right;
    Int32 bottom;
};

struct CWindowInfo {
    CWindowRect rect;
    bool maximized;
    UInt32 numPanels;
    UInt32 currentPanel;
    UInt32 splitterPos;
    void Save(const QString &settingsPath) const;
    void Read(bool &windowPosDefined, bool &panelInfoDefined, const QString &settingsPath);
};

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

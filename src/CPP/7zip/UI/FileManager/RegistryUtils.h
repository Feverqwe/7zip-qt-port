// Adapted from 7-Zip RegistryUtils.h (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "Common/MyString.h"
#include <QString>

// Shared Qt configuration-path adapter for callers of registry-ported modules.
QString FmSettingsPath();
void SaveFlatView(UInt32 panelIndex, bool enable, const QString &settingsPath);
bool ReadFlatView(UInt32 panelIndex, const QString &settingsPath);
void SaveRegEditor(bool useEditor, const UString &path, const QString &settingsPath);
void ReadRegEditor(bool useEditor, UString &path, const QString &settingsPath);

struct CFmSettings {
    bool ShowDots;
    bool ShowRealFileIcons;
    bool FullRow;
    bool ShowGrid;
    bool SingleClick;
    bool AlternativeSelection;
    bool ShowSystemMenu;

    void Save(const QString &settingsPath) const;
    void Load(const QString &settingsPath);
};

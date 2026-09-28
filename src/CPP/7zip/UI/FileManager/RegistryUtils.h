// Adapted from 7-Zip RegistryUtils.h (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include <QString>

// Shared Qt configuration-path adapter for callers of registry-ported modules.
QString FmSettingsPath();

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

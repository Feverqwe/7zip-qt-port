// Adapted from 7-Zip SettingsPage.h (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "RegistryUtils.h"
#include <QDialog>

class QCheckBox;
class CSettingsPage : public QDialog {
    bool _wasChanged = false;
    bool _initMode = true;
    CFmSettings _settings;
    QString _settingsPath;
    QCheckBox *_showDots;
    QCheckBox *_showRealFileIcons;
    QCheckBox *_fullRow;
    QCheckBox *_showGrid;
    QCheckBox *_singleClick;
    QCheckBox *_alternativeSelection;
    QCheckBox *_showSystemMenu;
    bool OnInit();
    void OnButtonClicked();
    void OnApply();

  public:
    explicit CSettingsPage(const QString &settingsPath, QWidget *parent = nullptr);
    void accept() override;
    CFmSettings settings() const { return _settings; }
};

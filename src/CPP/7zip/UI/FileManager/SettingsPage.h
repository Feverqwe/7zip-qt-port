// Adapted from 7-Zip SettingsPage.h (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "RegistryUtils.h"
#include <QDialog>

class QCheckBox;
class QtMemoryLimitSpin;
class CSettingsPage : public QDialog {
    Q_OBJECT
    bool _wasChanged = false;
    bool _initMode = true;
    bool _largePages_wasChanged = false;
    bool _memx_wasChanged = false;
    CFmSettings _settings;
    QString _settingsPath;
    QCheckBox *_showDots;
    QCheckBox *_showRealFileIcons;
    QCheckBox *_fullRow;
    QCheckBox *_showGrid;
    QCheckBox *_singleClick;
    QCheckBox *_alternativeSelection;
    QCheckBox *_showSystemMenu;
    QCheckBox *_largePages;
    QCheckBox *_memSet;
    QtMemoryLimitSpin *_memSpin;
    void EnableSpin(bool enable);
    bool OnInit();
    void OnButtonClicked();

  public:
    explicit CSettingsPage(const QString &settingsPath, QWidget *parent = nullptr,
                           bool embedded = false);
    long OnApply();
    void OnNotifyHelp();
    void accept() override;
    CFmSettings settings() const { return _settings; }
    bool IsModified() const { return _wasChanged || _largePages_wasChanged || _memx_wasChanged; }

  signals:
    void Changed();
};

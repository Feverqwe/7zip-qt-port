// Adapted from 7-Zip MenuPage.h (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include <QWidget>
class QCheckBox;
class QComboBox;
class QListWidget;
class QListWidgetItem;

// Windows source ArchiveExtractCallback can write Zone.Identifier. The shell
// DLL/registration service remains unavailable until the P2-09 source port.
bool IsZonePropagationSupported();

class CMenuPage : public QWidget {
    Q_OBJECT
    bool _initMode = true;
    bool _cascaded_Changed = false;
    bool _menuIcons_Changed = false;
    bool _elimDup_Changed = false;
    bool _writeZone_Changed = false;
    bool _flags_Changed = false;
    QString _settingsPath;
    QCheckBox *_cascaded;
    QCheckBox *_menuIcons;
    QCheckBox *_elimDup;
    QListWidget *_listView;
    QComboBox *_zoneCombo;
    void Clear_MenuChanged();
    bool OnButtonClicked(unsigned buttonID);
    bool OnCommand(unsigned itemID);
    bool OnItemChanged(QListWidgetItem *item);

  public:
    explicit CMenuPage(const QString &settingsPath, QWidget *parent = nullptr,
                       bool zoneSupported = IsZonePropagationSupported());
    bool OnInit();
    long OnApply();
    bool IsModified() const;

  signals:
    void Changed();
};

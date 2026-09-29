// Adapted from 7-Zip SettingsPage.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt replaces property-page controls. Extraction memory and large pages belong to P1-12.
#include "SettingsPage.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

CSettingsPage::CSettingsPage(const QString &settingsPath, QWidget *parent)
    : QDialog(parent), _settingsPath(settingsPath) {
    setObjectName("settingsDialog");
    setWindowTitle("Настройки");
    auto *layout = new QVBoxLayout(this);
    _showDots = new QCheckBox(QStringLiteral("Показывать элемент «..»"), this);
    _showDots->setObjectName(QStringLiteral("showDotsOption"));
    _showRealFileIcons = new QCheckBox("Показывать системные значки файлов", this);
    _showRealFileIcons->setObjectName("realFileIconsOption");
    _fullRow = new QCheckBox(QStringLiteral("Выделять всю строку"), this);
    _fullRow->setObjectName(QStringLiteral("fullRowOption"));
    _showGrid = new QCheckBox("Показывать линии сетки", this);
    _showGrid->setObjectName("gridOption");
    _singleClick = new QCheckBox("Открывать одним щелчком", this);
    _singleClick->setObjectName("singleClickOption");
    _alternativeSelection = new QCheckBox(QStringLiteral("Альтернативное выделение"), this);
    _alternativeSelection->setObjectName(QStringLiteral("alternativeSelectionOption"));
    _alternativeSelection->setEnabled(false);
    _showSystemMenu = new QCheckBox(QStringLiteral("Показывать системное меню"), this);
    _showSystemMenu->setObjectName(QStringLiteral("showSystemMenuOption"));
    for (auto *control : {_showDots, _showRealFileIcons, _fullRow, _showGrid, _singleClick,
                          _alternativeSelection, _showSystemMenu}) {
        layout->addWidget(control);
        connect(control, &QCheckBox::toggled, this, &CSettingsPage::OnButtonClicked);
    }
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->setObjectName("settingsButtons");
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &CSettingsPage::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    OnInit();
}

bool CSettingsPage::OnInit() {
    _initMode = true;
    _wasChanged = false;
    _settings.Load(_settingsPath);
    _showDots->setChecked(_settings.ShowDots);
    _showRealFileIcons->setChecked(_settings.ShowRealFileIcons);
    _fullRow->setChecked(_settings.FullRow);
    _showGrid->setChecked(_settings.ShowGrid);
    _singleClick->setChecked(_settings.SingleClick);
    _alternativeSelection->setChecked(_settings.AlternativeSelection);
    _showSystemMenu->setChecked(_settings.ShowSystemMenu);
    _initMode = false;
    return true;
}

void CSettingsPage::OnButtonClicked() {
    if (!_initMode)
        _wasChanged = true;
}

void CSettingsPage::OnApply() {
    if (_wasChanged) {
        // Retain the alternative-selection flag until its mark policy is ported.
        CFmSettings st = _settings;
        st.ShowDots = _showDots->isChecked();
        st.ShowRealFileIcons = _showRealFileIcons->isChecked();
        st.FullRow = _fullRow->isChecked();
        st.ShowGrid = _showGrid->isChecked();
        st.SingleClick = _singleClick->isChecked();
        st.ShowSystemMenu = _showSystemMenu->isChecked();
        st.Save(_settingsPath);
        _settings = st;
        _wasChanged = false;
    }
}

void CSettingsPage::accept() {
    OnApply();
    QDialog::accept();
}

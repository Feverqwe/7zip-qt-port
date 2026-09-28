// Adapted from 7-Zip SettingsPage.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Partial page: real icons, grid and single click. Qt replaces property-page controls.
#include "SettingsPage.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

CSettingsPage::CSettingsPage(const QString &settingsPath, QWidget *parent)
    : QDialog(parent), _settingsPath(settingsPath) {
    setObjectName("settingsDialog");
    setWindowTitle("Настройки");
    auto *layout = new QVBoxLayout(this);
    _showRealFileIcons = new QCheckBox("Показывать системные значки файлов", this);
    _showRealFileIcons->setObjectName("realFileIconsOption");
    _showGrid = new QCheckBox("Показывать линии сетки", this);
    _showGrid->setObjectName("gridOption");
    _singleClick = new QCheckBox("Открывать одним щелчком", this);
    _singleClick->setObjectName("singleClickOption");
    for (auto *control : {_showRealFileIcons, _showGrid, _singleClick}) {
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
    _showRealFileIcons->setChecked(_settings.ShowRealFileIcons);
    _showGrid->setChecked(_settings.ShowGrid);
    _singleClick->setChecked(_settings.SingleClick);
    _initMode = false;
    return true;
}

void CSettingsPage::OnButtonClicked() {
    if (!_initMode)
        _wasChanged = true;
}

void CSettingsPage::OnApply() {
    if (_wasChanged) {
        // Retain omitted page flags instead of overwriting them at this partial-port boundary.
        CFmSettings st = _settings;
        st.ShowRealFileIcons = _showRealFileIcons->isChecked();
        st.ShowGrid = _showGrid->isChecked();
        st.SingleClick = _singleClick->isChecked();
        st.Save(_settingsPath);
        _settings = st;
        _wasChanged = false;
    }
}

void CSettingsPage::accept() {
    OnApply();
    QDialog::accept();
}

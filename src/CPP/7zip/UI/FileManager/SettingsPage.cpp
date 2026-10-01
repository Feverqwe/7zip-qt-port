// Adapted from 7-Zip SettingsPage.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Qt replaces property-page controls; source state/apply/RAM calculation retained.
#include "SettingsPage.h"
#include "../Common/ZipRegistry.h"
#include "7zip/UI/FileManager/SettingsPageRes.h"
#include "Common/IntToString.h"
#include "Common/StringToInt.h"
#include "Panel.h"
#include "Windows/ErrorMsg.h"
#include "Windows/System.h"
#include "qtlangcontrols.h"
#include "qtmemorylimitspin.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

CSettingsPage::CSettingsPage(const QString &settingsPath, QWidget *parent, bool embedded)
    : QDialog(parent), _settingsPath(settingsPath) {
    setObjectName(embedded ? "settingsPage" : "settingsDialog");
    if (embedded)
        setWindowFlags(Qt::Widget);
    QtBindResourceText(this, IDD_SETTINGS, "Settings");
    auto *layout = new QVBoxLayout(this);
    _showDots = new QCheckBox(this);
    QtBindResourceText(_showDots, IDX_SETTINGS_SHOW_DOTS, "Show \"..\" item");
    _showDots->setObjectName(QStringLiteral("showDotsOption"));
    _showRealFileIcons = new QCheckBox(this);
    QtBindResourceText(_showRealFileIcons, IDX_SETTINGS_SHOW_REAL_FILE_ICONS,
                       "Show real file &icons");
    _showRealFileIcons->setObjectName("realFileIconsOption");
    _fullRow = new QCheckBox(this);
    QtBindResourceText(_fullRow, IDX_SETTINGS_FULL_ROW, "&Full row select");
    _fullRow->setObjectName(QStringLiteral("fullRowOption"));
    _showGrid = new QCheckBox(this);
    QtBindResourceText(_showGrid, IDX_SETTINGS_SHOW_GRID, "Show &grid lines");
    _showGrid->setObjectName("gridOption");
    _singleClick = new QCheckBox(this);
    QtBindResourceText(_singleClick, IDX_SETTINGS_SINGLE_CLICK, "&Single-click to open an item");
    _singleClick->setObjectName("singleClickOption");
    _alternativeSelection = new QCheckBox(this);
    QtBindResourceText(_alternativeSelection, IDX_SETTINGS_ALTERNATIVE_SELECTION,
                       "&Alternative selection mode");
    _alternativeSelection->setObjectName(QStringLiteral("alternativeSelectionOption"));
    _showSystemMenu = new QCheckBox(this);
    QtBindResourceText(_showSystemMenu, IDX_SETTINGS_SHOW_SYSTEM_MENU, "Show system &menu");
    _showSystemMenu->setObjectName(QStringLiteral("showSystemMenuOption"));
    for (auto *control : {_showDots, _showRealFileIcons, _fullRow, _showGrid, _singleClick,
                          _alternativeSelection, _showSystemMenu}) {
        layout->addWidget(control);
        connect(control, &QCheckBox::toggled, this, &CSettingsPage::OnButtonClicked);
    }
    _largePages = new QCheckBox(this);
    QtBindResourceText(_largePages, IDX_SETTINGS_LARGE_PAGES, "Use &large memory pages");
    _largePages->setObjectName("largePagesOption");
    layout->addWidget(_largePages);
    connect(_largePages, &QCheckBox::toggled, this, [this] {
        if (!_initMode) {
            _largePages_wasChanged = true;
            Changed();
        }
    });
    auto *memoryLabel = new QLabel(this);
    QtBindResourceText(memoryLabel, IDT_MEM_USAGE_EXTRACT,
                       "Maximum amount of RAM memory usage allowed to unpack archives:");
    memoryLabel->setObjectName("extractMemoryLabel");
    memoryLabel->setWordWrap(true);
    layout->addWidget(memoryLabel);
    auto *memoryRow = new QHBoxLayout;
    _memSet = new QCheckBox(":", this);
    _memSet->setObjectName("extractMemorySet");
    QtBindLangText(_memSet, IDT_MEM_USAGE_EXTRACT, {}, true);
    _memSpin = new QtMemoryLimitSpin(this);
    _memSpin->setObjectName("extractMemorySpin");
    QtBindLangText(_memSpin, IDT_MEM_USAGE_EXTRACT, {}, true);
    _memSpin->setFixedWidth(90);
    setFocusProxy(_memSpin);
    auto *ramLabel = new QLabel(this);
    ramLabel->setObjectName("extractMemoryRam");
    memoryRow->addWidget(_memSet);
    memoryRow->addWidget(_memSpin);
    memoryRow->addWidget(ramLabel);
    memoryRow->addStretch();
    layout->addLayout(memoryRow);
    connect(_memSet, &QCheckBox::toggled, this, [this](bool checked) {
        if (!_initMode) {
            _memx_wasChanged = true;
            EnableSpin(checked);
            Changed();
        }
    });
    connect(_memSpin, &QtMemoryLimitSpin::textChanged, this, [this] {
        if (!_initMode) {
            _memx_wasChanged = true;
            Changed();
        }
    });
    if (!embedded) {
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->setObjectName("settingsButtons");
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &CSettingsPage::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }
    OnInit();
}

bool CSettingsPage::OnInit() {
    _initMode = true;
    _wasChanged = false;
    _largePages_wasChanged = false;
    _memx_wasChanged = false;
    _settings.Load(_settingsPath);
    _showDots->setChecked(_settings.ShowDots);
    _showRealFileIcons->setChecked(_settings.ShowRealFileIcons);
    _fullRow->setChecked(_settings.FullRow);
    _showGrid->setChecked(_settings.ShowGrid);
    _singleClick->setChecked(_settings.SingleClick);
    _alternativeSelection->setChecked(_settings.AlternativeSelection);
    _showSystemMenu->setChecked(_settings.ShowSystemMenu);
    if (IsLargePageSupported())
        _largePages->setChecked(ReadLockMemoryEnable(_settingsPath));
    else
        _largePages->setEnabled(false);
    {
        size_t ramSize = (size_t)sizeof(size_t) << 29;
        const bool ramSize_defined = NWindows::NSystem::GetRamSize(ramSize);
        UInt32 ramSize_GB = (UInt32)(((UInt64)ramSize + (1u << 29)) >> 30);
        if (ramSize_GB == 0)
            ramSize_GB = 1;
        UString s("GB");
        if (ramSize_defined) {
            s += " / ";
            s.Add_UInt64(ramSize_GB);
            s += " GB (RAM)";
        }
        findChild<QLabel *>("extractMemoryRam")->setText(ToQString(s));
        const unsigned valMin = 1;
        unsigned valMax = 64;
        if (ramSize_defined) {
            const unsigned k_max_val = 1u << 14;
            if (ramSize_GB >= k_max_val)
                valMax = k_max_val;
            else if (ramSize_GB > 1)
                valMax = ramSize_GB - 1;
            else
                valMax = 1;
        }
        UInt32 limit = NExtract::Read_LimitGB(_settingsPath);
        if (limit != 0 && limit != (UInt32)(Int32)-1)
            _memSet->setChecked(true);
        else {
            limit = 4;
            EnableSpin(false);
        }
        _memSpin->setRange(valMin, valMax);
        s.Empty();
        s.Add_UInt32(limit);
        _memSpin->setText(ToQString(s));
    }
    _initMode = false;
    return true;
}

void CSettingsPage::OnButtonClicked() {
    if (!_initMode) {
        _wasChanged = true;
        Changed();
    }
}

void CSettingsPage::EnableSpin(bool enable) { _memSpin->setEnabled(enable); }

long CSettingsPage::OnApply() {
    if (_wasChanged) {
        CFmSettings st = _settings;
        st.ShowDots = _showDots->isChecked();
        st.ShowRealFileIcons = _showRealFileIcons->isChecked();
        st.FullRow = _fullRow->isChecked();
        st.ShowGrid = _showGrid->isChecked();
        st.SingleClick = _singleClick->isChecked();
        st.AlternativeSelection = _alternativeSelection->isChecked();
        st.ShowSystemMenu = _showSystemMenu->isChecked();
        st.Save(_settingsPath);
        _settings = st;
        _wasChanged = false;
    }
    if (_largePages_wasChanged) {
        if (IsLargePageSupported()) {
            const bool enable = _largePages->isChecked();
            // External engine cannot inherit this GUI's token. ArchiveBackend
            // passes -slp; upstream Console enables the privilege in that process.
            SaveLockMemoryEnable(enable, _settingsPath);
        }
        _largePages_wasChanged = false;
    }
    if (_memx_wasChanged) {
        UInt32 val = (UInt32)(Int32)-1;
        if (_memSet->isChecked()) {
            UString s = ToUString(_memSpin->text());
            const wchar_t *end;
            val = ConvertStringToUInt32(s.Ptr(), &end);
            if (s.IsEmpty() || *end != 0 || val > (1u << 30)) {
                QMessageBox::warning(this, "Sevenzip Desktop",
                                     ToQString(NWindows::NError::MyFormatMessage(E_INVALIDARG)));
                _memSpin->setFocus();
                return 1; // PSNRET_INVALID: keep the property sheet open.
            }
        }
        NExtract::Save_LimitGB(_settingsPath, val);
        _memx_wasChanged = false;
    }
    return 0;
}

void CSettingsPage::accept() {
    if (OnApply() == 0)
        QDialog::accept();
}

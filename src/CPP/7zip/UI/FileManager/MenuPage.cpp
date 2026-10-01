// Adapted from 7-Zip MenuPage.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Source menu state/list/apply bodies; Qt replaces property-page services.
// Shell DLL enumeration/registration and Help are owned by P2-09/P1-15.
#include "MenuPage.h"
#include "../Common/ZipRegistry.h"
#include "7zip/UI/Explorer/ContextMenuFlags.h"
#include "7zip/UI/Explorer/resource.h"
#include "7zip/UI/FileManager/MenuPageRes.h"
#include "7zip/UI/FileManager/PropertyNameRes.h"
#include "7zip/UI/GUI/ExtractDialogRes.h"
#include "Common/StringConvert.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

using namespace NContextMenuFlags;

struct CContextMenuItem {
    unsigned ControlID;
    UInt32 Flag;
};
static const CContextMenuItem kMenuItems[] = {
    {IDS_CONTEXT_OPEN, kOpen},
    {IDS_CONTEXT_OPEN, kOpenAs},
    {IDS_CONTEXT_EXTRACT, kExtract},
    {IDS_CONTEXT_EXTRACT_HERE, kExtractHere},
    {IDS_CONTEXT_EXTRACT_TO, kExtractTo},
    {IDS_CONTEXT_TEST, kTest},
    {IDS_CONTEXT_COMPRESS, kCompress},
    {IDS_CONTEXT_COMPRESS_TO, kCompressTo7z},
    {IDS_CONTEXT_COMPRESS_TO, kCompressToZip},
    {IDS_CONTEXT_COMPRESS_EMAIL, kCompressEmail},
    {IDS_CONTEXT_COMPRESS_TO_EMAIL, kCompressTo7zEmail},
    {IDS_CONTEXT_COMPRESS_TO_EMAIL, kCompressToZipEmail},
    {IDS_PROP_CHECKSUM, kCRC},
    {IDS_PROP_CHECKSUM, kCRC_Cascaded},
};

bool IsZonePropagationSupported() {
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

CMenuPage::CMenuPage(const QString &settingsPath, QWidget *parent, bool zoneSupported)
    : QWidget(parent), _settingsPath(settingsPath) {
    setObjectName("menuPage");
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(4);
    auto addCheck = [&](const QString &text, const char *name, unsigned id) {
        auto *check = new QCheckBox(text, this);
        check->setObjectName(name);
        check->setProperty("sourceControlID", id);
        check->setEnabled(false);
        check->setToolTip("Shell integration is unavailable in this build");
        layout->addWidget(check);
        connect(check, &QCheckBox::clicked, this, [this, id] { OnButtonClicked(id); });
        return check;
    };
    addCheck("Integrate 7-Zip to shell context menu", "shellIntegration",
             IDX_SYSTEM_INTEGRATE_TO_MENU);
#if defined(_WIN32) && !defined(_WIN64)
    const QString otherBitness = "64-bit";
#else
    const QString otherBitness = "32-bit";
#endif
    addCheck("Integrate 7-Zip to shell context menu (" + otherBitness + ")", "shellIntegration32",
             IDX_SYSTEM_INTEGRATE_TO_MENU_2);
    _cascaded = addCheck("Cascaded context menu", "cascadedMenu", IDX_SYSTEM_CASCADED_MENU);
    _menuIcons = addCheck("Icons in context menu", "menuIcons", IDX_SYSTEM_ICON_IN_MENU);
    _elimDup =
        addCheck("Eliminate duplication of root folder", "menuElimDup", IDX_EXTRACT_ELIM_DUP);
    auto *zoneRow = new QHBoxLayout;
    auto *zoneLabel = new QLabel("Propagate Zone.Id stream:", this);
    zoneLabel->setObjectName("menuZoneLabel");
    zoneLabel->setProperty("sourceControlID", IDT_SYSTEM_ZONE);
    _zoneCombo = new QComboBox(this);
    _zoneCombo->setObjectName("menuZone");
    _zoneCombo->setProperty("sourceControlID", IDC_SYSTEM_ZONE);
    _zoneCombo->setEnabled(zoneSupported);
    if (!zoneSupported)
        _zoneCombo->setToolTip("Zone.Identifier propagation is available in Windows");
    zoneLabel->setBuddy(_zoneCombo);
    zoneRow->addWidget(zoneLabel);
    zoneRow->addStretch();
    zoneRow->addWidget(_zoneCombo);
    layout->addLayout(zoneRow);
    auto *itemsLabel = new QLabel("Context menu items:", this);
    itemsLabel->setObjectName("menuItemsLabel");
    itemsLabel->setProperty("sourceControlID", IDT_SYSTEM_CONTEXT_MENU_ITEMS);
    layout->addWidget(itemsLabel);
    _listView = new QListWidget(this);
    _listView->setObjectName("menuItems");
    _listView->setProperty("sourceControlID", IDL_SYSTEM_OPTIONS);
    _listView->setEnabled(false);
    _listView->setToolTip("Shell integration is unavailable in this build");
    _listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    layout->addWidget(_listView, 1);
    connect(_zoneCombo, &QComboBox::currentIndexChanged, this,
            [this] { OnCommand(IDC_SYSTEM_ZONE); });
    connect(_listView, &QListWidget::itemChanged, this, &CMenuPage::OnItemChanged);
    OnInit();
}

void CMenuPage::Clear_MenuChanged() {
    _cascaded_Changed = false;
    _menuIcons_Changed = false;
    _elimDup_Changed = false;
    _writeZone_Changed = false;
    _flags_Changed = false;
}

bool CMenuPage::IsModified() const {
    return _cascaded_Changed || _menuIcons_Changed || _elimDup_Changed || _writeZone_Changed ||
           _flags_Changed;
}

static void LoadLang_Spec(UString &s, UInt32 id, const char *eng) {
    LangString(id, s);
    if (s.IsEmpty())
        s = eng;
    s.RemoveChar(L'&');
}

bool CMenuPage::OnInit() {
    _initMode = true;
    Clear_MenuChanged();
    CContextMenuInfo ci;
    ci.Load(_settingsPath);
    _cascaded->setChecked(ci.Cascaded.Val);
    _menuIcons->setChecked(ci.MenuIcons.Val);
    _elimDup->setChecked(ci.ElimDup.Val);
    _zoneCombo->clear();
    _listView->clear();
    {
        unsigned wz = ci.WriteZone;
        if (wz == (UInt32)(Int32)-1)
            wz = 0;
        for (unsigned i = 0; i <= 3; i++) {
            unsigned val = i;
            UString s;
            if (i == 3) {
                if (wz < 3)
                    break;
                val = wz;
            } else {
                if (i == 0)
                    LoadLang_Spec(s, 407, "No");
                else if (i == 1)
                    LoadLang_Spec(s, 406, "Yes");
                else
                    s = "For Office files"; // MenuPage2.rc; language owner P1-14.
            }
            if (s.IsEmpty())
                s.Add_UInt32(val);
            if (i == 0)
                s.Insert(0, L"* ");
            const int index = _zoneCombo->count();
            _zoneCombo->addItem(QString::fromWCharArray(s), val);
            if (val == wz)
                _zoneCombo->setCurrentIndex(index);
        }
    }
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(kMenuItems); i++) {
        const CContextMenuItem &menuItem = kMenuItems[i];
        UString s = LangString(menuItem.ControlID);
        if (menuItem.Flag == kCRC)
            s = "CRC SHA";
        else if (menuItem.Flag == kCRC_Cascaded)
            s = "7-Zip > CRC SHA";
        if (menuItem.Flag == kOpenAs || menuItem.Flag == kCRC || menuItem.Flag == kCRC_Cascaded)
            s += " >";
        switch (menuItem.ControlID) {
        case IDS_CONTEXT_EXTRACT_TO:
            s = MyFormatNew(s, LangString(IDS_CONTEXT_FOLDER));
            break;
        case IDS_CONTEXT_COMPRESS_TO:
        case IDS_CONTEXT_COMPRESS_TO_EMAIL: {
            UString s2 = LangString(IDS_CONTEXT_ARCHIVE);
            switch (menuItem.Flag) {
            case kCompressTo7z:
            case kCompressTo7zEmail:
                s2 += ".7z";
                break;
            case kCompressToZip:
            case kCompressToZipEmail:
                s2 += ".zip";
                break;
            }
            s = MyFormatNew(s, s2);
            break;
        }
        }
        auto *item = new QListWidgetItem(QString::fromWCharArray(s), _listView);
        item->setData(Qt::UserRole, menuItem.Flag);
        item->setCheckState((ci.Flags & menuItem.Flag) != 0 ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole + 1, item->checkState());
    }
    _initMode = false;
    return true;
}

long CMenuPage::OnApply() {
    if (IsModified()) {
        CContextMenuInfo ci;
        ci.Cascaded.Val = _cascaded->isChecked();
        ci.Cascaded.Def = _cascaded_Changed;
        ci.MenuIcons.Val = _menuIcons->isChecked();
        ci.MenuIcons.Def = _menuIcons_Changed;
        ci.ElimDup.Val = _elimDup->isChecked();
        ci.ElimDup.Def = _elimDup_Changed;
        int zoneIndex = _zoneCombo->currentData().toInt();
        if (zoneIndex <= 0)
            zoneIndex = -1;
        ci.WriteZone = (UInt32)(Int32)zoneIndex;
        ci.Flags = 0;
        for (unsigned i = 0; i < Z7_ARRAY_SIZE(kMenuItems); i++)
            if (_listView->item(int(i))->checkState() == Qt::Checked)
                ci.Flags |= kMenuItems[i].Flag;
        ci.Flags_Def = _flags_Changed;
        ci.Save(_settingsPath);
        Clear_MenuChanged();
    }
    return 0;
}

bool CMenuPage::OnButtonClicked(unsigned buttonID) {
    switch (buttonID) {
    case IDX_SYSTEM_CASCADED_MENU:
        _cascaded_Changed = true;
        break;
    case IDX_SYSTEM_ICON_IN_MENU:
        _menuIcons_Changed = true;
        break;
    case IDX_EXTRACT_ELIM_DUP:
        _elimDup_Changed = true;
        break;
    default:
        return false;
    }
    Changed();
    return true;
}

bool CMenuPage::OnCommand(unsigned itemID) {
    if (!_initMode && _zoneCombo->isEnabled() && itemID == IDC_SYSTEM_ZONE) {
        _writeZone_Changed = true;
        Changed();
        return true;
    }
    return false;
}

bool CMenuPage::OnItemChanged(QListWidgetItem *item) {
    if (_initMode)
        return true;
    const int oldState = item->data(Qt::UserRole + 1).toInt();
    const int newState = item->checkState();
    if (oldState != newState) {
        item->setData(Qt::UserRole + 1, newState);
        if (_listView->isEnabled()) {
            _flags_Changed = true;
            Changed();
        }
    }
    return true;
}

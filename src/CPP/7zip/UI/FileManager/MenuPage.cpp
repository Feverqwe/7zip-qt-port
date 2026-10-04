// Adapted from 7-Zip MenuPage.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Source menu state/list/apply bodies; Qt replaces property-page services.
// Shell DLL enumeration/registration and Help are owned by P2-09/P1-15.
#include "MenuPage.h"
#include "HelpUtils.h"

#include "../Common/ZipRegistry.h"
#include "7zip/UI/Explorer/ContextMenuFlags.h"
#include "7zip/UI/Explorer/resource.h"
#include "7zip/UI/FileManager/MenuPageRes.h"
#include "7zip/UI/FileManager/PropertyNameRes.h"
#include "7zip/UI/GUI/ExtractDialogRes.h"
#include "Common/StringConvert.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include "qtlangcontrols.h"
#include "qtmenupageservices.h"
#include "qtnativemenupageservices.h"
#include "qtshellregistryservices.h"
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

#define kMenuTopic "fm/options.htm#sevenZip"
void CMenuPage::OnNotifyHelp() { ShowHelpWindow(kMenuTopic, this); }

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

CMenuPage::CMenuPage(const QString &settingsPath, QWidget *parent, bool zoneSupported,
                     std::shared_ptr<IQtMenuPageServices> services)
    : QWidget(parent), _settingsPath(settingsPath),
      _services(services ? std::move(services) : CreateNativeMenuPageServices()) {
    setObjectName("menuPage");
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(4);
    auto addCheck = [&](const QString &text, const char *name, unsigned id) {
        auto *check = new QCheckBox(text, this);
        check->setObjectName(name);
        check->setProperty("languageWrapWidth", 380);
        check->setProperty("sourceControlID", id);
        QtBindResourceText(check, id, text);
        check->setEnabled(false);
        QtBindLocalToolTip(check, "Shell integration is unavailable in this build.",
                           "Интеграция с оболочкой недоступна в этой сборке.");
        layout->addWidget(check);
        connect(check, &QCheckBox::clicked, this, [this, id] { OnButtonClicked(id); });
        return check;
    };
    addCheck("Integrate 7-Zip to shell context menu", "shellIntegration",
             IDX_SYSTEM_INTEGRATE_TO_MENU);
    addCheck("Integrate 7-Zip to shell context menu", "shellIntegration32",
             IDX_SYSTEM_INTEGRATE_TO_MENU_2);
    _cascaded = addCheck("Cascaded context menu", "cascadedMenu", IDX_SYSTEM_CASCADED_MENU);
    _menuIcons = addCheck("Icons in context menu", "menuIcons", IDX_SYSTEM_ICON_IN_MENU);
    _elimDup =
        addCheck("Eliminate duplication of root folder", "menuElimDup", IDX_EXTRACT_ELIM_DUP);
    const bool commandsAvailable = _services && _services->CommandsAvailable();
    for (auto *control : {_cascaded, _menuIcons, _elimDup}) {
        control->setEnabled(commandsAvailable);
        if (commandsAvailable)
            QtBindLocalToolTip(control, {}, {});
    }
    auto *zoneRow = new QHBoxLayout;
    auto *zoneLabel = new QLabel("Propagate Zone.Id stream:", this);
    zoneLabel->setObjectName("menuZoneLabel");
    zoneLabel->setProperty("languageWrapWidth", 160);
    zoneLabel->setProperty("sourceControlID", IDT_SYSTEM_ZONE);
    QtBindResourceText(zoneLabel, IDT_SYSTEM_ZONE, "Propagate Zone.Id stream:");
    _zoneCombo = new QComboBox(this);
    _zoneCombo->setObjectName("menuZone");
    _zoneCombo->setProperty("sourceControlID", IDC_SYSTEM_ZONE);
    _zoneCombo->setEnabled(zoneSupported);
    if (!zoneSupported)
        QtBindLocalToolTip(_zoneCombo, "Zone.Identifier propagation is available on Windows.",
                           "Перенос Zone.Identifier доступен в Windows.");
    zoneLabel->setBuddy(_zoneCombo);
    zoneRow->addWidget(zoneLabel);
    zoneRow->addStretch();
    zoneRow->addWidget(_zoneCombo);
    layout->addLayout(zoneRow);
    auto *itemsLabel = new QLabel("Context menu items:", this);
    itemsLabel->setObjectName("menuItemsLabel");
    itemsLabel->setProperty("sourceControlID", IDT_SYSTEM_CONTEXT_MENU_ITEMS);
    QtBindResourceText(itemsLabel, IDT_SYSTEM_CONTEXT_MENU_ITEMS, "Context menu items:");
    layout->addWidget(itemsLabel);
    _listView = new QListWidget(this);
    _listView->setObjectName("menuItems");
    _listView->setProperty("sourceControlID", IDL_SYSTEM_OPTIONS);
    _listView->setEnabled(commandsAvailable);
    QtBindLocalToolTip(_listView, "Shell integration is unavailable in this build.",
                       "Интеграция с оболочкой недоступна в этой сборке.");
    if (commandsAvailable)
        QtBindLocalToolTip(_listView, {}, {});
    _listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    layout->addWidget(_listView, 1);
    connect(_zoneCombo, &QComboBox::currentIndexChanged, this,
            [this] { OnCommand(IDC_SYSTEM_ZONE); });
    connect(_listView, &QListWidget::itemChanged, this, &CMenuPage::OnItemChanged);
    OnInit();
    ReloadLangItems();
}

void CMenuPage::Clear_MenuChanged() {
    _cascaded_Changed = false;
    _menuIcons_Changed = false;
    _elimDup_Changed = false;
    _writeZone_Changed = false;
    _flags_Changed = false;
}

bool CMenuPage::IsModified() const {
    for (const auto &dll : _dlls)
        if (dll.wasChanged)
            return true;
    return _cascaded_Changed || _menuIcons_Changed || _elimDup_Changed || _writeZone_Changed ||
           _flags_Changed;
}

QCheckBox *CMenuPage::ShellControl(unsigned id) const {
    return findChild<QCheckBox *>(id == IDX_SYSTEM_INTEGRATE_TO_MENU ? "shellIntegration"
                                                                     : "shellIntegration32");
}

static void LoadLang_Spec(UString &s, UInt32 id, const char *eng) {
    LangString(id, s);
    if (s.IsEmpty())
        s = eng;
    s.RemoveChar(L'&');
}

static UString GetMenuItemText(const CContextMenuItem &menuItem) {
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
    return s;
}

void CMenuPage::ReloadLangItems() {
    QtReloadLangTexts(*this);
    // MenuPage.cpp derives the other shell's caption from the translated first
    // control and IDS_PROP_BIT64. The controls remain unavailable until P2-09.
    UString s = LangString(IDX_SYSTEM_INTEGRATE_TO_MENU);
    UString bit64 = LangString(IDS_PROP_BIT64);
    if (bit64.IsEmpty())
        bit64 = "64-bit";
    if (_services ? _services->Is64Bit() : sizeof(void *) == 8)
        bit64.Replace(L"64", L"32");
    s.Add_Space();
    s.Add_Char('(');
    s += bit64;
    s.Add_Char(')');
    auto *integration32 = findChild<QCheckBox *>("shellIntegration32");
    integration32->setText(QtWrapControlText(*integration32, QString::fromWCharArray(s)));
    for (unsigned i = 0; i < Z7_ARRAY_SIZE(kMenuItems); i++)
        _listView->item(int(i))->setText(QString::fromWCharArray(GetMenuItemText(kMenuItems[i])));
    for (int i = 0; i < _zoneCombo->count(); i++) {
        const unsigned val = _zoneCombo->itemData(i).toUInt();
        UString s;
        if (val == 0) {
            LoadLang_Spec(s, 407, "No");
            s.Insert(0, L"* ");
        } else if (val == 1)
            LoadLang_Spec(s, 406, "Yes");
        else if (val == 2)
            LangString(IDT_ZONE_FOR_OFFICE, s);
        if (s.IsEmpty())
            s.Add_UInt32(val);
        _zoneCombo->setItemText(i, QString::fromWCharArray(s));
    }
}

bool CMenuPage::OnInit() {
    _initMode = true;
    Clear_MenuChanged();
    _dlls[0].ctrl = IDX_SYSTEM_INTEGRATE_TO_MENU;
    _dlls[1].ctrl = IDX_SYSTEM_INTEGRATE_TO_MENU_2;
    _dlls[0].wow = 0;
    _dlls[1].wow =
        _services && _services->Is64Bit() ? NQtShellRegistry::kWow32 : NQtShellRegistry::kWow64;
    const FString prefix = _services ? _services->GetModuleDirPrefix() : FString();
    for (unsigned d = 0; d < 2; d++) {
        CShellDll &dll = _dlls[d];
        dll.wasChanged = false;
        auto *control = ShellControl(dll.ctrl);
        if (_services && !_services->Is64Bit() && d != 0 && !_services->OtherBitnessAvailable()) {
            control->hide();
            dll.Path.Empty();
            continue;
        }
        control->show();
        dll.Path = prefix;
        if (_services)
            dll.Path += _services->GetDllName(d);
        if (!_services || !_services->DoesFileExist(dll.Path)) {
            dll.Path.Empty();
            control->setEnabled(false);
        } else {
            dll.prevValue = _services->CheckContextMenuHandler(fs2us(dll.Path), dll.wow);
            control->setChecked(dll.prevValue);
            control->setEnabled(true);
            QtBindLocalToolTip(control, {}, {});
        }
    }
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
                    LangString(IDT_ZONE_FOR_OFFICE, s);
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
        UString s = GetMenuItemText(menuItem);
        auto *item = new QListWidgetItem(QString::fromWCharArray(s), _listView);
        item->setData(Qt::UserRole, menuItem.Flag);
        item->setCheckState((ci.Flags & menuItem.Flag) != 0 ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole + 1, item->checkState());
    }
    _initMode = false;
    return true;
}

long CMenuPage::OnApply() {
    for (unsigned d = 2; d != 0;) {
        d--;
        CShellDll &dll = _dlls[d];
        if (dll.wasChanged && !dll.Path.IsEmpty()) {
            const bool newVal = ShellControl(dll.ctrl)->isChecked();
            const LONG res = _services->SetContextMenuHandler(newVal, fs2us(dll.Path), dll.wow);
            if (res != 0 && (dll.prevValue != newVal || newVal))
                _services->ShowMenuErrorMessage(res, this);
            dll.prevValue = _services->CheckContextMenuHandler(fs2us(dll.Path), dll.wow);
            ShellControl(dll.ctrl)->setChecked(dll.prevValue);
            dll.wasChanged = false;
        }
    }
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
    case IDX_SYSTEM_INTEGRATE_TO_MENU:
    case IDX_SYSTEM_INTEGRATE_TO_MENU_2:
        for (unsigned d = 0; d < 2; d++) {
            CShellDll &dll = _dlls[d];
            if (buttonID == dll.ctrl && !dll.Path.IsEmpty())
                dll.wasChanged = true;
        }
        break;
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

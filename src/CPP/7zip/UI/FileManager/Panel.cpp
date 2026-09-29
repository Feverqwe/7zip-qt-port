// Adapted from 7-Zip CPP/7zip/UI/FileManager/Panel.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "Panel.h"
#include "7zip/PropID.h"
#include "Windows/PropVariant.h"
#include "Windows/WinDefs.h"
#include <QEvent>
#include <QWidget>

CPanel::~CPanel() {
    // Native destructor folder cleanup, after stopping Qt notifications and
    // joining the per-panel external-file workers that can refer to this owner.
    Disable_Processing_Timer_Notify_StatusBar();
    if (_exitEventLauncher)
        _exitEventLauncher->Exit(true);
    CloseOpenFolders();
}

UString CPanel::GetFolderTypeID() const {
    {
        NWindows::NCOM::CPropVariant prop;
        if (_folder->GetFolderProperty(kpidType, &prop) == S_OK)
            if (prop.vt == VT_BSTR)
                return (const wchar_t *)prop.bstrVal;
    }
    return UString();
}

bool CPanel::IsFolderTypeEqTo(const char *s) const {
    return StringsAreEqual_Ascii(GetFolderTypeID(), s);
}

bool CPanel::IsRootFolder() const { return IsFolderTypeEqTo("RootFolder"); }
bool CPanel::IsFSFolder() const { return IsFolderTypeEqTo("FSFolder"); }
bool CPanel::IsFSDrivesFolder() const { return IsFolderTypeEqTo("FSDrives"); }
bool CPanel::IsAltStreamsFolder() const { return IsFolderTypeEqTo("AltStreamsFolder"); }
bool CPanel::IsArcFolder() const { return GetFolderTypeID().IsPrefixedBy_Ascii_NoCase("7-Zip"); }

bool CPanel::IsHashFolder() const {
    if (_folder) {
        NWindows::NCOM::CPropVariant prop;
        if (_folder->GetFolderProperty(kpidIsHash, &prop) == S_OK)
            if (prop.vt == VT_BOOL)
                return VARIANT_BOOLToBool(prop.boolVal);
    }
    return false;
}

UString CPanel::GetFsPath() const {
    if (IsFSDrivesFolder() && !IsDeviceDrivesPrefix() && !IsSuperDrivesPrefix())
        return UString();
    return _currentFolderPrefix;
}

void CPanel::SetFocusToList() {
    if (_listView)
        _listView->setFocus();
}

void CPanel::SetFocusToLastRememberedItem() {
    if (_lastFocusedIsList)
        SetFocusToList();
    else if (_headerComboBox)
        _headerComboBox->setFocus();
}

// Presentation binding replaces the two Win32 control handles. Switching the
// displayed list must not reset the last focused control (OnCreate defaults).
void CPanel::AttachPresentation(QWidget *list, QWidget *address) {
    if (_listView)
        _listView->removeEventFilter(this);
    if (_headerComboBox)
        _headerComboBox->removeEventFilter(this);
    _listView = list;
    _headerComboBox = address;
    if (_listView)
        _listView->installEventFilter(this);
    if (_headerComboBox)
        _headerComboBox->installEventFilter(this);
}

bool CPanel::eventFilter(QObject *watched, QEvent *event) {
    // CMyListView WM_SETFOCUS and OnNotifyComboBox CBEN_BEGINEDIT.
    if (event->type() == QEvent::FocusIn) {
        if (watched == _listView)
            _lastFocusedIsList = true;
        else if (watched == _headerComboBox)
            _lastFocusedIsList = false;
    }
    return QObject::eventFilter(watched, event);
}

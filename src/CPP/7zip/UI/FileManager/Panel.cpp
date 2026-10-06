// Adapted from 7-Zip CPP/7zip/UI/FileManager/Panel.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "Panel.h"
#include "../Agent/IFolderArchive.h"
#include "../Common/CompressCall.h"
#include "7zip/PropID.h"
#include "7zip/UI/Common/ArchiveName.h"
#include "7zip/UI/FileManager/resource.h"
#include "Common/StringConvert.h"
#include "LangUtils.h"
#include "ProgressDialog2.h"
#include "Windows/FileName.h"
#include "Windows/PropVariant.h"
#include "Windows/WinDefs.h"
#include <QComboBox>
#include <QEvent>
#include <QWidget>

HRESULT CPanel::Create(QWidget *mainWindow, QWidget *parentWindow, UINT id,
                       const UString &currentFolderPrefix, const UString &arcFormat,
                       CPanelCallback *panelCallback, CAppState *appState, bool needOpenArc,
                       COpenResult &openRes) {
    _mainWindow = mainWindow;
    _parentWindow = parentWindow;
    _processTimer = true;
    _processNotify = true;
    _processStatusBar = true;
    _panelCallback = panelCallback;
    _appState = appState;
    _baseID = id;
    _comboBoxID = _baseID + 3;
    _statusBarID = _comboBoxID + 1;
    UString cfp = currentFolderPrefix;
    if (!currentFolderPrefix.IsEmpty())
        if (currentFolderPrefix[0] == L'.') {
            FString cfpF;
            if (NWindows::NFile::NDir::MyGetFullPathName(us2fs(currentFolderPrefix), cfpF))
                cfp = fs2us(cfpF);
        }
    RINOK(BindToPath(cfp, arcFormat, openRes))
    if (needOpenArc && !openRes.ArchiveIsOpened)
        return S_OK;
    // Qt service replaces CreateEx and its WM_CREATE dispatch.
    if (!_createPresentation)
        return E_NOTIMPL;
    if (!OnCreate())
        return E_FAIL;
    PanelCreated = true;
    return S_OK;
}

bool CPanel::OnCreate() {
    _sortID = kpidName;
    _ascending = true;
    _lastFocusedIsList = true;
    const UInt32 kNumListModes = 4;
    if (_listViewMode >= kNumListModes)
        _listViewMode = kNumListModes - 1;
    // Original list/header/up/status child controls are Qt presentation. The
    // source owner retains mode, sort, focus, binding and processing state.
    if (!_createPresentation(*this))
        return false;
    RefreshListCtrl();
    return true;
}

void CPanel::ExitExternalWorkers(bool hardExit) {
    if (_exitEventLauncher)
        _exitEventLauncher->Exit(hardExit);
}

void CPanel::MessageBox_Error_LangID(UINT resourceID) const {
    MessageBox_Error(LangString(resourceID));
}

static UString ErrorHResult_To_Message(HRESULT errorCode) {
    if (errorCode == 0)
        errorCode = E_FAIL;
    return HResultToMessage(errorCode);
}

void CPanel::MessageBox_Error_HRESULT(HRESULT errorCode) const {
    MessageBox_Error(ErrorHResult_To_Message(errorCode));
}

void CPanel::MessageBox_Error_2Lines_Message_HRESULT(LPCWSTR message, HRESULT errorCode) const {
    UString m = message;
    m.Add_LF();
    m += ErrorHResult_To_Message(errorCode);
    MessageBox_Error(m);
}

void CPanel::ReleasePanel() {
    Disable_Processing_Timer_Notify_StatusBar();
    // Local per-panel launcher replaces the original global exit owner. Join
    // before folder cleanup, since its workers may still refer to this panel.
    if (_exitEventLauncher)
        _exitEventLauncher->Exit(true);
    // It's for unloading COM dll's: don't change it.
    CloseOpenFolders();
    // Win32 context-menu COM objects have no Qt owner here. Their mandatory
    // menu integration remains P0-03/P0-09/P2-09; do not add dummy COM members.
}

CPanel::~CPanel() {
    // Native destructor folder cleanup, after stopping Qt notifications and
    // joining the per-panel external-file workers that can refer to this owner.
    Disable_Processing_Timer_Notify_StatusBar();
    if (_exitEventLauncher)
        _exitEventLauncher->Exit(true);
    CloseOpenFolders();
}

bool CPanel::IsFolder_with_FsItems() const {
    if (IsFsOrPureDrivesFolder())
        return true;
#if defined(_WIN32) && !defined(UNDER_CE)
    FString prefix = us2fs(GetFsPath());
    return (prefix.Len() == NWindows::NFile::NName::GetNetworkServerPrefixSize(prefix));
#else
    return false;
#endif
}

int CPanel::FindDir_InOperatedList(const CRecordVector<UInt32> &operatedIndices) const {
    const UInt32 *indices = operatedIndices.ConstData();
    const unsigned numItems = operatedIndices.Size();
    for (unsigned i = 0; i < numItems; i++)
        // Native directory cache is loaded by full RefreshListCtrl (P0-05).
        // Query the same authoritative provider through existing IsItem_Folder
        // until that complete cache owner is connected.
        if (IsItem_Folder(indices[i]))
            return (int)i;
    return -1;
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
bool CPanel::IsNetFolder() const { return IsFolderTypeEqTo("NetFolder"); }
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

// Original AddToArchive body. The retained engine/Qt modal replaces the
// process-global CompressFiles entry; only the root prefix is platform-specific.
void CPanel::AddToArchive(IQtCompressCallServices &services) {
    if (!Is_IO_FS_Folder()) {
        MessageBox_Error_UnsupportOperation();
        return;
    }
    CRecordVector<UInt32> indices;
    Get_ItemIndices_Operated(indices);
    if (indices.Size() == 0) {
        MessageBox_Error_LangID(IDS_SELECT_FILES);
        return;
    }
    UString destCurDirPrefix = GetFsPath();
    if (IsFSDrivesFolder()) {
#ifdef _WIN32
        destCurDirPrefix = L"C:\\";
#else
        destCurDirPrefix = L"/";
#endif
    }
    UStringVector names;
    GetFilePaths(indices, names);
    UString baseName;
    const UString arcName = CreateArchiveName(names,
                                              false,   // isHash
                                              nullptr, // CFileInfo *fi
                                              baseName);
    const HRESULT res = CompressFiles(destCurDirPrefix, arcName, L"",
                                      true, // addExtension
                                      names,
                                      false, // email
                                      true,  // showDialog
                                      false, // waitFinish
                                      services);
    if (res != S_OK) {
        if (destCurDirPrefix.Len() >= MAX_PATH)
            MessageBox_Error_LangID(IDS_MESSAGE_UNSUPPORTED_OPERATION_FOR_LONG_PATH_FOLDER);
    }
    // KillSelection();
}

void CPanel::GetFilePaths(const CRecordVector<UInt32> &operatedIndices,
                          UStringVector &paths) const {
    paths.ClearAndReserve(operatedIndices.Size());
    UString path = GetFsPath();
    const unsigned prefixLen = path.Len();
    const UInt32 *indices = operatedIndices.ConstData();
    const unsigned numItems = operatedIndices.Size();
    for (unsigned i = 0; i < numItems; i++) {
        path.DeleteFrom(prefixLen);
        Add_ItemRelPath2_To_String(indices[i], path);
        paths.AddInReserved(path);
    }
}

void CPanel::SetFocusToList() {
    if (_listView)
        _listView->setFocus();
}

void CPanel::SetFocusToPath() {
    if (_headerComboBox) {
        _headerComboBox->setFocus();
        if (auto *combo = qobject_cast<QComboBox *>(_headerComboBox.data()))
            combo->showPopup();
    }
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

// Source Panel.cpp::TestArchives archive-folder branch. Filesystem batch
// entry point remains outside the currently exposed open-archive command.
void CPanel::TestArchives() {
    CRecordVector<UInt32> indices;
    Get_ItemIndices_OperSmart(indices);
    CMyComPtr<IArchiveFolder> archiveFolder;
    _folder.QueryInterface(IID_IArchiveFolder, &archiveFolder);
    if (archiveFolder) {
        CCopyToOptions options;
        options.streamMode = true;
        options.showErrorMessages = true;
        options.testMode = true;

        UStringVector messages;
        HRESULT res = CopyTo(options, indices, &messages);
        if (res != S_OK) {
            if (res != E_ABORT)
                MessageBox_Error_HRESULT(res);
        }
        return;
    }
    MessageBox_Error_UnsupportOperation();
}

bool CPanel::eventFilter(QObject *watched, QEvent *event) {
    // CMyListView WM_SETFOCUS and OnNotifyComboBox CBEN_BEGINEDIT.
    if (event->type() == QEvent::FocusIn) {
        if (watched == _listView)
            _lastFocusedIsList = true;
        else if (watched == _headerComboBox)
            _lastFocusedIsList = false;
        if (_panelCallback && _processNotify)
            _panelCallback->PanelWasFocused();
    }
    return QObject::eventFilter(watched, event);
}

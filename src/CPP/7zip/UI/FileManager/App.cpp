// Adapted from 7-Zip CPP/7zip/UI/FileManager/App.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// GetItemsInfoString and its helpers retain upstream control flow. Qt resource
// strings adapt language access; IFolder and variant ownership stay upstream.
#include "App.h"
#include "7zip/PropID.h"
#include "AppState.h"
#include "CopyDialog.h"
#include "Panel.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "qtfilemanagercommandservices.h"
#include "qtpanelfolder.h"
#include <QWidget>

CApp::CApp() : _qtOperations(Panels) { SetPanels_AutoRefresh_Mode(); }

CApp::~CApp() { ReleaseApp(); }

bool CApp::ExecuteCommand(unsigned id) {
    return !_qtOperations.closed() && _qtCommandServices && WndProc(id, *this, *_qtCommandServices);
}

void CPanelCallbackImp::OnTab() {
    if (_app->NumPanels != 1)
        _app->Panels[1 - _index].SetFocusToList();
    _app->RefreshTitle();
}

void CPanelCallbackImp::SetFocusToPath(unsigned index) {
    unsigned newPanelIndex = index;
    if (_app->NumPanels == 1)
        newPanelIndex = _app->LastFocusedPanel;
    _app->RefreshTitle();
    _app->Panels[newPanelIndex].SetFocusToPath();
}

void CPanelCallbackImp::OnCopy(bool move, bool copyToSame) {
    _app->OnCopy(move, copyToSame, _index);
}
void CPanelCallbackImp::OnSetSameFolder() { _app->OnSetSameFolder(_index); }
void CPanelCallbackImp::OnSetSubFolder() { _app->OnSetSubFolder(_index); }
void CPanelCallbackImp::PanelWasFocused() {
    _app->SetFocusedPanel(_index);
    _app->RefreshTitlePanel(_index);
}
void CPanelCallbackImp::DragBegin() { _app->DragBegin(_index); }
void CPanelCallbackImp::DragEnd() { _app->DragEnd(); }
void CPanelCallbackImp::RefreshTitle(bool always) { _app->RefreshTitlePanel(_index, always); }

void CApp::RefreshTitle(bool always) {
    if (_qtOperations.closed())
        return;
    UString path = GetFocusedPanel().GetCurrentFolderPrefix();
    if (path.IsEmpty())
        path = "7-Zip";
    if (!always && path == PrevTitle)
        return;
    PrevTitle = path;
    if (_window)
        _window->setWindowTitle(ToQString(path));
}

void CApp::RefreshTitlePanel(unsigned index, bool always) {
    if (index != GetFocusedPanelIndex())
        return;
    RefreshTitle(always);
}

void CApp::OnCopy(bool move, bool copyToSame, unsigned index) {
    if (_presentation)
        if (auto *callback = _presentation->PanelCallback(index))
            callback->OnCopy(move, copyToSame);
}

void CApp::OnSetSameFolder(unsigned index) {
    if (_presentation)
        if (auto *callback = _presentation->PanelCallback(index))
            callback->OnSetSameFolder();
}

void CApp::OnSetSubFolder(unsigned index) {
    if (_presentation)
        if (auto *callback = _presentation->PanelCallback(index))
            callback->OnSetSubFolder();
}

void CApp::DragBegin(unsigned index) {
    if (_presentation)
        if (auto *callback = _presentation->PanelCallback(index))
            callback->DragBegin();
}

void CApp::DragEnd() {
    if (_presentation)
        if (auto *callback = _presentation->PanelCallback(LastFocusedPanel))
            callback->DragEnd();
}

HRESULT CApp::CreateOnePanel(unsigned panelIndex, const UString &mainPath, const UString &arcFormat,
                             bool needOpenArc, COpenResult &openRes) {
    if (Panels[panelIndex].PanelCreated)
        return S_OK;
    if (!_presentation)
        return E_NOTIMPL;
    m_PanelCallbackImp[panelIndex].Init(this, panelIndex);
    UString path;
    if (mainPath.IsEmpty()) {
        if (!ReadPanelPath(panelIndex, path, _settingsPath))
            path.Empty();
    } else
        path = mainPath;
    const unsigned id = 1000 + 100 * panelIndex; // check it
    Panels[panelIndex].AttachCreationPresentation([this, panelIndex](CPanel &panel) {
        return _presentation->CreatePanel(panel, panelIndex);
    });
    return Panels[panelIndex].Create(_window, _window, id, path, arcFormat,
                                     &m_PanelCallbackImp[panelIndex], &AppState, needOpenArc,
                                     openRes);
}

void CApp::SetListSettings() {
    CFmSettings st;
    st.Load(_settingsPath);
    ShowSystemMenu = st.ShowSystemMenu;
    for (unsigned i = 0; i < kNumPanelsMax; i++) {
        CPanel &panel = Panels[i];
        panel.SetAlternativeSelection(st.AlternativeSelection);
        panel._showDots = st.ShowDots;
        panel._showRealFileIcons = st.ShowRealFileIcons;
        // Qt view styles replace LVS_EX/LVS_SINGLESEL bit fields.
        if (_presentation)
            _presentation->SetListSettings(i, st);
    }
}

void CApp::ReadToolbar(bool smallScreen) {
    const UInt32 mask = ReadToolbarsMask(_settingsPath);
    if (mask & ((UInt32)1 << 31)) {
        ShowButtonsLables = !smallScreen;
        LargeButtons = false;
        ShowStandardToolbar = ShowArchiveToolbar = true;
    } else {
        ShowButtonsLables = ((mask & 1) != 0);
        LargeButtons = ((mask & 2) != 0);
        ShowStandardToolbar = ((mask & 4) != 0);
        ShowArchiveToolbar = ((mask & 8) != 0);
    }
}
void CApp::SaveToolbar() {
    UInt32 mask = 0;
    if (ShowButtonsLables)
        mask |= 1;
    if (LargeButtons)
        mask |= 2;
    if (ShowStandardToolbar)
        mask |= 4;
    if (ShowArchiveToolbar)
        mask |= 8;
    SaveToolbarsMask(mask, _settingsPath);
}
void CApp::ReloadToolbars() {
    if (_presentation)
        _presentation->ReloadToolbars(*this);
}
void CApp::MoveSubWindows() {
    if (_presentation)
        _presentation->MoveSubWindows();
}
void CApp::SaveToolbarChanges() {
    SaveToolbar();
    ReloadToolbars();
    MoveSubWindows();
}
void CApp::SwitchStandardToolbar() {
    ShowStandardToolbar = !ShowStandardToolbar;
    SaveToolbarChanges();
}
void CApp::SwitchArchiveToolbar() {
    ShowArchiveToolbar = !ShowArchiveToolbar;
    SaveToolbarChanges();
}
void CApp::SwitchButtonsLables() {
    ShowButtonsLables = !ShowButtonsLables;
    SaveToolbarChanges();
}
void CApp::SwitchLargeButtons() {
    LargeButtons = !LargeButtons;
    SaveToolbarChanges();
}

HRESULT CApp::Create(QWidget *window, const UString &mainPath, const UString &arcFormat,
                     int xSizes[2], bool needOpenArc, COpenResult &openRes,
                     IQtAppPresentation &presentation, const QString &settingsPath) {
    _qtOperations.ResetForCreate();
    _qtCommandServices = nullptr;
    PrevTitle.Empty();
    _window = window;
    _presentation = &presentation;
    _settingsPath = settingsPath;
    // Qt child/menu construction replaces Win32 handles, not toolbar state.
    _presentation->Initialize(*this);
    ReadToolbar();
    ReloadToolbars();
    unsigned i;
    for (i = 0; i < kNumPanelsMax; i++)
        Panels[i].PanelCreated = false;
    AppState.Read(_settingsPath);
    SetListSettings();
    if (LastFocusedPanel >= kNumPanelsMax)
        LastFocusedPanel = 0;
    CListMode listMode;
    listMode.Read(_settingsPath);
    for (i = 0; i < kNumPanelsMax; i++) {
        CPanel &panel = Panels[i];
        panel._listViewMode = listMode.Panels[i];
        panel._xSize = xSizes[i];
        panel._flatModeForArc = ReadFlatView(i, _settingsPath);
    }
    for (i = 0; i < kNumPanelsMax; i++) {
        unsigned panelIndex = i;
        if (needOpenArc && LastFocusedPanel == 1)
            panelIndex = 1 - i;
        bool isMainPanel = (panelIndex == LastFocusedPanel);
        if (NumPanels > 1 || isMainPanel) {
            if (NumPanels == 1)
                Panels[panelIndex]._xSize = xSizes[0] + xSizes[1];
            COpenResult openRes2;
            UString path;
            if (isMainPanel)
                path = mainPath;
            RINOK(CreateOnePanel(panelIndex, path, arcFormat, isMainPanel && needOpenArc,
                                 *(isMainPanel ? &openRes : &openRes2)))
            if (isMainPanel) {
                if (needOpenArc && !openRes.ArchiveIsOpened)
                    return S_OK;
            }
        }
    }
    SetFocusedPanel(LastFocusedPanel);
    Panels[LastFocusedPanel].SetFocusToList();
    return S_OK;
}

void CApp::Save() {
    AppState.Save(_settingsPath);
    CListMode listMode;
    for (unsigned i = 0; i < kNumPanelsMax; i++) {
        const CPanel &panel = Panels[i];
        UString path;
        if (panel._parentFolders.IsEmpty())
            path = panel._currentFolderPrefix;
        else
            path = panel._parentFolders[0].ParentFolderPath;
        SavePanelPath(i, path, _settingsPath);
        listMode.Panels[i] = panel.GetListViewMode();
        SaveFlatView(i, panel._flatModeForArc, _settingsPath);
    }
    listMode.Save(_settingsPath);
}

void CApp::ReleaseApp() {
    _qtOperations.Close();
    _qtCommandServices = nullptr;
    // 24.09: ReleasePanel() will stop panel timer processing.
    // but we want to stop timer processing for all panels
    // before ReleasePanel() calling.
    unsigned i;
    for (i = 0; i < kNumPanelsMax; i++)
        Panels[i].Disable_Processing_Timer_Notify_StatusBar();
    // It's for unloading COM dll's: don't change it.
    for (i = 0; i < kNumPanelsMax; i++)
        Panels[i].ReleasePanel();
}

static void AddUniqueStringToHead(UStringVector &list, const UString &s) {
    for (unsigned i = 0; i < list.Size();)
        if (s.IsEqualTo_NoCase(list[i]))
            list.Delete(i);
        else
            i++;
    list.Insert(0, s);
}

void CFolderHistory::Normalize() {
    const unsigned kMaxSize = 100;
    if (Strings.Size() > kMaxSize)
        Strings.DeleteFrom(kMaxSize);
}

void CFolderHistory::AddString(const UString &s) {
    AddUniqueStringToHead(Strings, s);
    Normalize();
}

CPanel::CPanel(const QString &prefix, const CObjectVector<CPanelItem> &items)
    : _currentFolderPrefix(ToUString(prefix)) {
    SetNewFolder(new QtPanelFolder(prefix, items));
}

CPanel::CPanel(IFolderFolder *folder) {
    SetNewFolder(folder);
    if (_folder)
        _currentFolderPrefix = GetFolderPath(_folder);
}

static void AddSizeValue(QString &s, UInt64 size) {
    s += QStringLiteral("%1 байт").arg(ConvertSizeToString(size));
}

static void AddValuePair1(QString &s, const QString &label, UInt64 size) {
    s += label;
    s += ": ";
    AddSizeValue(s, size);
    s += '\n';
}

void AddValuePair2(QString &s, const QString &label, UInt64 num, UInt64 size) {
    if (num == 0)
        return;
    s += label;
    s += ": ";
    s += ConvertSizeToString(num);
    if (size != (UInt64)(Int64)-1) {
        s += "    ( ";
        AddSizeValue(s, size);
        s += " )";
    }
    s += '\n';
}

static void AddPropValueToSum(IFolderFolder *folder, UInt32 index, PROPID propID, UInt64 &sum) {
    if (sum == (UInt64)(Int64)-1)
        return;
    NWindows::NCOM::CPropVariant prop;
    folder->GetProperty(index, propID, &prop);
    UInt64 val = 0;
    if (ConvertPropVariantToUInt64(prop, val))
        sum += val;
    else
        sum = (UInt64)(Int64)-1;
}

QString CPanel::GetItemsInfoString(const CRecordVector<UInt32> &indices) const {
    QString info;
    UInt64 numDirs, numFiles, filesSize, foldersSize;
    numDirs = numFiles = filesSize = foldersSize = 0;
    unsigned i;
    for (i = 0; i < indices.Size(); i++) {
        const UInt32 index = indices[i];
        if (IsItem_Folder(index)) {
            AddPropValueToSum(_folder, index, kpidSize, foldersSize);
            numDirs++;
        } else {
            AddPropValueToSum(_folder, index, kpidSize, filesSize);
            numFiles++;
        }
    }
    AddValuePair2(info, QStringLiteral("Папок"), numDirs, foldersSize);
    AddValuePair2(info, QStringLiteral("Файлов"), numFiles, filesSize);
    int numDefined = ((foldersSize != (UInt64)(Int64)-1) && foldersSize != 0) ? 1 : 0;
    numDefined += ((filesSize != (UInt64)(Int64)-1) && filesSize != 0) ? 1 : 0;
    if (numDefined == 2)
        AddValuePair1(info, QStringLiteral("Размер"), filesSize + foldersSize);
    info += '\n';
    info += ToQString(_currentFolderPrefix);
    for (i = 0; i < indices.Size() && (int)i < (int)kCopyDialog_NumInfoLines - 6; i++) {
        info += '\n';
        info += "  ";
        const UInt32 index = indices[i];
        info += ToQString(GetItemRelPath(index));
        if (IsItem_Folder(index))
            info += '/';
    }
    if (i != indices.Size()) {
        info += '\n';
        info += "  ...";
    }
    return info;
}

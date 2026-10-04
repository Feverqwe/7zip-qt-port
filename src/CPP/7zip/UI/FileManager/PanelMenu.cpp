// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelMenu.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Properties enumeration retains upstream item/raw/folder/archive-level flow.
// Qt supplies modal presentation; OS shell invocation remains a separate service.
#include "../Common/OpenArchive.h"
#include "../Explorer/ContextMenu.h"
#include "7zip/PropID.h"
#include "7zip/UI/Common/PropIDUtils.h"
#include "7zip/UI/FileManager/PropertyNameRes.h"
#include "7zip/UI/FileManager/resource.h"
#include "App.h"
#include "Common/IntToString.h"
#include "Common/StringConvert.h"
#include "ExtractCallback.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include "ListViewDialog.h"
#include "MyLoadMenu.h"
#include "Panel.h"
#include "PropertyName.h"
#include "ViewSettings.h"
#include "Windows/FileName.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "qtfilemenuservices.h"
#include "qtlangcontrols.h"
#include "qtpanelpluginservices.h"
#ifdef _WIN32
#include "Windows/Menu.h"
#include <QWidget>
#include <shlobj.h>
#endif

static const unsigned kSevenZipStartMenuID = kMenuCmdID_Plugin_Start;
static const unsigned kSystemStartMenuID = kMenuCmdID_Plugin_Start + 400;
#ifdef _WIN32
using namespace NWindows;
#define ODS(sz)
#define ODS_U(s)
struct CFolderPidls {
    LPITEMIDLIST parent;
    CRecordVector<LPITEMIDLIST> items;

    CFolderPidls() : parent(NULL) {}
    ~CFolderPidls() {
        FOR_VECTOR(i, items)
        CoTaskMemFree(items[i]);
        CoTaskMemFree(parent);
    }
};

HRESULT ShellFolder_ParseDisplayName(IShellFolder *shellFolder, HWND hwnd, const UString &path,
                                     LPITEMIDLIST *ppidl);
HRESULT ShellFolder_ParseDisplayName(IShellFolder *shellFolder, HWND hwnd, const UString &path,
                                     LPITEMIDLIST *ppidl) {
    // ULONG eaten = 0;
    return shellFolder->ParseDisplayName(hwnd, NULL, path.Ptr_non_const(),
                                         NULL, // &eaten
                                         ppidl, NULL);
}

HRESULT CPanel::CreateShellContextMenu(const CRecordVector<UInt32> &operatedIndices,
                                       CMyComPtr<IContextMenu> &systemContextMenu) {
    ODS("==== CPanel::CreateShellContextMenu");
    systemContextMenu.Release();
    UString folderPath = GetFsPath();

    CMyComPtr<IShellFolder> desktopFolder;
    RINOK(::SHGetDesktopFolder(&desktopFolder))
    if (!desktopFolder) {
        // ShowMessage("Failed to get Desktop folder");
        return E_FAIL;
    }

    CFolderPidls pidls;
    // NULL is allowed for parentHWND in ParseDisplayName()
    const HWND parentHWND_for_ParseDisplayName =
        GetParent() ? reinterpret_cast<HWND>(GetParent()->winId()) : NULL;
    // if (folderPath.IsEmpty()), then ParseDisplayName returns pidls of "My Computer"
    /* win10: ParseDisplayName() supports folder path with tail slash
      ParseDisplayName() returns {
        E_INVALIDARG         : path with super path prefix "\\\\?\\"
        ERROR_FILE_NOT_FOUND : path for network share (\\server\path1\long path2") larger than
      MAX_PATH
      } */
    const HRESULT res = ShellFolder_ParseDisplayName(desktopFolder, parentHWND_for_ParseDisplayName,
                                                     folderPath, &pidls.parent);
    if (res != S_OK) {
        ODS_U(folderPath);
        if (res != E_INVALIDARG)
            return res;
        if (!NFile::NName::If_IsSuperPath_RemoveSuperPrefix(folderPath))
            return res;
        RINOK(ShellFolder_ParseDisplayName(desktopFolder, parentHWND_for_ParseDisplayName,
                                           folderPath, &pidls.parent))
    }
    if (!pidls.parent)
        return E_FAIL;

    /*
    UString path2;
    NShell::GetPathFromIDList(pidls.parent, path2);
    ODS_U(path2);
    */

    if (operatedIndices.IsEmpty()) {
        // how to get IContextMenu, if there are no selected files?
        return E_FAIL;

        /*
        xp64 :
        1) we can't use GetUIObjectOf() with (numItems == 0), it throws exception
        2) we can't use desktopFolder->GetUIObjectOf() with absolute pidls of folder
            context menu items are different in that case:
              "Open / Explorer" for folder
              "Delete" for "My Computer" icon
              "Preperties" for "System"
        */
        /*
        parentFolder = desktopFolder;
        pidls.items.AddInReserved(pidls.parent);
        pidls.parent = NULL;
        */

        // CreateViewObject() doesn't show all context menu items
        /*
        HRESULT res = parentFolder->CreateViewObject(
            GetParent(), IID_IContextMenu, (void**)&systemContextMenu);
        */
    }

    CMyComPtr<IShellFolder> parentFolder;
    RINOK(desktopFolder->BindToObject(pidls.parent, NULL, IID_IShellFolder, (void **)&parentFolder))
    if (!parentFolder)
        return E_FAIL;

    ODS("==== CPanel::CreateShellContextMenu pidls START");

    pidls.items.ClearAndReserve(operatedIndices.Size());
    UString fileName;
    FOR_VECTOR(i, operatedIndices) {
        fileName.Empty();
        Add_ItemRelPath2_To_String(operatedIndices[i], fileName);
        /* ParseDisplayName() in win10 returns:
             E_INVALIDARG : if empty name, or path with dots only: "." , ".."
             HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) : if there is no such file
        */
        LPITEMIDLIST pidl = NULL;
        RINOK(ShellFolder_ParseDisplayName(parentFolder, parentHWND_for_ParseDisplayName, fileName,
                                           &pidl))
        if (!pidl)
            return E_FAIL;
        pidls.items.AddInReserved(pidl);
    }

    ODS("==== CPanel::CreateShellContextMenu pidls END");
    // Get IContextMenu for items
    RINOK(parentFolder->GetUIObjectOf(parentHWND_for_ParseDisplayName, pidls.items.Size(),
                                      (LPCITEMIDLIST *)(void *)pidls.items.ConstData(),
                                      IID_IContextMenu, NULL, (void **)&systemContextMenu))
    ODS("==== CPanel::CreateShellContextMenu GetUIObjectOf finished");
    if (!systemContextMenu) {
        // ShowMessage("Unable to get context menu interface");
        return E_FAIL;
    }
    return S_OK;
}
void CPanel::CreateSystemMenu(HMENU menuSpec, bool showExtendedVerbs,
                              const CRecordVector<UInt32> &operatedIndices,
                              CMyComPtr<IContextMenu> &systemContextMenu) {
    systemContextMenu.Release();

    CreateShellContextMenu(operatedIndices, systemContextMenu);

    if (!systemContextMenu)
        return;

    /*
    // Set up a CMINVOKECOMMANDINFO structure.
    CMINVOKECOMMANDINFO ci;
    ZeroMemory(&ci, sizeof(ci));
    ci.cbSize = sizeof(CMINVOKECOMMANDINFO);
    ci.hwnd = GetParent();
    */

    /*
    if (Sender == GoBtn)
    {
      // Verbs that can be used are cut, paste,
      // properties, delete, and so on.
      String action;
      if (CutRb->Checked)
        action = "cut";
      else if (CopyRb->Checked)
        action = "copy";
      else if (DeleteRb->Checked)
        action = "delete";
      else if (PropertiesRb->Checked)
        action = "properties";

      ci.lpVerb = action.c_str();
      result = cm->InvokeCommand(&ci);
      if (result)
        ShowMessage(
        "Error copying file to clipboard.");

    }
    else
    */
    {
        // HMENU hMenu = CreatePopupMenu();
        CMenu popupMenu;
        CMenuDestroyer menuDestroyer(popupMenu);
        if (!popupMenu.CreatePopup())
            throw 210503;
        const HMENU hMenu = popupMenu;
        DWORD flags = CMF_EXPLORE;
        if (showExtendedVerbs)
            flags |= 0x00000100;
        ODS("=== systemContextMenu->QueryContextMenu START");
        const HRESULT res =
            systemContextMenu->QueryContextMenu(hMenu, 0, kSystemStartMenuID, 0x7FFF, flags);
        ODS("=== systemContextMenu->QueryContextMenu END");
        if (SUCCEEDED(res)) {
#ifdef SHOW_DEBUG_FM_CTX_MENU
            PrintAllContextItems(systemContextMenu, (unsigned)res);
#endif

            CMenu menu;
            menu.Attach(menuSpec);
            CMenuItem menuItem;
            menuItem.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
            menuItem.fType = MFT_STRING;
            menuItem.hSubMenu = popupMenu.Detach();
            menuDestroyer.Disable();
            LangString(IDS_SYSTEM, menuItem.StringValue);
            menu.InsertItem(0, true, menuItem);
        }
        /*
        if (Cmd < 100 && Cmd != 0)
        {
          ci.lpVerb = MAKEINTRESOURCE(Cmd - 1);
          ci.lpParameters = "";
          ci.lpDirectory = "";
          ci.nShow = SW_SHOWNORMAL;
          cm->InvokeCommand(&ci);
        }
        // If Cmd is > 100 then it's one of our
        // inserted menu items.
        else
          // Find the menu item.
          for (int i = 0; i < popupMenu1->Items->Count; i++)
          {
            TMenuItem* menu = popupMenu1->Items->Items[i];
            // Call its OnClick handler.
            if (menu->Command == Cmd - 100)
              menu->OnClick(this);
          }
          // Release the memory allocated for the menu.
          DestroyMenu(hMenu);
        */
    }
}
#undef ODS
#undef ODS_U

HRESULT CPanel::InvokeShellContextMenu(unsigned offset, IContextMenu *systemContextMenu) {
    if (!systemContextMenu)
        return E_INVALIDARG;
    // Original InvokePluginCommand's native invocation body. Numeric routing,
    // selection clearing and error presentation remain in the shared caller.
    CMINVOKECOMMANDINFOEX commandInfo;
    memset(&commandInfo, 0, sizeof(commandInfo));
    commandInfo.cbSize = sizeof(commandInfo);
    commandInfo.fMask = CMIC_MASK_UNICODE;
    commandInfo.hwnd = GetParent() ? reinterpret_cast<HWND>(GetParent()->winId()) : NULL;
    commandInfo.lpVerb = (LPCSTR)(MAKEINTRESOURCE(offset));
    commandInfo.lpParameters = NULL;
    const AString currentFolderA(GetAnsiString(_currentFolderPrefix));
    commandInfo.lpDirectory = (LPCSTR)(currentFolderA);
    commandInfo.nShow = SW_SHOW;
    commandInfo.lpParametersW = NULL;
    commandInfo.lpTitle = "";
    // The shell dispatcher converts lpVerb only; upstream keeps lpVerbW NULL
    // for compatibility with Explorer's context-menu subhandlers.
    commandInfo.lpVerbW = NULL;
    const UString currentFolderUnicode = _currentFolderPrefix;
    commandInfo.lpDirectoryW = currentFolderUnicode;
    commandInfo.lpTitleW = L"";
    commandInfo.ptInvoke.x = 0;
    commandInfo.ptInvoke.y = 0;
    return systemContextMenu->InvokeCommand(LPCMINVOKECOMMANDINFO(&commandInfo));
}
#endif

bool CPanel::InvokePluginCommand(unsigned id, IQtPanelPluginServices &services) {
    UInt32 offset;
    const bool isSystemMenu = (id >= kSystemStartMenuID);
    if (isSystemMenu) {
        if (!services.HasContextMenu(true))
            return false;
        offset = id - kSystemStartMenuID;
    } else {
        if (!services.HasContextMenu(false))
            return false;
        offset = id - kSevenZipStartMenuID;
    }
    // CMINVOKECOMMANDINFO's numeric verb, parent and current directory are
    // transported by the native/Qt facade rather than Win32 pointers.
    const HRESULT result = services.InvokeCommand(offset, isSystemMenu, _currentFolderPrefix);
    if (result == S_OK) {
        KillSelection();
        return true;
    } else
        services.ShowInvokeError(result);
    return false;
}

void CPanel::CreateSevenZipMenu(NQtShellMenu::CMenuHandle *menuSpec, bool showExtendedVerbs,
                                const CRecordVector<UInt32> &operatedIndices, int firstDirIndex,
                                std::unique_ptr<CZipContextMenu> &sevenZipContextMenu,
                                const QString &settingsPath, IQtContextMenuServices &services) {
    sevenZipContextMenu.reset();
    NQtShellMenu::CMenu menu;
    menu.Attach(menuSpec);
    auto contextMenu = std::make_unique<CZipContextMenu>(settingsPath, &services);
    CZipContextMenu *contextMenuSpec = contextMenu.get();
    GetFilePaths(operatedIndices, contextMenuSpec->_fileNames);
    contextMenuSpec->Init_For_7zFM();
    contextMenuSpec->_attribs.FirstDirIndex = firstDirIndex;
    // Original CMF_EXPLORE / Z7_WIN_CMF_EXTENDEDVERBS, transported as values.
    unsigned flags = 4;
    if (showExtendedVerbs)
        flags |= 0x00000100;
    const HRESULT res =
        contextMenu->QueryContextMenu(menu, 0, kSevenZipStartMenuID, kSystemStartMenuID - 1, flags);
    const bool sevenZipMenuCreated = SUCCEEDED(res);
    if (sevenZipMenuCreated)
        sevenZipContextMenu = std::move(contextMenu);
}

void CPanel::CreateFileMenu(IQtFileMenuServices &menu, bool programMenu,
                            IQtPanelPluginMenuServices *plugins) {
    if (plugins)
        plugins->ReleasePluginMenus();
    const bool showExtendedVerbs = plugins && plugins->IsShiftDown();
    CRecordVector<UInt32> operatedIndices;
    Get_ItemIndices_Operated(operatedIndices);
    const int firstDirIndex = FindDir_InOperatedList(operatedIndices);
    if (!IsArcFolder() && plugins) {
        plugins->CreateSevenZipMenu(*this, showExtendedVerbs, operatedIndices, firstDirIndex);
        if (plugins->ShowSystemMenu())
            plugins->CreateSystemMenu(*this, showExtendedVerbs, operatedIndices);
    }
    CFileMenu fm;
    fm.readOnly = IsThereReadOnlyFolder();
    fm.isHashFolder = IsHashFolder();
    fm.isFsFolder = Is_IO_FS_Folder();
    fm.programMenu = programMenu;
    fm.allAreFiles = (firstDirIndex == -1);
    fm.numItems = operatedIndices.Size();
    fm.isAltStreamsSupported = false;
    if (fm.numItems == 1)
        fm.FilePath = us2fs(GetItemFullPath(operatedIndices[0]));
    if (_folderAltStreams) {
        if (operatedIndices.Size() <= 1) {
            UInt32 realIndex = (UInt32)(Int32)-1;
            if (operatedIndices.Size() == 1)
                realIndex = operatedIndices[0];
            Int32 val = 0;
            if (_folderAltStreams->AreAltStreamsSupported(realIndex, &val) == S_OK)
                fm.isAltStreamsSupported = IntToBool(val);
        }
    } else {
#if defined(_WIN32) && !defined(UNDER_CE)
        if (fm.numItems == 0)
            fm.isAltStreamsSupported = IsFSFolder();
        else
            fm.isAltStreamsSupported = IsFolder_with_FsItems();
#endif
    }
    // Qt stores/renders plugin rows separately from CFileMenu's projection;
    // its zero position is the original menu.GetItemCount() insertion point.
    fm.Load(menu, 0);
}

static const char *const kSeparator = "------------------------";
static const char *const kSeparatorSmall = "----------------";

static bool IsReadOnlyFolder(IFolderFolder *folder) {
    if (!folder)
        return false;
    bool res = false;
    {
        NWindows::NCOM::CPropVariant prop;
        if (folder->GetFolderProperty(kpidReadOnly, &prop) == S_OK)
            if (prop.vt == VT_BOOL)
                res = VARIANT_BOOLToBool(prop.boolVal);
    }
    return res;
}

bool CPanel::IsThereReadOnlyFolder() const {
    if (!_folderOperations)
        return true;
    if (IsReadOnlyFolder(_folder))
        return true;
    FOR_VECTOR(i, _parentFolders)
    if (IsReadOnlyFolder(_parentFolders[i].ParentFolder))
        return true;
    return false;
}

bool CPanel::CheckBeforeUpdate(UINT resourceID) {
    if (!_folderOperations) {
        MessageBox_Error_UnsupportOperation();
        return false;
    }
    for (int i = (int)_parentFolders.Size(); i >= 0; i--) {
        IFolderFolder *folder;
        if (i == (int)_parentFolders.Size())
            folder = _folder;
        else
            folder = _parentFolders[i].ParentFolder;
        if (!IsReadOnlyFolder(folder))
            continue;
        UString s;
        AddLangString(s, resourceID);
        s.Add_LF();
        AddLangString(s, IDS_OPERATION_IS_NOT_SUPPORTED);
        s.Add_LF();
        if (i == 0)
            s += GetFolderPath(folder);
        else
            s += _parentFolders[i - 1].VirtualPath;
        s.Add_LF();
        AddLangString(s, IDS_PROP_READ_ONLY);
        MessageBox_Error(s);
        return false;
    }
    return true;
}

static void AddListAscii(CListViewDialog &dialog, const char *s) {
    dialog.Strings.Add((UString)s);
    dialog.Values.AddNew();
}

static void AddSeparator(CListViewDialog &dialog) { AddListAscii(dialog, kSeparator); }
static void AddSeparatorSmall(CListViewDialog &dialog) { AddListAscii(dialog, kSeparatorSmall); }

static void AddPropertyPair(const UString &name, const UString &val, CListViewDialog &dialog) {
    dialog.Strings.Add(name);
    dialog.Values.Add(val);
}

static void AddPropertyString(PROPID propID, const wchar_t *nameBSTR,
                              const NWindows::NCOM::CPropVariant &prop, CListViewDialog &dialog) {
    if (prop.vt != VT_EMPTY) {
        UString val;

        if (propID == kpidErrorFlags || propID == kpidWarningFlags) {
            UInt32 flags = GetOpenArcErrorFlags(prop);
            if (flags == 0)
                return;
            if (flags != 0)
                val = GetOpenArcErrorMessage(flags);
        }

        if (val.IsEmpty()) {
            if ((prop.vt == VT_UI8 || prop.vt == VT_UI4 || prop.vt == VT_UI2) &&
                IsSizeProp(propID)) {
                UInt64 v = 0;
                ConvertPropVariantToUInt64(prop, v);
                val = ToUString(ConvertSizeToString(v));
            } else
                ConvertPropertyToString2(val, prop, propID, 9);
        }

        if (!val.IsEmpty()) {
            if (propID == kpidErrorType)
                AddPropertyPair(L"Open WARNING:", L"Cannot open the file as expected archive type",
                                dialog);
            AddPropertyPair(GetNameOfProperty(propID, nameBSTR), val, dialog);
            dialog.QtBindPropertyName(dialog.Strings.Size() - 1, propID, nameBSTR);
        }
    }
}

static void AddPropertyString(PROPID propID, UInt64 val, CListViewDialog &dialog) {
    NWindows::NCOM::CPropVariant prop = val;
    AddPropertyString(propID, NULL, prop, dialog);
}

static const Byte kSpecProps[] = {kpidPath,       kpidType,    kpidErrorType,    kpidError,
                                  kpidErrorFlags, kpidWarning, kpidWarningFlags, kpidOffset,
                                  kpidPhySize,    kpidTailSize};

void CPanel::Properties(const CRecordVector<UInt32> &operatedIndices,
                        CListViewDialog &message) const {
    CMyComPtr<IGetFolderArcProps> getFolderArcProps;
    _folder.QueryInterface(IID_IGetFolderArcProps, &getFolderArcProps);
    // Filesystem Properties uses this same portable dialog rather than the
    // Windows-only InvokeSystemCommand branch; item properties remain native.
    if (operatedIndices.Size() == 1) {
        UInt32 index = operatedIndices[0];
        UInt32 numProps;
        if (_folder->GetNumberOfProperties(&numProps) == S_OK) {
            for (UInt32 i = 0; i < numProps; i++) {
                CMyComBSTR name;
                PROPID propID;
                VARTYPE varType;
                if (_folder->GetPropertyInfo(i, &name, &propID, &varType) != S_OK)
                    continue;
                NWindows::NCOM::CPropVariant prop;
                if (_folder->GetProperty(index, propID, &prop) != S_OK)
                    continue;
                AddPropertyString(propID, name, prop, message);
            }
        }
        if (_folderRawProps) {
            _folderRawProps->GetNumRawProps(&numProps);
            for (UInt32 i = 0; i < numProps; i++) {
                CMyComBSTR name;
                PROPID propID;
                if (_folderRawProps->GetRawPropInfo(i, &name, &propID) != S_OK)
                    continue;
                const void *data;
                UInt32 dataSize;
                UInt32 propType;
                if (_folderRawProps->GetRawProp(index, propID, &data, &dataSize, &propType) != S_OK)
                    continue;
                if (dataSize != 0) {
                    AString s;
                    if (propID == kpidNtSecure)
                        ConvertNtSecureToString((const Byte *)data, dataSize, s);
                    else {
                        const unsigned kMaxDataSize = 1 << 8;
                        if (dataSize > kMaxDataSize) {
                            s += "data:";
                            s.Add_UInt32(dataSize);
                        } else {
                            char temp[kMaxDataSize * 2 + 2];
                            if (dataSize <= 8 && (propID == kpidCRC || propID == kpidChecksum))
                                ConvertDataToHex_Upper(temp, (const Byte *)data, dataSize);
                            else
                                ConvertDataToHex_Lower(temp, (const Byte *)data, dataSize);
                            s += temp;
                        }
                    }
                    AddPropertyPair(GetNameOfProperty(propID, name), (UString)s.Ptr(), message);
                }
            }
        }
        AddSeparator(message);
    } else if (operatedIndices.Size() >= 1) {
        UInt64 packSize = 0;
        UInt64 unpackSize = 0;
        UInt64 numFiles = 0;
        UInt64 numDirs = 0;
        FOR_VECTOR(i, operatedIndices) {
            const UInt32 index = operatedIndices[i];
            unpackSize += GetItemSize(index);
            packSize += GetItem_UInt64Prop(index, kpidPackSize);
            if (IsItem_Folder(index)) {
                numDirs++;
                numDirs += GetItem_UInt64Prop(index, kpidNumSubDirs);
                numFiles += GetItem_UInt64Prop(index, kpidNumSubFiles);
            } else
                numFiles++;
        }
        {
            wchar_t temp[32];
            ConvertUInt32ToString(operatedIndices.Size(), temp);
            AddPropertyPair(L"", MyFormatNew(LangString(IDS_N_SELECTED_ITEMS), temp), message);
        }
        if (numDirs != 0)
            AddPropertyString(kpidNumSubDirs, numDirs, message);
        if (numFiles != 0)
            AddPropertyString(kpidNumSubFiles, numFiles, message);
        AddPropertyString(kpidSize, unpackSize, message);
        AddPropertyString(kpidPackSize, packSize, message);
        AddSeparator(message);
    }
    {
        NWindows::NCOM::CPropVariant prop;
        if (_folder->GetFolderProperty(kpidPath, &prop) == S_OK)
            AddPropertyString(kpidName, L"Path", prop, message);
    }
    CMyComPtr<IFolderProperties> folderProperties;
    _folder.QueryInterface(IID_IFolderProperties, &folderProperties);
    if (folderProperties) {
        UInt32 numProps;
        if (folderProperties->GetNumberOfFolderProperties(&numProps) == S_OK) {
            for (UInt32 i = 0; i < numProps; i++) {
                CMyComBSTR name;
                PROPID propID;
                VARTYPE vt;
                if (folderProperties->GetFolderPropertyInfo(i, &name, &propID, &vt) != S_OK)
                    continue;
                NWindows::NCOM::CPropVariant prop;
                if (_folder->GetFolderProperty(propID, &prop) != S_OK)
                    continue;
                AddPropertyString(propID, name, prop, message);
            }
        }
    }
    if (getFolderArcProps) {
        CMyComPtr<IFolderArcProps> getProps;
        getFolderArcProps->GetFolderArcProps(&getProps);
        if (getProps) {
            UInt32 numLevels;
            if (getProps->GetArcNumLevels(&numLevels) != S_OK)
                numLevels = 0;
            for (UInt32 level2 = 0; level2 < numLevels; level2++) {
                {
                    UInt32 level = numLevels - 1 - level2;
                    UInt32 numProps;
                    if (getProps->GetArcNumProps(level, &numProps) == S_OK) {
                        const int kNumSpecProps = Z7_ARRAY_SIZE(kSpecProps);
                        AddSeparator(message);
                        for (Int32 i = -(int)kNumSpecProps; i < (Int32)numProps; i++) {
                            CMyComBSTR name;
                            PROPID propID;
                            VARTYPE vt;
                            if (i < 0)
                                propID = kSpecProps[i + kNumSpecProps];
                            else if (getProps->GetArcPropInfo(level, (UInt32)i, &name, &propID,
                                                              &vt) != S_OK)
                                continue;
                            NWindows::NCOM::CPropVariant prop;
                            if (getProps->GetArcProp(level, propID, &prop) != S_OK)
                                continue;
                            AddPropertyString(propID, name, prop, message);
                        }
                    }
                }
                if (level2 < numLevels - 1) {
                    const UInt32 level = numLevels - 1 - level2;
                    UInt32 numProps;
                    if (getProps->GetArcNumProps2(level, &numProps) == S_OK) {
                        AddSeparatorSmall(message);
                        for (UInt32 i = 0; i < numProps; i++) {
                            CMyComBSTR name;
                            PROPID propID;
                            VARTYPE vt;
                            if (getProps->GetArcPropInfo2(level, i, &name, &propID, &vt) != S_OK)
                                continue;
                            NWindows::NCOM::CPropVariant prop;
                            if (getProps->GetArcProp2(level, propID, &prop) != S_OK)
                                continue;
                            AddPropertyString(propID, name, prop, message);
                        }
                    }
                }
            }
            {
                bool needSep = true;
                const int kNumSpecProps = Z7_ARRAY_SIZE(kSpecProps);
                for (Int32 i = -(int)kNumSpecProps; i < 0; i++) {
                    CMyComBSTR name;
                    const PROPID propID = kSpecProps[i + kNumSpecProps];
                    NWindows::NCOM::CPropVariant prop;
                    if (getProps->GetArcProp(numLevels, propID, &prop) != S_OK)
                        continue;
                    if (needSep) {
                        AddSeparator(message);
                        AddSeparator(message);
                        needSep = false;
                    }
                    AddPropertyString(propID, name, prop, message);
                }
            }
        }
    }
    message.Title = LangString(IDS_PROPERTIES);
    QtBindLangText(&message, IDS_PROPERTIES);
    message.NumColumns = 2;
}

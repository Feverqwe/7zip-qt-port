// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelMenu.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Properties enumeration retains upstream item/raw/folder/archive-level flow.
// Qt supplies modal presentation; OS shell invocation remains a separate service.
#include "../Common/OpenArchive.h"
#include "7zip/PropID.h"
#include "7zip/UI/Common/PropIDUtils.h"
#include "7zip/UI/FileManager/PropertyNameRes.h"
#include "7zip/UI/FileManager/resource.h"
#include "Common/IntToString.h"
#include "ExtractCallback.h"
#include "FormatUtils.h"
#include "LangUtils.h"
#include "ListViewDialog.h"
#include "MyLoadMenu.h"
#include "Panel.h"
#include "PropertyName.h"
#include "ViewSettings.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"
#include "qtfilemenuservices.h"
#include "qtlangcontrols.h"

void CPanel::CreateFileMenu(IQtFileMenuServices &menu, bool programMenu) {
    CRecordVector<UInt32> operatedIndices;
    Get_ItemIndices_Operated(operatedIndices);
    const int firstDirIndex = FindDir_InOperatedList(operatedIndices);
    // Windows shell/plugin context-menu objects are mandatory P0-09/P2-09.
    // This source-owned policy applies to the current Qt command items.
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

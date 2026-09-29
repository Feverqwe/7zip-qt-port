// Adapted from 7-Zip CPP/7zip/UI/FileManager/PanelMenu.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Properties single-item enumeration and multi-item totals retain upstream
// control flow. IFolder supplies the bound owner's typed properties. Raw/archive-level
// properties and Windows shell invocation are outside this partial port.
#include "../Common/OpenArchive.h"
#include "7zip/PropID.h"
#include "7zip/UI/Common/PropIDUtils.h"
#include "ExtractCallback.h"
#include "ListViewDialog.h"
#include "Panel.h"
#include "PropertyName.h"
#include "ViewSettings.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConv.h"

static const char *const kSeparator = "------------------------";

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

static void AddListAscii(CListViewDialog &dialog, const char *s) {
    dialog.Strings.Add((UString)s);
    dialog.Values.AddNew();
}

static void AddSeparator(CListViewDialog &dialog) { AddListAscii(dialog, kSeparator); }

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
        }
    }
}

static void AddPropertyString(PROPID propID, UInt64 val, CListViewDialog &dialog) {
    NWindows::NCOM::CPropVariant prop = val;
    AddPropertyString(propID, NULL, prop, dialog);
}

void CPanel::Properties(const CRecordVector<UInt32> &operatedIndices,
                        CListViewDialog &message) const {
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
        AddPropertyPair(
            UString(),
            ToUString(QStringLiteral("Выбрано элементов: %1").arg(operatedIndices.Size())),
            message);
        if (numDirs != 0)
            AddPropertyString(kpidNumSubDirs, numDirs, message);
        if (numFiles != 0)
            AddPropertyString(kpidNumSubFiles, numFiles, message);
        AddPropertyString(kpidSize, unpackSize, message);
        AddPropertyString(kpidPackSize, packSize, message);
        AddSeparator(message);
    }
    AddPropertyPair(ToUString(QStringLiteral("Путь")), _currentFolderPrefix, message);
    message.Title = ToUString(QStringLiteral("Свойства"));
    message.NumColumns = 2;
}

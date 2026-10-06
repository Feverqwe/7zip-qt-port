#include "panel_load_folder.h"
#include "Windows/PropVariant.h"

Z7_COM7F_IMF(LoadFolder::SetFlatMode(Int32)) {
    calls << "flat";
    return S_OK;
}
Z7_COM7F_IMF(LoadFolder::LoadItems()) {
    calls << "load";
    countResult = loadedCountResult;
    return loadResult;
}
Z7_COM7F_IMF(LoadFolder::GetNumberOfItems(UInt32 *count)) {
    *count = 2;
    return countResult;
}
Z7_COM7F_IMF(LoadFolder::WasChanged(Int32 *wasChanged)) {
    *wasChanged = BoolToInt(changed);
    changed = false;
    return S_OK;
}
Z7_COM7F_IMF(LoadFolder::GetNumberOfProperties(UInt32 *count)) {
    calls << "columns";
    *count = 1;
    return S_OK;
}
Z7_COM7F_IMF(LoadFolder::GetPropertyInfo(UInt32, BSTR *name, PROPID *id, VARTYPE *type)) {
    *name = nullptr;
    *id = kpidName;
    *type = VT_BSTR;
    return S_OK;
}
Z7_COM7F_IMF(LoadFolder::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidName) {
        ++namePropertyCalls;
        if (nameResult != S_OK)
            return nameResult;
        prop = reverseNames ? (index ? UString(L"a 文件") : UString(L"z δ"))
                            : (index ? secondName : firstName);
    } else if (id == kpidPrefix) {
        ++prefixPropertyCalls;
        if (prefixResult != S_OK)
            return prefixResult;
        prop = prefix;
    } else if (id == kpidIsDir) {
        calls << "directory";
        if (directoryResult != S_OK)
            return directoryResult;
        if (directoryType == VT_BOOL)
            prop = directory;
        else if (directoryType != VT_EMPTY)
            prop = UInt32(1);
    } else if (id == kpidAttrib) {
        ++attributeCalls;
        if (attributeResult != S_OK)
            return attributeResult;
        prop = attributes;
    }
    return prop.Detach(value);
}
Z7_COM7F_IMF(LoadFolder::GetFolderProperty(PROPID id, PROPVARIANT *value)) {
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidType)
        prop = folderType;
    else if (id == kpidPath && !folderPath.IsEmpty())
        prop = folderPath;
    return prop.Detach(value);
}
Z7_COM7F_IMF(LoadFolder::BindToFolder(UInt32, IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}
Z7_COM7F_IMF(LoadFolder::BindToFolder(const wchar_t *, IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}
Z7_COM7F_IMF(LoadFolder::BindToParentFolder(IFolderFolder **folder)) {
    *folder = parentFolder;
    if (*folder)
        (*folder)->AddRef();
    return S_OK;
}
Z7_COM7F_IMF(LoadFolder::GetSystemIconIndex(UInt32, Int32 *index)) {
    ++iconCalls;
    *index = iconIndex;
    return iconResult;
}
Z7_COM7F_IMF(LoadFolder::GetItemName(UInt32 index, const wchar_t **name, unsigned *len)) {
    const UString &value = index ? secondName : firstName;
    *name = fastNameDefined ? value.Ptr() : nullptr;
    *len = fastNameDefined ? value.Len() : 0;
    return S_OK;
}
Z7_COM7F_IMF(LoadFolder::GetItemPrefix(UInt32, const wchar_t **name, unsigned *len)) {
    *name = fastPrefixDefined ? prefix.Ptr() : nullptr;
    *len = fastPrefixDefined ? prefix.Len() : 0;
    return S_OK;
}
Z7_COM7F_IMF2(UInt64, LoadFolder::GetItemSize(UInt32)) { return 0; }

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
    if (id == kpidName)
        prop =
            reverseNames ? (index ? L"a 文件" : L"z δ") : (index ? L"second 文件.txt" : L"first δ");
    else if (id == kpidIsDir) {
        calls << "directory";
        if (directoryResult != S_OK)
            return directoryResult;
        if (directoryType == VT_BOOL)
            prop = directory;
        else if (directoryType != VT_EMPTY)
            prop = UInt32(1);
    }
    return prop.Detach(value);
}
Z7_COM7F_IMF(LoadFolder::GetFolderProperty(PROPID id, PROPVARIANT *value)) {
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidType)
        prop = L"RootFolder";
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
    *folder = nullptr;
    return S_OK;
}

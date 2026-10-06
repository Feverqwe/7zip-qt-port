#include "deleted_folder.h"
#include "Windows/PropVariant.h"

Z7_COM7F_IMF(DeletedFolder::LoadItems()) { return items->LoadItems(); }
Z7_COM7F_IMF(DeletedFolder::GetNumberOfItems(UInt32 *count)) {
    return items->GetNumberOfItems(count);
}
Z7_COM7F_IMF(DeletedFolder::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    if (id != kpidIsDeleted)
        return items->GetProperty(index, id, value);
    ++deletedQueries;
    if (deletedResult != S_OK)
        return deletedResult;
    NWindows::NCOM::CPropVariant prop;
    if (deletedType == VT_BOOL)
        prop = index == 0 && firstDeleted;
    else if (deletedType != VT_EMPTY)
        prop = UInt32(1);
    return prop.Detach(value);
}
Z7_COM7F_IMF(DeletedFolder::GetFolderProperty(PROPID id, PROPVARIANT *value)) {
    return items->GetFolderProperty(id, value);
}
Z7_COM7F_IMF(DeletedFolder::BindToFolder(UInt32 index, IFolderFolder **folder)) {
    return items->BindToFolder(index, folder);
}
Z7_COM7F_IMF(DeletedFolder::BindToFolder(const wchar_t *name, IFolderFolder **folder)) {
    return items->BindToFolder(name, folder);
}
Z7_COM7F_IMF(DeletedFolder::BindToParentFolder(IFolderFolder **folder)) {
    return items->BindToParentFolder(folder);
}
Z7_COM7F_IMF(DeletedFolder::GetNumberOfProperties(UInt32 *count)) {
    return items->GetNumberOfProperties(count);
}
Z7_COM7F_IMF(DeletedFolder::GetPropertyInfo(UInt32 index, BSTR *name, PROPID *id, VARTYPE *type)) {
    return items->GetPropertyInfo(index, name, id, type);
}
Z7_COM7F_IMF(DeletedFolder::GetFolderArcProps(IFolderArcProps **object)) {
    *object = this;
    AddRef();
    return S_OK;
}
Z7_COM7F_IMF(DeletedFolder::GetArcNumLevels(UInt32 *count)) {
    *count = 1;
    return S_OK;
}
Z7_COM7F_IMF(DeletedFolder::GetArcProp(UInt32, PROPID id, PROPVARIANT *value)) {
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidIsDeleted)
        prop = archiveHasDeleted;
    return prop.Detach(value);
}
Z7_COM7F_IMF(DeletedFolder::GetArcNumProps(UInt32, UInt32 *count)) {
    *count = 0;
    return S_OK;
}
Z7_COM7F_IMF(DeletedFolder::GetArcPropInfo(UInt32, UInt32, BSTR *, PROPID *, VARTYPE *)) {
    return E_NOTIMPL;
}
Z7_COM7F_IMF(DeletedFolder::GetArcProp2(UInt32, PROPID, PROPVARIANT *)) { return E_NOTIMPL; }
Z7_COM7F_IMF(DeletedFolder::GetArcNumProps2(UInt32, UInt32 *count)) {
    *count = 0;
    return S_OK;
}
Z7_COM7F_IMF(DeletedFolder::GetArcPropInfo2(UInt32, UInt32, BSTR *, PROPID *, VARTYPE *)) {
    return E_NOTIMPL;
}

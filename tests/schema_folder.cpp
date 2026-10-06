#include "schema_folder.h"
#include "Windows/PropVariant.h"

Z7_COM7F_IMF(SchemaFolder::LoadItems()) { return items->LoadItems(); }
Z7_COM7F_IMF(SchemaFolder::GetNumberOfItems(UInt32 *count)) {
    return items->GetNumberOfItems(count);
}
Z7_COM7F_IMF(SchemaFolder::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    if (id != kpidCRC)
        return items->GetProperty(index, id, value);
    NWindows::NCOM::CPropVariant prop = UInt32(0x12345678);
    return prop.Detach(value);
}
Z7_COM7F_IMF(SchemaFolder::GetFolderProperty(PROPID id, PROPVARIANT *value)) {
    return items->GetFolderProperty(id, value);
}
Z7_COM7F_IMF(SchemaFolder::BindToFolder(UInt32 index, IFolderFolder **folder)) {
    return items->BindToFolder(index, folder);
}
Z7_COM7F_IMF(SchemaFolder::BindToFolder(const wchar_t *name, IFolderFolder **folder)) {
    return items->BindToFolder(name, folder);
}
Z7_COM7F_IMF(SchemaFolder::BindToParentFolder(IFolderFolder **folder)) {
    return items->BindToParentFolder(folder);
}
Z7_COM7F_IMF(SchemaFolder::GetNumberOfProperties(UInt32 *count)) {
    *count = 3;
    return S_OK;
}
Z7_COM7F_IMF(SchemaFolder::GetPropertyInfo(UInt32 index, BSTR *name, PROPID *id, VARTYPE *type)) {
    if (index >= 3)
        return E_INVALIDARG;
    *name = nullptr;
    *id = index == 0 ? kpidName : index == 1 ? kpidSize : kpidCRC;
    *type = index == 0 ? VT_BSTR : index == 1 ? VT_UI8 : VT_UI4;
    return S_OK;
}
Z7_COM7F_IMF(SchemaFolder::GetNumRawProps(UInt32 *count)) {
    *count = 1;
    return S_OK;
}
Z7_COM7F_IMF(SchemaFolder::GetRawPropInfo(UInt32 index, BSTR *name, PROPID *id)) {
    if (index != 0)
        return E_INVALIDARG;
    *name = nullptr;
    *id = rawId;
    return S_OK;
}
Z7_COM7F_IMF(SchemaFolder::GetRawProp(UInt32 index, PROPID id, const void **data, UInt32 *size,
                                      UInt32 *type)) {
    if (index >= 2 || id != rawId)
        return E_INVALIDARG;
    ++rawQueries;
    if (rawResult != S_OK)
        return rawResult;
    static const Byte bytes[] = {0xde, 0xad, 0xbe, 0xef};
    *data = bytes;
    *size = sizeof(bytes);
    *type = NPropDataType::kRaw;
    return S_OK;
}
Z7_COM7F_IMF(SchemaFolder::GetParent(UInt32, UInt32 *, UInt32 *)) { return E_NOTIMPL; }

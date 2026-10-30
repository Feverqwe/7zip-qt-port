#include "archive/support/schema_folder.h"
#include "Windows/PropVariant.h"

Z7_COM7F_IMF(SchemaFolder::LoadItems()) { return items->LoadItems(); }
Z7_COM7F_IMF(SchemaFolder::GetNumberOfItems(UInt32 *count)) {
    ++countQueries;
    return items->GetNumberOfItems(count);
}
Z7_COM7F_IMF(SchemaFolder::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    if (id != kpidCRC)
        return items->GetProperty(index, id, value);
    if (crcResult != S_OK)
        return crcResult;
    NWindows::NCOM::CPropVariant prop = UInt32(0x12345678);
    if (!crcText.IsEmpty())
        prop = crcText;
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
    if (writePropertyCount)
        *count = UInt32(properties.size());
    return propertyCountResult;
}
Z7_COM7F_IMF(SchemaFolder::GetPropertyInfo(UInt32 index, BSTR *name, PROPID *id, VARTYPE *type)) {
    ++propertyInfoQueries;
    if (index >= unsigned(properties.size()))
        return E_INVALIDARG;
    const auto &property = properties[int(index)];
    if (property.result != S_OK)
        return property.result;
    *name =
        property.name.IsEmpty() ? nullptr : ::SysAllocStringLen(property.name, property.name.Len());
    *id = property.id;
    *type = property.type;
    return S_OK;
}
Z7_COM7F_IMF(SchemaFolder::GetNumRawProps(UInt32 *count)) {
    if (writeRawCount)
        *count = rawEnabled ? 1 : 0;
    return rawCountResult;
}
Z7_COM7F_IMF(SchemaFolder::GetRawPropInfo(UInt32 index, BSTR *name, PROPID *id)) {
    ++rawInfoQueries;
    if (index != 0 || !rawEnabled)
        return E_INVALIDARG;
    if (rawInfoResult != S_OK)
        return rawInfoResult;
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

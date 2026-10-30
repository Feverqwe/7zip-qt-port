#include "filemanager/panels/qtpanelfolder.h"

// Console/List.cpp emits raw checksum bytes as hex only while <=64 bytes.
// No raw parent/name/ADS data is synthesized by this file-only transport.
Z7_COM7F_IMF(QtPanelFolder::GetParent(UInt32, UInt32 *, UInt32 *)) { return E_NOTIMPL; }

Z7_COM7F_IMF(QtPanelFolder::GetNumRawProps(UInt32 *count)) {
    *count = rawPropertyIds_.Size();
    return S_OK;
}

Z7_COM7F_IMF(QtPanelFolder::GetRawPropInfo(UInt32 index, BSTR *name, PROPID *id)) {
    if (index >= rawPropertyIds_.Size())
        return E_INVALIDARG;
    *name = nullptr;
    *id = rawPropertyIds_[index];
    return S_OK;
}

Z7_COM7F_IMF(QtPanelFolder::GetRawProp(UInt32 index, PROPID id, const void **data, UInt32 *size,
                                       UInt32 *type)) {
    *data = nullptr;
    *size = 0;
    *type = 0;
    if (index >= items_.Size())
        return E_INVALIDARG;
    if (rawPropertyIds_.FindInSorted(id) < 0)
        return E_NOTIMPL;
    const auto property = items_[index].RawProperties.constFind(id);
    if (property != items_[index].RawProperties.cend() && !property->isEmpty()) {
        *data = property->constData();
        *size = UInt32(property->size());
        *type = NPropDataType::kRaw;
    }
    return S_OK;
}

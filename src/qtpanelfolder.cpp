#include "qtpanelfolder.h"
#include "7zip/PropID.h"
#include "Windows/PropVariant.h"

namespace {
constexpr PROPID kSnapshotProperty = 0x10000;
const PROPID kArchiveProperties[] = {kpidPath, kpidSize, kpidPackSize};
} // namespace

QtPanelFolder::QtPanelFolder(const QString &prefix, const CObjectVector<CPanelItem> &items,
                             PropertyCatalog catalog)
    : prefix_(prefix), items_(items), catalog_(catalog) {
    if (catalog_ == PropertyCatalog::ArchiveTransport) {
        FOR_VECTOR(i, items_)
        if (!items_[i].MTimeText.isEmpty())
            hasMTimeText_ = true;
        return;
    }
    // Names are already localized/formatted by the Qt property presentation.
    // Describe their union once; missing values use the source VT_EMPTY contract.
    FOR_VECTOR(i, items_)
    FOR_VECTOR(j, items_[i].Properties) {
        const UString &name = items_[i].Properties[j].Name;
        unsigned k = 0;
        for (; k < propertyNames_.Size(); ++k)
            if (propertyNames_[k] == name)
                break;
        if (k == propertyNames_.Size())
            propertyNames_.Add(name);
    }
}

Z7_COM7F_IMF(QtPanelFolder::LoadItems()) { return S_OK; }

Z7_COM7F_IMF(QtPanelFolder::GetNumberOfItems(UInt32 *count)) {
    *count = items_.Size();
    return S_OK;
}

Z7_COM7F_IMF(QtPanelFolder::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    if (index >= items_.Size())
        return E_INVALIDARG;
    const CPanelItem &item = items_[index];
    NWindows::NCOM::CPropVariant prop;
    const int separator = item.RelPath.lastIndexOf('/');
    switch (id) {
    case kpidPath:
        prop = ToUString(item.RelPath);
        break;
    case kpidName:
        prop = ToUString(item.RelPath.mid(separator + 1));
        break;
    case kpidPrefix:
        prop = ToUString(item.RelPath.left(separator + 1));
        break;
    case kpidIsDir:
        prop = item.IsFolder;
        break;
    case kpidSize:
        if (item.SizeDefined)
            prop = item.Size;
        break;
    case kpidPackSize:
        if (item.PackSizeDefined)
            prop = item.PackSize;
        break;
    case kpidMTime:
        if (!hasMTimeText_)
            return E_NOTIMPL;
        if (!item.MTimeText.isEmpty())
            prop = ToUString(item.MTimeText);
        break;
    case kpidNumSubDirs:
        if (catalog_ != PropertyCatalog::ArchiveTransport)
            prop = item.NumSubDirs;
        break;
    case kpidNumSubFiles:
        if (catalog_ != PropertyCatalog::ArchiveTransport)
            prop = item.NumSubFiles;
        break;
    default:
        if (id < kSnapshotProperty || id - kSnapshotProperty >= propertyNames_.Size())
            return E_NOTIMPL;
        FOR_VECTOR(i, item.Properties)
        if (item.Properties[i].Name == propertyNames_[id - kSnapshotProperty]) {
            prop = item.Properties[i].Value;
            break;
        }
    }
    return prop.Detach(value);
}

Z7_COM7F_IMF(QtPanelFolder::BindToFolder(UInt32, IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}

Z7_COM7F_IMF(QtPanelFolder::BindToFolder(const wchar_t *, IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}

Z7_COM7F_IMF(QtPanelFolder::BindToParentFolder(IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}

Z7_COM7F_IMF(QtPanelFolder::GetNumberOfProperties(UInt32 *count)) {
    *count = catalog_ == PropertyCatalog::ArchiveTransport
                 ? Z7_ARRAY_SIZE(kArchiveProperties) + (hasMTimeText_ ? 1 : 0)
                 : propertyNames_.Size();
    return S_OK;
}

Z7_COM7F_IMF(QtPanelFolder::GetPropertyInfo(UInt32 index, BSTR *name, PROPID *id, VARTYPE *type)) {
    if (catalog_ == PropertyCatalog::ArchiveTransport) {
        if (hasMTimeText_ && index == Z7_ARRAY_SIZE(kArchiveProperties)) {
            *name = NULL;
            *id = kpidMTime;
            // Truthfully describe unparsed transport text, not a native FILETIME.
            *type = VT_BSTR;
            return S_OK;
        }
        if (index >= Z7_ARRAY_SIZE(kArchiveProperties))
            return E_INVALIDARG;
        *name = NULL;
        *id = kArchiveProperties[index];
        *type = k7z_PROPID_To_VARTYPE[(unsigned)*id];
        return S_OK;
    }
    if (index >= propertyNames_.Size())
        return E_INVALIDARG;
    *name = ::SysAllocStringLen(propertyNames_[index], propertyNames_[index].Len());
    *id = kSnapshotProperty + index;
    *type = VT_BSTR;
    return *name ? S_OK : E_OUTOFMEMORY;
}

Z7_COM7F_IMF(QtPanelFolder::GetFolderProperty(PROPID id, PROPVARIANT *value)) {
    if (id != kpidPath)
        return E_NOTIMPL;
    NWindows::NCOM::CPropVariant prop(ToUString(prefix_));
    return prop.Detach(value);
}

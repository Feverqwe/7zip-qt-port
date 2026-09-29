#include "qtarchiveproperties.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "qtarchivefileresource.h"

namespace {
class CFolderArchiveProperties final : public IInArchive, public IOutArchive, public CMyUnknownImp {
    Z7_COM_QI_BEGIN2(IInArchive)
    else if (_resourceSpec && _resourceSpec->HasUpdateService() && iid == IID_IOutArchive) {
        *outObject = static_cast<IOutArchive *>(this);
    }
    Z7_COM_QI_END
    Z7_COM_ADDREF_RELEASE
    CMyComPtr<IFolderFolder> _properties;
    CMyComPtr<IUnknown> _resource;
    QtArchiveFileResource *_resourceSpec;

  public:
    CFolderArchiveProperties(IFolderFolder *properties, IUnknown *resource)
        : _properties(properties), _resource(resource),
          _resourceSpec(dynamic_cast<QtArchiveFileResource *>(resource)) {}
    Z7_IFACE_COM7_IMP(IInArchive)
    Z7_IFACE_COM7_IMP(IOutArchive)
};

Z7_COM7F_IMF(CFolderArchiveProperties::GetFileTimeType(UInt32 *type)) {
    if (!type)
        return E_INVALIDARG;
    *type = NFileTimeType::kWindows;
    return S_OK;
}
Z7_COM7F_IMF(CFolderArchiveProperties::UpdateItems(ISequentialOutStream *output, UInt32 count,
                                                   IArchiveUpdateCallback *callback)) {
    return _resourceSpec ? _resourceSpec->Update(output, this, count, callback) : E_NOTIMPL;
}

Z7_COM7F_IMF(CFolderArchiveProperties::Open(IInStream *, const UInt64 *,
                                            IArchiveOpenCallback *callback)) {
    if (!_resourceSpec)
        return E_NOTIMPL;
    CMyComPtr<IFolderFolder> metadata;
    RINOK(_resourceSpec->ReOpen(&metadata, callback))
    _properties = metadata;
    return S_OK;
}
Z7_COM7F_IMF(CFolderArchiveProperties::Close()) { return S_OK; }
Z7_COM7F_IMF(CFolderArchiveProperties::GetNumberOfItems(UInt32 *count)) {
    return _properties->GetNumberOfItems(count);
}
Z7_COM7F_IMF(CFolderArchiveProperties::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    return _properties->GetProperty(index, id, value);
}
Z7_COM7F_IMF(CFolderArchiveProperties::Extract(const UInt32 *indices, UInt32 count, Int32 testMode,
                                               IArchiveExtractCallback *callback)) {
    return _resourceSpec ? _resourceSpec->Extract(this, indices, count, testMode, callback)
                         : E_NOTIMPL;
}
Z7_COM7F_IMF(CFolderArchiveProperties::GetArchiveProperty(PROPID id, PROPVARIANT *value)) {
    if (_resourceSpec)
        return _resourceSpec->GetArchiveProperty(id, value);
    NWindows::NCOM::CPropVariant prop;
    return prop.Detach(value);
}
Z7_COM7F_IMF(CFolderArchiveProperties::GetNumberOfProperties(UInt32 *count)) {
    return _properties->GetNumberOfProperties(count);
}
Z7_COM7F_IMF(CFolderArchiveProperties::GetPropertyInfo(UInt32 index, BSTR *name, PROPID *id,
                                                       VARTYPE *type)) {
    return _properties->GetPropertyInfo(index, name, id, type);
}
Z7_COM7F_IMF(CFolderArchiveProperties::GetNumberOfArchiveProperties(UInt32 *count)) {
    *count = 0;
    return S_OK;
}
Z7_COM7F_IMF(CFolderArchiveProperties::GetArchivePropertyInfo(UInt32, BSTR *, PROPID *,
                                                              VARTYPE *)) {
    return E_INVALIDARG;
}
} // namespace

HRESULT CreateArchivePropertyTransport(IFolderFolder *properties, IInArchive **archive,
                                       IUnknown *engineResource) {
    if (!properties || !archive)
        return E_INVALIDARG;
    *archive = nullptr;
    if (!engineResource) {
        const HRESULT result = properties->QueryInterface(IID_IInArchive, (void **)archive);
        if (result != E_NOINTERFACE)
            return result;
    }
    CMyComPtr<IInArchive> adapter = new CFolderArchiveProperties(properties, engineResource);
    *archive = adapter.Detach();
    return S_OK;
}

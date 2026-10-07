#include "qtarchiveproperties.h"
#include "CPP/7zip/UI/Common/OpenArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "qtarchivefileresource.h"
#include "qtarchiveitemstream.h"

bool ArchiveUpdateEnabled(IInArchive *archive) {
    if (!archive)
        return false;
    CMyComPtr<IOutArchive> update;
    archive->QueryInterface(IID_IOutArchive, reinterpret_cast<void **>(&update));
    return bool(update);
}

namespace {
class CFolderArchiveProperties final : public IInArchive,
                                       public IInArchiveGetStream,
                                       public IOutArchive,
                                       public ISetProperties,
                                       public IArchiveGetRawProps,
                                       public CMyUnknownImp {
    Z7_COM_QI_BEGIN2(IInArchive)
    else if (_resourceSpec && _resourceSpec->HasNativeItemStreamService() &&
             iid == IID_IInArchiveGetStream) {
        *outObject = static_cast<IInArchiveGetStream *>(this);
    }
    else if (_rawProps && iid == IID_IArchiveGetRawProps) {
        *outObject = static_cast<IArchiveGetRawProps *>(this);
    }
    else if (_resourceSpec && _resourceSpec->HasUpdateService() && iid == IID_IOutArchive) {
        *outObject = static_cast<IOutArchive *>(this);
    }
    else if (_resourceSpec && _resourceSpec->HasNativePropertyService() &&
             iid == IID_ISetProperties) {
        *outObject = static_cast<ISetProperties *>(this);
    }
    Z7_COM_QI_END
    Z7_COM_ADDREF_RELEASE
    // Native interfaces must release before their module/catalog lease.
    CMyComPtr<IUnknown> _resource;
    CMyComPtr<IFolderFolder> _properties;
    CMyComPtr<IArchiveGetRawProps> _rawProps;
    QtArchiveFileResource *_resourceSpec;
    void BindRawProperties() {
        _rawProps.Release();
        _properties.QueryInterface(IID_IArchiveGetRawProps, &_rawProps);
        if (_resourceSpec) {
            _resourceSpec->BindNativeItemOrder(_properties);
            if (auto *native = _resourceSpec->NativeItemProperties()) {
                CMyComPtr<IArchiveGetRawProps> raw;
                native->QueryInterface(IID_IArchiveGetRawProps, (void **)&raw);
                if (raw)
                    _rawProps = raw;
            }
        }
    }

  public:
    CFolderArchiveProperties(IFolderFolder *properties, IUnknown *resource)
        : _resource(resource), _properties(properties),
          _resourceSpec(dynamic_cast<QtArchiveFileResource *>(resource)) {
        BindRawProperties();
    }
    Z7_IFACE_COM7_IMP(IInArchive)
    Z7_IFACE_COM7_IMP(IInArchiveGetStream)
    Z7_IFACE_COM7_IMP(IOutArchive)
    Z7_IFACE_COM7_IMP(ISetProperties)
    Z7_IFACE_COM7_IMP(IArchiveGetRawProps)
};

Z7_COM7F_IMF(CFolderArchiveProperties::GetStream(UInt32 index, ISequentialInStream **stream)) {
    return CreateArchiveItemStream(_resource, index, stream);
}

Z7_COM7F_IMF(CFolderArchiveProperties::SetProperties(const wchar_t *const *names,
                                                     const PROPVARIANT *values, UInt32 count)) {
    return _resourceSpec ? _resourceSpec->SetNativeProperties(names, values, count) : E_NOTIMPL;
}

Z7_COM7F_IMF(CFolderArchiveProperties::GetParent(UInt32 index, UInt32 *parent, UInt32 *type)) {
    return _rawProps ? _rawProps->GetParent(index, parent, type) : E_NOTIMPL;
}
Z7_COM7F_IMF(CFolderArchiveProperties::GetNumRawProps(UInt32 *count)) {
    return _rawProps ? _rawProps->GetNumRawProps(count) : E_NOTIMPL;
}
Z7_COM7F_IMF(CFolderArchiveProperties::GetRawPropInfo(UInt32 index, BSTR *name, PROPID *id)) {
    return _rawProps ? _rawProps->GetRawPropInfo(index, name, id) : E_NOTIMPL;
}
Z7_COM7F_IMF(CFolderArchiveProperties::GetRawProp(UInt32 index, PROPID id, const void **data,
                                                  UInt32 *size, UInt32 *type)) {
    return _rawProps ? _rawProps->GetRawProp(index, id, data, size, type) : E_NOTIMPL;
}

Z7_COM7F_IMF(CFolderArchiveProperties::GetFileTimeType(UInt32 *type)) {
    if (!type)
        return E_INVALIDARG;
    if (_resourceSpec)
        if (auto *native = _resourceSpec->NativeItemProperties()) {
            CMyComPtr<IOutArchive> writer;
            if (native->QueryInterface(IID_IOutArchive, (void **)&writer) == S_OK)
                return writer->GetFileTimeType(type);
        }
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
    BindRawProperties();
    return S_OK;
}
Z7_COM7F_IMF(CFolderArchiveProperties::Close()) {
    return _resourceSpec ? _resourceSpec->CloseNativeOpen() : S_OK;
}
Z7_COM7F_IMF(CFolderArchiveProperties::GetNumberOfItems(UInt32 *count)) {
    return _properties->GetNumberOfItems(count);
}
Z7_COM7F_IMF(CFolderArchiveProperties::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    if (_resourceSpec)
        if (auto *native = _resourceSpec->NativeItemProperties()) {
            // IInArchive permits an omitted IsDir property (gzip is a file).
            // The folder/process-plan boundary needs its defined bool. Reuse
            // the original archive property reader's default/type rules.
            if (id == kpidIsDir) {
                bool isDir;
                RINOK(Archive_IsItem_Dir(native, index, isDir))
                NWindows::NCOM::CPropVariant prop(isDir);
                return prop.Detach(value);
            }
            return native->GetProperty(index, id, value);
        }
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
    if (_resourceSpec)
        return _resourceSpec->GetNumberOfArchiveProperties(count);
    *count = 0;
    return S_OK;
}
Z7_COM7F_IMF(CFolderArchiveProperties::GetArchivePropertyInfo(UInt32 index, BSTR *name, PROPID *id,
                                                              VARTYPE *type)) {
    return _resourceSpec ? _resourceSpec->GetArchivePropertyInfo(index, name, id, type)
                         : E_INVALIDARG;
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

HRESULT CreateArchiveLinkTransport(CArchiveLink &link, IFolderFolder *properties,
                                   IUnknown *engineResource, const CCodecs *codecs) {
    if (!properties)
        return E_INVALIDARG;
    if (!link.Arcs.IsEmpty())
        return E_FAIL;
    CArc &arc = link.Arcs.AddNew();
    const HRESULT result = CreateArchivePropertyTransport(properties, &arc.Archive, engineResource);
    if (result != S_OK) {
        link.Release();
        return result;
    }
    arc.DefaultName = L"[Content]";
    arc.FormatIndex = -1;
    arc.ArcStreamOffset = 0;
    arc.Offset = 0;
    arc.FileSize = 0;
    arc.AvailPhySize = 0;
    arc.PhySize = 0;
    arc.PhySize_Defined = false;
    arc.Archive.QueryInterface(IID_IArchiveGetRawProps, &arc.GetRawProps);
    if (auto *resource = dynamic_cast<QtArchiveFileResource *>(engineResource)) {
        RINOK(resource->ConfigureArc(arc, codecs))
        resource->CopyNativeVolumeMetadata(link);
    }
    link.IsOpen = true;
    return S_OK;
}

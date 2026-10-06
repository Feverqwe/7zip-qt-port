#pragma once
#include "panel_load_folder.h"

class SchemaFolder final : public IFolderFolder, public IArchiveGetRawProps, public CMyUnknownImp {
    Z7_COM_QI_BEGIN2(IFolderFolder)
    Z7_COM_QI_ENTRY(IArchiveGetRawProps)
    Z7_COM_QI_END
    Z7_COM_ADDREF_RELEASE
  public:
    struct Property {
        PROPID id;
        VARTYPE type;
        HRESULT result = S_OK;
    };
    QVector<Property> properties{{kpidName, VT_BSTR}, {kpidSize, VT_UI8}, {kpidCRC, VT_UI4}};
    HRESULT propertyCountResult = S_OK;
    HRESULT rawCountResult = S_OK;
    HRESULT rawInfoResult = S_OK;
    bool writePropertyCount = true;
    bool writeRawCount = true;
    bool rawEnabled = true;
    unsigned propertyInfoQueries = 0;
    unsigned rawInfoQueries = 0;
    LoadFolder *items = new LoadFolder;
    CMyComPtr<IFolderFolder> owner = items;
    PROPID rawId = kpidCRC;
    unsigned rawQueries = 0;
    HRESULT rawResult = S_OK;
    HRESULT crcResult = S_OK;
    unsigned countQueries = 0;
    SchemaFolder() { items->directory = false; }
    Z7_IFACE_COM7_IMP(IFolderFolder)
    Z7_IFACE_COM7_IMP(IArchiveGetRawProps)
};

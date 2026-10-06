#pragma once
#include "panel_load_folder.h"

class SchemaFolder final : public IFolderFolder, public IArchiveGetRawProps, public CMyUnknownImp {
    Z7_COM_QI_BEGIN2(IFolderFolder)
    Z7_COM_QI_ENTRY(IArchiveGetRawProps)
    Z7_COM_QI_END
    Z7_COM_ADDREF_RELEASE
  public:
    LoadFolder *items = new LoadFolder;
    CMyComPtr<IFolderFolder> owner = items;
    PROPID rawId = kpidCRC;
    unsigned rawQueries = 0;
    HRESULT rawResult = S_OK;
    SchemaFolder() { items->directory = false; }
    Z7_IFACE_COM7_IMP(IFolderFolder)
    Z7_IFACE_COM7_IMP(IArchiveGetRawProps)
};

#pragma once

#include "panel_load_folder.h"

class DeletedFolder final : public IFolderFolder,
                            public IGetFolderArcProps,
                            public IFolderArcProps,
                            public CMyUnknownImp {
    Z7_COM_QI_BEGIN2(IFolderFolder)
    Z7_COM_QI_ENTRY(IGetFolderArcProps)
    Z7_COM_QI_ENTRY(IFolderArcProps)
    Z7_COM_QI_END
    Z7_COM_ADDREF_RELEASE
  public:
    LoadFolder *items = new LoadFolder;
    CMyComPtr<IFolderFolder> owner = items;
    bool archiveHasDeleted = true;
    bool firstDeleted = true;
    VARTYPE deletedType = VT_BOOL;
    HRESULT deletedResult = S_OK;
    unsigned deletedQueries = 0;
    DeletedFolder() { items->directory = false; }
    Z7_IFACE_COM7_IMP(IFolderFolder)
    Z7_IFACE_COM7_IMP(IGetFolderArcProps)
    Z7_IFACE_COM7_IMP(IFolderArcProps)
};

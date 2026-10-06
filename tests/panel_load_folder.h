#pragma once

#include "CPP/7zip/UI/FileManager/Panel.h"
#include <QStringList>

class LoadFolder final : public IFolderFolder,
                         public IFolderSetFlatMode,
                         public IFolderWasChanged,
                         public CMyUnknownImp {
    Z7_COM_QI_BEGIN2(IFolderFolder)
    Z7_COM_QI_ENTRY(IFolderSetFlatMode)
    Z7_COM_QI_ENTRY(IFolderWasChanged)
    Z7_COM_QI_END
    Z7_COM_ADDREF_RELEASE
  public:
    QStringList calls;
    HRESULT loadResult = S_OK;
    HRESULT directoryResult = S_OK;
    HRESULT countResult = S_OK;
    HRESULT loadedCountResult = S_OK;
    VARTYPE directoryType = VT_BOOL;
    bool directory = true;
    bool reverseNames = false;
    bool changed = false;
    Z7_IFACE_COM7_IMP(IFolderFolder)
    Z7_IFACE_COM7_IMP(IFolderSetFlatMode)
    Z7_IFACE_COM7_IMP(IFolderWasChanged)
};

#pragma once

#include "CPP/7zip/UI/FileManager/Panel.h"
#include <QStringList>

class LoadFolder final : public IFolderFolder,
                         public IFolderSetFlatMode,
                         public IFolderWasChanged,
                         public IFolderGetSystemIconIndex,
                         public IFolderGetItemName,
                         public CMyUnknownImp {
    Z7_COM_QI_BEGIN2(IFolderFolder)
    Z7_COM_QI_ENTRY(IFolderSetFlatMode)
    Z7_COM_QI_ENTRY(IFolderWasChanged)
    Z7_COM_QI_ENTRY(IFolderGetSystemIconIndex)
    else if (iid == IID_IFolderGetItemName && fastNames) {
        *outObject = static_cast<IFolderGetItemName *>(this);
    }
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
    UString folderType = L"RootFolder";
    HRESULT iconResult = E_NOTIMPL;
    Int32 iconIndex = -1;
    HRESULT attributeResult = S_OK;
    UInt32 attributes = 0;
    unsigned iconCalls = 0;
    unsigned attributeCalls = 0;
    bool fastNames = false;
    UString firstName = L"first δ";
    UString secondName = L"second 文件.txt";
    UString prefix;
    HRESULT nameResult = S_OK;
    HRESULT prefixResult = S_OK;
    Z7_IFACE_COM7_IMP(IFolderFolder)
    Z7_IFACE_COM7_IMP(IFolderSetFlatMode)
    Z7_IFACE_COM7_IMP(IFolderWasChanged)
    Z7_IFACE_COM7_IMP(IFolderGetSystemIconIndex)
    Z7_IFACE_COM7_IMP(IFolderGetItemName)
};

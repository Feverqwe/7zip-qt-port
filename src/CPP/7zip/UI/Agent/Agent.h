// Adapted from 7-Zip UI/Agent/Agent.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "7zip/UI/FileManager/IFolder.h"
#include "AgentProxy.h"
#include "Common/MyCom.h"
#include "Common/MyString.h"
#include "Common/MyVector.h"

struct CProxyItem {
    unsigned DirIndex;
    unsigned Index;
};

class CAgentFolder final : public IFolderFolder, public IFolderSetFlatMode, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IFolderFolder, IFolderSetFlatMode)
  public:
    CAgentFolder() = default;
    CAgentFolder(const CProxyArc &proxy, unsigned dirIndex, bool flatMode);
    Z7_IFACE_COM7_IMP(IFolderFolder)
    Z7_IFACE_COM7_IMP(IFolderSetFlatMode)
    void Init(const CProxyArc *proxy, unsigned proxyDirIndex, IFolderFolder *archiveProps,
              const UString &archiveType);
    HRESULT BindToFolder_Internal(unsigned proxyDirIndex, IFolderFolder **resultFolder);
    unsigned GetNumberOfItems() const;
    CProxyItem GetItem(unsigned index) const;

  private:
    void LoadFolder(unsigned dirIndex);
    void GetPrefix(UInt32 index, UString &prefix) const;
    bool IsBound() const;
    const CProxyArc *_proxy = nullptr;
    unsigned _proxyDirIndex = 0;
    bool _flatMode = false;
    CRecordVector<CProxyItem> _items;
    // Typed 7zz metadata boundary replaces the in-process archive property call.
    CMyComPtr<IFolderFolder> _archiveProps;
    UString _archiveType;
};

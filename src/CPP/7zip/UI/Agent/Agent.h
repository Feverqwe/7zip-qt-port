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

class CAgent;

class CAgentFolder final : public IFolderFolder, public IFolderSetFlatMode, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IFolderFolder, IFolderSetFlatMode)
  public:
    CAgentFolder() = default;
    Z7_IFACE_COM7_IMP(IFolderFolder)
    Z7_IFACE_COM7_IMP(IFolderSetFlatMode)
    void Init(const CProxyArc *proxy, unsigned proxyDirIndex, CAgent *agent);
    HRESULT BindToFolder_Internal(unsigned proxyDirIndex, IFolderFolder **resultFolder);
    unsigned GetNumberOfItems() const;
    CProxyItem GetItem(unsigned index) const;
    // Read-only Qt presentation access to the source owner's authoritative tree.
    const CProxyArc &GetProxy() const { return *_proxy; }

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
    // IUnknown replaces the in-process IInFolderArchive owner at the 7zz boundary.
    CMyComPtr<IUnknown> _agent;
    CAgent *_agentSpec = nullptr;
};

// Partial source owner: proxy lifetime/root binding only. Engine operations remain
// behind ArchiveBackend; IInFolderArchive is not advertised by this adapter.
class CAgent final : public IUnknown, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_0
  public:
    CAgent();
    ~CAgent();
    HRESULT Init(const QList<ArchiveEntry> &entries, IFolderFolder *archiveProps,
                 const UString &archiveType);
    HRESULT BindToRootFolder(IFolderFolder **resultFolder);
    CProxyArc *_proxy;
    UString ArchiveType;
    bool ThereIsPathProp = false;
    IFolderFolder *GetArchive() const { return _archiveProps; }

  private:
    HRESULT ReadItems();
    CMyComPtr<IFolderFolder> _archiveProps;
    QList<ArchiveEntry> _entries;
};

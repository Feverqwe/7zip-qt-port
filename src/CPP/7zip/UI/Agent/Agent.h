// Adapted from 7-Zip UI/Agent/Agent.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "7zip/UI/Common/UpdateAction.h"
#include "7zip/UI/FileManager/IFolder.h"
#include "AgentProxy.h"
#include "Common/MyCom.h"
#include "Common/MyString.h"
#include "Common/MyVector.h"
#include "IFolderArchive.h"
#include "archivebackend.h"
#include "qtprogresstransport.h"

struct CProxyItem {
    unsigned DirIndex;
    unsigned Index;
};

class CAgent;
class CAgentFolder;
enum AGENT_OP {
    AGENT_OP_Uni,
    AGENT_OP_Delete,
    AGENT_OP_CreateFolder,
    AGENT_OP_Rename,
    AGENT_OP_CopyFromFile,
    AGENT_OP_Comment
};
Z7_PURE_INTERFACES_BEGIN
#define Z7_IFACEM_IArchiveFolderInternal(x) x(GetAgentFolder(CAgentFolder **agentFolder))
Z7_IFACE_CONSTR_FOLDERARC(IArchiveFolderInternal, 0xC)
Z7_PURE_INTERFACES_END

class CAgentFolder final : public IFolderFolder,
                           public IArchiveGetRawProps,
                           public IFolderSetFlatMode,
                           public IArchiveFolderInternal,
                           public IFolderOperations,
                           public IArchiveFolder,
                           public IFolderSetZoneIdMode,
                           public IFolderSetZoneIdFile,
                           public IQtProgressServices,
                           public CMyUnknownImp {
    Z7_COM_QI_BEGIN2(IFolderFolder)
    Z7_COM_QI_ENTRY(IArchiveGetRawProps)
    Z7_COM_QI_ENTRY(IFolderSetFlatMode)
    Z7_COM_QI_ENTRY(IArchiveFolderInternal)
    else if (iid == IID_IFolderOperations && (CanCopyFromFile() || CanCopyTo())) {
        *outObject = static_cast<IFolderOperations *>(this);
    }
    else if (iid == IID_IArchiveFolder && CanCopyTo()) {
        *outObject = static_cast<IArchiveFolder *>(this);
    }
    Z7_COM_QI_ENTRY(IFolderSetZoneIdMode)
    Z7_COM_QI_ENTRY(IFolderSetZoneIdFile)
    Z7_COM_QI_END
    Z7_COM_ADDREF_RELEASE
  public:
    CAgentFolder() = default;
    Z7_IFACE_COM7_IMP(IFolderFolder)
    Z7_IFACE_COM7_IMP(IArchiveGetRawProps)
    Z7_IFACE_COM7_IMP(IFolderSetFlatMode)
    Z7_IFACE_COM7_IMP(IArchiveFolderInternal)
    Z7_IFACE_COM7_IMP(IFolderOperations)
    Z7_IFACE_COM7_IMP(IArchiveFolder)
    Z7_IFACE_COM7_IMP(IFolderSetZoneIdMode)
    Z7_IFACE_COM7_IMP(IFolderSetZoneIdFile)
    void Init(const CProxyArc *proxy, unsigned proxyDirIndex, CAgent *agent);
    HRESULT BindToFolder_Internal(unsigned proxyDirIndex, IFolderFolder **resultFolder);
    int GetRealIndex(unsigned index) const;
    UString GetName(UInt32 index) const;
    UString GetFullPrefix(UInt32 index) const;
    void GetRealIndices(const UInt32 *indices, UInt32 numItems, bool includeAltStreams,
                        bool includeFolderSubItemsInFlatMode, CUIntVector &realIndices) const;
    unsigned GetNumberOfItems() const;
    CProxyItem GetItem(unsigned index) const;
    // Read-only Qt presentation access to the source owner's authoritative tree.
    const CProxyArc &GetProxy() const { return *_proxy; }
    // Read-only 7zz transport boundary for original archive-index Path queries.
    IInArchive *GetArchiveProperties() const;
    // Borrowed engine service for Qt presentation; the source agent retains it.
    IUnknown *GetEngineResource() const;
    // File-only refresh bridge; native ReOpen and folder binding retain ownership.
    HRESULT ReOpen(IArchiveOpenCallback *callback, IFolderFolder **folder);
    std::unique_ptr<IQtProgressTransport> CreateProgressTransport(CProgressDialog &dialog) override;
    void GetPathParts(UStringVector &pathParts, bool &isAltStreamFolder);
    HRESULT CommonUpdateOperation(AGENT_OP operation, bool moveMode, const wchar_t *newItemName,
                                  const NUpdateArchive::CActionSet *actionSet,
                                  const UInt32 *indices, UInt32 numItems, IProgress *progress);

  private:
    friend class CAgent;
    void LoadFolder(unsigned dirIndex);
    void GetPrefix(UInt32 index, UString &prefix) const;
    bool IsBound() const;
    bool CanCopyFromFile() const;
    bool CanCopyTo() const;
    const CProxyArc *_proxy = nullptr;
    const CProxyArc2 *_proxy2 = nullptr;
    bool _isAltStreamFolder = false;
    unsigned _proxyDirIndex = 0;
    bool _flatMode = false;
    bool _loadAltStreams = false;
    NExtract::NZoneIdMode::EEnum _zoneMode = NExtract::NZoneIdMode::kNone;
    CByteBuffer _zoneBuf;
    CRecordVector<CProxyItem> _items;
    // Typed 7zz metadata boundary replaces the in-process archive property call.
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
                 const UString &archiveType, IUnknown *engineResource = nullptr);
    HRESULT BindToRootFolder(IFolderFolder **resultFolder);
    CProxyArc *_proxy;
    CProxyArc2 *_proxy2;
    CArchiveLink _archiveLink;
    UString _updatePathPrefix;
    bool _updatePathPrefix_is_AltFolder;
    bool _isDeviceFile;
    bool _isHashHandler = false;
    CAgentFolder *_agentFolder;
    UString ArchiveType;
    UString _archiveFilePath;
    // INVALID_FILE_ATTRIBUTES has no non-Windows header definition.
    DWORD _attrib = (DWORD)-1;
    FString _hashBaseFolderPrefix;
    bool ThereIsPathProp = false;
    const CArc &GetArc() const { return _archiveLink.Arcs.Back(); }
    IInArchive *GetArchive() const {
        if (_archiveLink.Arcs.IsEmpty())
            return NULL;
        return GetArc().Archive;
    }
    bool CanUpdate() const;
    bool Is_Attrib_ReadOnly() const {
        return _attrib != (DWORD)-1 && (_attrib & FILE_ATTRIBUTE_READONLY);
    }
    bool IsThere_ReadOnlyArc() const;
    HRESULT ReOpen(IArchiveOpenCallback *callback);
    HRESULT Close();
    void KeepModeForNextOpen() { _archiveLink.KeepModeForNextOpen(); }
    HRESULT SetFolder(IFolderFolder *folder);
    HRESULT CommonUpdate(ISequentialOutStream *outArchiveStream, unsigned numUpdateItems,
                         IArchiveUpdateCallback *updateCallback);
    HRESULT UpdateOneFile(ISequentialOutStream *outArchiveStream, const UInt32 *indices,
                          UInt32 numItems, const wchar_t *diskFilePath,
                          IFolderArchiveUpdateCallback *updateCallback100);
    // External engine stream/file lifetime replaces the in-process archive's
    // stream ownership. All folders retain this source CAgent owner.
    void AttachEngineResource(IUnknown *resource) { _engineResource = resource; }
    IUnknown *GetEngineResource() const { return _engineResource; }

  private:
    HRESULT ReadItems();
    CMyComPtr<IUnknown> _engineResource;
};

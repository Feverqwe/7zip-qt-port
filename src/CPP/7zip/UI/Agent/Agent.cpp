// Adapted from 7-Zip CPP/7zip/UI/Agent/Agent.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// The CProxyArc folder walk/binding/property control flow is retained. The
// _proxy2 and engine operation branches remain outside this partial port.
#include "Agent.h"
#include "../../../../../third_party/7zip/C/Sort.h"
#include "../../../Windows/FileDir.h"
#include "../Common/ArchiveExtractCallback.h"
#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "Common/ComTry.h"
#include "Common/Defs.h"
#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/PropVariant.h"
#include "Windows/System.h"
#include "qtarchivefileresource.h"
#include "qtarchiveproperties.h"

static const bool k_keepEmptyDirPrefixes = false; // upstream 22.00 and later

Z7_COM7F_IMF(CAgentFolder::GetAgentFolder(CAgentFolder **agentFolder)) {
    *agentFolder = this;
    return S_OK;
}

void CAgentFolder::LoadFolder(unsigned dirIndex) {
    CProxyItem item;
    item.DirIndex = dirIndex;
    const CProxyDir &dir = _proxy->Dirs[dirIndex];
    unsigned i;
    for (i = 0; i < dir.SubDirs.Size(); i++) {
        item.Index = i;
        _items.Add(item);
        LoadFolder(dir.SubDirs[i]);
    }
    const unsigned start = dir.SubDirs.Size();
    for (i = 0; i < dir.SubFiles.Size(); i++) {
        item.Index = start + i;
        _items.Add(item);
    }
}

Z7_COM7F_IMF(CAgentFolder::LoadItems()) {
    if (!IsBound())
        return E_FAIL;
    _items.Clear();
    if (_flatMode)
        LoadFolder(_proxyDirIndex);
    return S_OK;
}

unsigned CAgentFolder::GetNumberOfItems() const {
    UInt32 numItems = 0;
    // Legacy Qt enumeration convenience; one source interface body owns count.
    const_cast<CAgentFolder *>(this)->GetNumberOfItems(&numItems);
    return numItems;
}

CProxyItem CAgentFolder::GetItem(unsigned index) const {
    if (_flatMode)
        return _items[index];
    return {_proxyDirIndex, index};
}

bool CAgentFolder::IsBound() const { return _proxy && _proxyDirIndex < _proxy->Dirs.Size(); }

void CAgentFolder::Init(const CProxyArc *proxy, unsigned proxyDirIndex, CAgent *agent) {
    _proxy = proxy;
    _proxyDirIndex = proxyDirIndex;
    _agent = (IUnknown *)agent;
    _agentSpec = agent;
    _archiveType = agent->ArchiveType;
}

IInArchive *CAgentFolder::GetArchiveProperties() const {
    return _agentSpec ? _agentSpec->GetArchive() : nullptr;
}

Z7_COM7F_IMF(CAgentFolder::GetNumberOfItems(UInt32 *numItems)) {
    if (!IsBound()) {
        *numItems = 0;
        return E_FAIL;
    }
    if (_flatMode)
        *numItems = _items.Size();
    else {
        const CProxyDir *dir = &_proxy->Dirs[_proxyDirIndex];
        *numItems = (UInt32)(dir->SubDirs.Size() + dir->SubFiles.Size());
    }
    return S_OK;
}

#define SET_realIndex_AND_dir                                                                      \
    unsigned realIndex;                                                                            \
    const CProxyDir *dir;                                                                          \
    if (_flatMode) {                                                                               \
        const CProxyItem &item = _items[index];                                                    \
        dir = &_proxy->Dirs[item.DirIndex];                                                        \
        realIndex = item.Index;                                                                    \
    } else {                                                                                       \
        dir = &_proxy->Dirs[_proxyDirIndex];                                                       \
        realIndex = index;                                                                         \
    }

UString CAgentFolder::GetName(UInt32 index) const {
    SET_realIndex_AND_dir;
    if (realIndex < dir->SubDirs.Size())
        return _proxy->Dirs[dir->SubDirs[realIndex]].Name;
    return _proxy->Files[dir->SubFiles[realIndex - dir->SubDirs.Size()]].Name;
}

void CAgentFolder::GetPrefix(UInt32 index, UString &prefix) const {
    prefix.Empty();
    if (!_flatMode)
        return;
    const unsigned itemDirIndex = _items[index].DirIndex;
    if (itemDirIndex == _proxyDirIndex)
        return;
    _proxy->GetDirPath_as_Prefix_from_Base(itemDirIndex, prefix, _proxyDirIndex);
}

UString CAgentFolder::GetFullPrefix(const UInt32 index) const {
    unsigned foldIndex = _proxyDirIndex;
    if (_flatMode)
        foldIndex = _items[index].DirIndex;
    return _proxy->GetDirPath_as_Prefix(foldIndex);
}

Z7_COM7F_IMF(CAgentFolder::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    if (!IsBound() || index >= GetNumberOfItems())
        return E_INVALIDARG;
    NWindows::NCOM::CPropVariant prop;
    if (propID == kpidPrefix) {
        if (_flatMode) {
            UString prefix;
            GetPrefix(index, prefix);
            prop = prefix;
        }
    } else {
        const CProxyItem proxyItem = GetItem(index); // SET_realIndex_AND_dir control adapter.
        const unsigned realIndex = proxyItem.Index;
        const CProxyDir *dir = &_proxy->Dirs[proxyItem.DirIndex];
        if (realIndex < dir->SubDirs.Size()) {
            const CProxyDir &item = _proxy->Dirs[dir->SubDirs[realIndex]];
            if (!_flatMode && propID == kpidSize)
                prop = UInt64(item.Size);
            else if (!_flatMode && propID == kpidPackSize)
                prop = UInt64(item.PackSize);
            else
                switch (propID) {
                case kpidIsDir:
                    prop = true;
                    break;
                case kpidNumSubDirs:
                    prop = item.NumSubDirs;
                    break;
                case kpidNumSubFiles:
                    prop = item.NumSubFiles;
                    break;
                case kpidName:
                    prop = item.Name;
                    break;
                default:
                    if (item.IsLeaf()) {
                        if (!_agentSpec->GetArchive())
                            return E_NOTIMPL;
                        return _agentSpec->GetArchive()->GetProperty((unsigned)item.ArcIndex,
                                                                     propID, value);
                    }
                }
        } else {
            const unsigned arcIndex = dir->SubFiles[realIndex - dir->SubDirs.Size()];
            switch (propID) {
            case kpidIsDir:
                prop = false;
                break;
            case kpidName:
                prop = _proxy->Files[arcIndex].Name;
                break;
            default:
                if (!_agentSpec->GetArchive())
                    return E_NOTIMPL;
                return _agentSpec->GetArchive()->GetProperty(arcIndex, propID, value);
            }
        }
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

HRESULT CAgentFolder::BindToFolder_Internal(unsigned proxyDirIndex, IFolderFolder **resultFolder) {
    CAgentFolder *folderSpec = new CAgentFolder;
    CMyComPtr<IFolderFolder> agentFolder = folderSpec;
    folderSpec->Init(_proxy, proxyDirIndex, _agentSpec);
    *resultFolder = agentFolder.Detach();
    return S_OK;
}

Z7_COM7F_IMF(CAgentFolder::BindToFolder(UInt32 index, IFolderFolder **resultFolder)) {
    COM_TRY_BEGIN
    *resultFolder = nullptr;
    if (!IsBound())
        return E_FAIL;
    if (_flatMode && index >= _items.Size())
        return E_INVALIDARG;
    const CProxyItem item = GetItem(index);
    const unsigned realIndex = item.Index;
    const CProxyDir *dir = &_proxy->Dirs[item.DirIndex];
    if (realIndex >= (UInt32)dir->SubDirs.Size())
        return E_INVALIDARG;
    return BindToFolder_Internal(dir->SubDirs[realIndex], resultFolder);
    COM_TRY_END
}

Z7_COM7F_IMF(CAgentFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)) {
    COM_TRY_BEGIN
    *resultFolder = nullptr;
    if (!IsBound())
        return E_FAIL;
    const int index = _proxy->FindSubDir(_proxyDirIndex, name);
    if (index == -1)
        return E_INVALIDARG;
    return BindToFolder_Internal((unsigned)index, resultFolder);
    COM_TRY_END
}

Z7_COM7F_IMF(CAgentFolder::BindToParentFolder(IFolderFolder **resultFolder)) {
    COM_TRY_BEGIN
    *resultFolder = nullptr;
    if (!IsBound())
        return E_FAIL;
    unsigned proxyDirIndex;
    const int parent = _proxy->Dirs[_proxyDirIndex].ParentDir;
    if (parent == -1)
        return S_OK;
    proxyDirIndex = (unsigned)parent;
    CAgentFolder *folderSpec = new CAgentFolder;
    CMyComPtr<IFolderFolder> agentFolder = folderSpec;
    folderSpec->Init(_proxy, proxyDirIndex, _agentSpec);
    *resultFolder = agentFolder.Detach();
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CAgentFolder::GetFolderProperty(PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    if (!IsBound())
        return E_FAIL;
    NWindows::NCOM::CPropVariant prop;
    const CProxyDir &dir = _proxy->Dirs[_proxyDirIndex];
    switch (propID) {
    case kpidReadOnly:
        if ((_agentSpec->_proxy && _agentSpec->_proxy->Are_Changed_LongPaths) ||
            _agentSpec->Is_Attrib_ReadOnly())
            prop = true;
        else
            prop = _agentSpec->IsThere_ReadOnlyArc();
        break;
    case kpidIsHash:
        prop = _agentSpec->_isHashHandler;
        break;
    case kpidSize:
        prop = UInt64(dir.Size);
        break;
    case kpidPackSize:
        prop = UInt64(dir.PackSize);
        break;
    case kpidNumSubDirs:
        prop = dir.NumSubDirs;
        break;
    case kpidNumSubFiles:
        prop = dir.NumSubFiles;
        break;
    case kpidName:
        prop = dir.Name;
        break;
    case kpidPath:
        prop = _proxy->GetDirPath_as_Prefix(_proxyDirIndex);
        break;
    case kpidType:
        prop = UString("7-Zip.") + _archiveType;
        break;
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

static const PROPID kProps[] = {kpidNumSubDirs, kpidNumSubFiles, kpidPrefix};

Z7_COM7F_IMF(CAgentFolder::GetNumberOfProperties(UInt32 *numProps)) {
    COM_TRY_BEGIN
    if (!IsBound()) {
        *numProps = 0;
        return E_FAIL;
    }
    RINOK(_agentSpec->GetArchive()->GetNumberOfProperties(numProps))
    *numProps += Z7_ARRAY_SIZE(kProps);
    if (!_flatMode)
        (*numProps)--;
    if (!_agentSpec->ThereIsPathProp)
        (*numProps)++;
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CAgentFolder::GetPropertyInfo(UInt32 index, BSTR *name, PROPID *propID,
                                           VARTYPE *varType)) {
    COM_TRY_BEGIN
    if (!IsBound())
        return E_FAIL;
    UInt32 numProps;
    RINOK(_agentSpec->GetArchive()->GetNumberOfProperties(&numProps))
    // Source in-process callers supply valid indices. The public 7zz adapter
    // validates them and propagates a failed transport query before using count.
    if (!_agentSpec->ThereIsPathProp) {
        if (index == 0) {
            *propID = kpidName;
            *varType = VT_BSTR;
            *name = NULL;
            return S_OK;
        }
        index--;
    }
    if (index < numProps) {
        RINOK(_agentSpec->GetArchive()->GetPropertyInfo(index, name, propID, varType))
        if (*propID == kpidPath)
            *propID = kpidName;
    } else {
        index -= numProps;
        if (index >= Z7_ARRAY_SIZE(kProps) - (!_flatMode ? 1 : 0))
            return E_INVALIDARG;
        *propID = kProps[index];
        *varType = k7z_PROPID_To_VARTYPE[(unsigned)*propID];
        *name = NULL;
    }
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CAgentFolder::SetFlatMode(Int32 flatMode)) {
    _flatMode = IntToBool(flatMode);
    return S_OK;
}

Z7_COM7F_IMF(CAgentFolder::GetNumRawProps(UInt32 *numProps)) {
    CMyComPtr<IInArchive> archive = _agentSpec ? _agentSpec->GetArchive() : nullptr;
    CMyComPtr<IArchiveGetRawProps> rawProps;
    if (archive)
        archive.QueryInterface(IID_IArchiveGetRawProps, &rawProps);
    if (rawProps)
        return rawProps->GetNumRawProps(numProps);
    *numProps = 0;
    return S_OK;
}

Z7_COM7F_IMF(CAgentFolder::GetParent(UInt32 /* index */, UInt32 * /* parent */,
                                     UInt32 * /* parentType */)) {
    return E_FAIL;
}

Z7_COM7F_IMF(CAgentFolder::GetRawPropInfo(UInt32 index, BSTR *name, PROPID *propID)) {
    CMyComPtr<IInArchive> archive = _agentSpec ? _agentSpec->GetArchive() : nullptr;
    CMyComPtr<IArchiveGetRawProps> rawProps;
    if (archive)
        archive.QueryInterface(IID_IArchiveGetRawProps, &rawProps);
    return rawProps ? rawProps->GetRawPropInfo(index, name, propID) : E_FAIL;
}

Z7_COM7F_IMF(CAgentFolder::GetRawProp(UInt32 index, PROPID propID, const void **data,
                                      UInt32 *dataSize, UInt32 *propType)) {
    CMyComPtr<IInArchive> archive = _agentSpec ? _agentSpec->GetArchive() : nullptr;
    CMyComPtr<IArchiveGetRawProps> rawProps;
    if (archive)
        archive.QueryInterface(IID_IArchiveGetRawProps, &rawProps);
    if (rawProps) {
        if (!IsBound() || index >= GetNumberOfItems())
            return E_INVALIDARG;
        const int arcIndex = GetRealIndex(index);
        if (arcIndex >= 0)
            return rawProps->GetRawProp(unsigned(arcIndex), propID, data, dataSize, propType);
    }
    *data = nullptr;
    *dataSize = 0;
    *propType = 0;
    return S_OK;
}

int CAgentFolder::GetRealIndex(unsigned index) const {
    if (!_flatMode)
        return _proxy->GetRealIndex(_proxyDirIndex, index);
    {
        const CProxyItem &item = _items[index];
        {
            const CProxyDir *dir = &_proxy->Dirs[item.DirIndex];
            const unsigned realIndex = item.Index;
            if (realIndex < dir->SubDirs.Size()) {
                const CProxyDir &f = _proxy->Dirs[dir->SubDirs[realIndex]];
                if (!f.IsLeaf())
                    return -1;
                return f.ArcIndex;
            }
            return (int)dir->SubFiles[realIndex - dir->SubDirs.Size()];
        }
    }
}

void CAgentFolder::GetRealIndices(const UInt32 *indices, const UInt32 numItems,
                                  const bool includeAltStreams,
                                  const bool includeFolderSubItemsInFlatMode,
                                  CUIntVector &realIndices) const {
    // The omitted proxy2 branch owns alternate-stream expansion (P1-10).
    (void)includeAltStreams;
    if (!_flatMode) {
        _proxy->GetRealIndices_Unsorted(_proxyDirIndex, indices, numItems, realIndices);
    } else {
        realIndices.Clear();
        for (UInt32 i = 0; i < numItems; i++) {
            const CProxyItem &item = _items[indices[i]];
            UInt32 arcIndex;
            {
                const CProxyDir *dir = &_proxy->Dirs[item.DirIndex];
                const unsigned realIndex = item.Index;
                if (realIndex < dir->SubDirs.Size()) {
                    if (includeFolderSubItemsInFlatMode) {
                        _proxy->AddRealIndices(dir->SubDirs[realIndex], realIndices);
                        continue;
                    }
                    const CProxyDir &f = _proxy->Dirs[dir->SubDirs[realIndex]];
                    if (!f.IsLeaf())
                        continue;
                    arcIndex = (unsigned)f.ArcIndex;
                } else
                    arcIndex = dir->SubFiles[realIndex - dir->SubDirs.Size()];
            }
            realIndices.Add(arcIndex);
        }
    }
    HeapSort(realIndices.NonConstData(), realIndices.Size());
}

CAgent::CAgent()
    : _proxy(NULL), _proxy2(NULL), _updatePathPrefix_is_AltFolder(false), _isDeviceFile(false),
      _agentFolder(NULL) {}

CAgent::~CAgent() {
    if (_proxy)
        delete _proxy;
    if (_proxy2)
        delete _proxy2;
}

bool CAgent::CanUpdate() const {
    if (_proxy && _proxy->Are_Changed_LongPaths)
        return false;
    // FAR plugin uses empty agent to create new archive !!!
    if (_archiveLink.Arcs.Size() == 0)
        return true;
    if (_isDeviceFile)
        return false;
    if (_archiveLink.Arcs.Size() != 1)
        return false;
    if (_archiveLink.Arcs[0].ErrorInfo.ThereIsTail)
        return false;
    return true;
}

bool CAgent::IsThere_ReadOnlyArc() const {
    FOR_VECTOR(i, _archiveLink.Arcs) {
        const CArc &arc = _archiveLink.Arcs[i];
        // Original FormatIndex/UpdateEnabled lookup is supplied by the typed
        // file-only engine capability; the codec catalog is mandatory P2-01.
        if (arc.IsReadOnly || !ArchiveUpdateEnabled(arc.Archive))
            return true;
    }
    return false;
}

HRESULT CAgent::Init(const QList<ArchiveEntry> &entries, IFolderFolder *archiveProps,
                     const UString &archiveType, IUnknown *engineResource) {
    COM_TRY_BEGIN
    if (!archiveProps)
        return E_INVALIDARG;
    // Initial binding creates one source owner. Native ReOpen refreshes that
    // owner's metadata; CommonUpdateOperation restores its bound folder paths.
    if (!_archiveLink.Arcs.IsEmpty())
        return E_FAIL;
    UNUSED_VAR(entries)
    CArc &arc = _archiveLink.Arcs.AddNew();
    const HRESULT result =
        CreateArchivePropertyTransport(archiveProps, &arc.Archive, engineResource);
    if (result != S_OK) {
        _archiveLink.Release();
        return result;
    }
    arc.DefaultName = L"[Content]";
    arc.ArcStreamOffset = 0;
    arc.Offset = 0;
    arc.FileSize = 0;
    arc.AvailPhySize = 0;
    arc.PhySize = 0;
    arc.PhySize_Defined = false;
    if (auto *resource = dynamic_cast<QtArchiveFileResource *>(engineResource)) {
        RINOK(resource->ConfigureArc(arc))
    }
    _archiveLink.IsOpen = true;
    ArchiveType = archiveType;
    _archiveFilePath = arc.Path;
    NWindows::NFile::NFind::CFileInfo fi;
    _attrib = fi.Find(us2fs(_archiveFilePath)) ? fi.GetWinAttrib() : (DWORD)-1;
    _engineResource = engineResource;
    return S_OK;
    COM_TRY_END
}

HRESULT CAgent::ReOpen(IArchiveOpenCallback *openArchiveCallback) {
    COM_TRY_BEGIN
    if (_proxy2) {
        delete _proxy2;
        _proxy2 = NULL;
    }
    if (_proxy) {
        delete _proxy;
        _proxy = NULL;
    }

    CObjectVector<COpenType> incl;
    CIntVector exl;

    COpenOptions options;
    options.props = NULL;
    options.codecs = NULL; // Codec engine is supplied by the file-only transport.
    options.types = &incl;
    options.excludedFormats = &exl;
    options.stdInMode = false;
    options.filePath = _archiveFilePath;
    options.callback = openArchiveCallback;

    RINOK(_archiveLink.ReOpen(options))
    return ReadItems();
    COM_TRY_END
}

HRESULT CAgent::Close() {
    COM_TRY_BEGIN
    return _archiveLink.Close();
    COM_TRY_END
}

IUnknown *CAgentFolder::GetEngineResource() const {
    return _agentSpec ? _agentSpec->GetEngineResource() : nullptr;
}

HRESULT CAgentFolder::ReOpen(IArchiveOpenCallback *callback, IFolderFolder **folder) {
    COM_TRY_BEGIN
    if (!folder)
        return E_INVALIDARG;
    *folder = nullptr;
    UStringVector pathParts;
    bool isAltStreamFolder;
    GetPathParts(pathParts, isAltStreamFolder);
    RINOK(_agentSpec->ReOpen(callback))
    CMyComPtr<IFolderFolder> archiveFolder;
    RINOK(_agentSpec->BindToRootFolder(&archiveFolder))
    // Ordinary-folder restore from ArchiveFolderOut::CommonUpdateOperation.
    // Raw/alternate-stream binding remains mandatory with the proxy2 port.
    FOR_VECTOR(i, pathParts) {
        CMyComPtr<IFolderFolder> newFolder;
        archiveFolder->BindToFolder(pathParts[i], &newFolder);
        if (!newFolder)
            break;
        archiveFolder = newFolder;
    }
    *folder = archiveFolder.Detach();
    return S_OK;
    COM_TRY_END
}

HRESULT CAgent::ReadItems() {
    if (_proxy)
        return S_OK;
    {
        ThereIsPathProp = false;
        UInt32 numProps;
        RINOK(GetArchive()->GetNumberOfProperties(&numProps))
        for (UInt32 i = 0; i < numProps; i++) {
            CMyComBSTR name;
            PROPID propID;
            VARTYPE varType;
            RINOK(GetArchive()->GetPropertyInfo(i, &name, &propID, &varType))
            if (propID == kpidPath)
                ThereIsPathProp = true;
        }
    }
    _proxy = new CProxyArc();
    size_t ramSize;
    if (NWindows::NSystem::GetRamSize(ramSize)) {
        ramSize = ramSize / 4 * 3;
        _proxy->MemUsage_Limit = ramSize;
    }
    try {
        const HRESULT result = _proxy->Load(GetArc(), nullptr);
        if (result != S_OK) {
            delete _proxy;
            _proxy = NULL;
            return result;
        }
    } catch (...) {
        delete _proxy;
        _proxy = NULL;
        throw;
    }
    return S_OK;
}

HRESULT CAgent::BindToRootFolder(IFolderFolder **resultFolder) {
    COM_TRY_BEGIN
    *resultFolder = NULL;
    if (_archiveLink.Arcs.IsEmpty())
        return E_FAIL;
    RINOK(ReadItems())
    CAgentFolder *folderSpec = new CAgentFolder;
    CMyComPtr<IFolderFolder> rootFolder = folderSpec;
    folderSpec->Init(_proxy, k_Proxy_RootDirIndex, this);
    *resultFolder = rootFolder.Detach();
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CAgentFolder::Extract(const UInt32 *indices, UInt32 numItems, Int32 includeAltStreams,
                                   Int32 replaceAltStreamColon, NExtract::NPathMode::EEnum pathMode,
                                   NExtract::NOverwriteMode::EEnum overwriteMode,
                                   const wchar_t *path, Int32 testMode,
                                   IFolderArchiveExtractCallback *extractCallback2)) {
    COM_TRY_BEGIN

    if (!testMode && _agentSpec->_isHashHandler)
        return E_NOTIMPL;

    CArchiveExtractCallback *extractCallbackSpec = new CArchiveExtractCallback;
    CMyComPtr<IArchiveExtractCallback> extractCallback = extractCallbackSpec;
    UStringVector pathParts;
    bool isAltStreamFolder = false;
    bool isChangedPath = false;
    if (_proxy2)
        _proxy2->GetDirPathParts(_proxyDirIndex, pathParts, isAltStreamFolder);
    else
        _proxy->GetDirPathParts_isChanged(_proxyDirIndex, pathParts, isChangedPath);
    if (isChangedPath)
        return E_NOTIMPL;
    /*
    if (_flatMode)
      pathMode = NExtract::NPathMode::kNoPathnames;
    */

    extractCallbackSpec->InitForMulti(false, // multiArchives
                                      pathMode, overwriteMode, _zoneMode, k_keepEmptyDirPrefixes);

    if (extractCallback2)
        extractCallback2->SetTotal(_agentSpec->GetArc().GetEstmatedPhySize());

    FString pathU;
    if (path) {
        pathU = us2fs(path);
        if (!pathU.IsEmpty()
#ifdef _WIN32
            && !NWindows::NFile::NName::IsAltStreamPrefixWithColon(path)
#endif
        ) {
            NWindows::NFile::NName::NormalizeDirPathPrefix(pathU);
            NWindows::NFile::NDir::CreateComplexDir(pathU);
        }
    }

    CExtractNtOptions extractNtOptions;
    extractNtOptions.AltStreams.Val = IntToBool(includeAltStreams); // change it!!!
    extractNtOptions.AltStreams.Def = true;

    extractNtOptions.ReplaceColonForAltStream = IntToBool(replaceAltStreamColon);

    extractCallbackSpec->InitBeforeNewArchive();

#if defined(_WIN32) && !defined(UNDER_CE)
    if (_zoneMode != NExtract::NZoneIdMode::kNone) {
        ReadZoneFile_Of_BaseFile(us2fs(_agentSpec->_archiveFilePath), extractCallbackSpec->ZoneBuf);
        if (_zoneBuf.Size() != 0)
            extractCallbackSpec->ZoneBuf = _zoneBuf;
    }
#endif

    extractCallbackSpec->Init(extractNtOptions, NULL, &_agentSpec->GetArc(), extractCallback2,
                              false, // stdOutMode
                              IntToBool(testMode), pathU, pathParts, isAltStreamFolder,
                              (UInt64)(Int64)-1);

    if (_proxy2)
        extractCallbackSpec->SetBaseParentFolderIndex(
            (unsigned)_proxy2->Dirs[_proxyDirIndex].ArcIndex);

    // do we need another base folder for subfolders ?
    extractCallbackSpec->DirPathPrefix_for_HashFiles = _agentSpec->_hashBaseFolderPrefix;

    CUIntVector realIndices;
    GetRealIndices(indices, numItems,
                   (_loadAltStreams && _flatMode) ? false : IntToBool(includeAltStreams), // v26.03
                   false,        // includeFolderSubItemsInFlatMode
                   realIndices); //

#ifdef SUPPORT_LINKS

    if (!testMode) {
        RINOK(extractCallbackSpec->PrepareHardLinks(&realIndices))
    }

#endif

    {
        CArchiveExtractCallback_Closer ecsCloser(extractCallbackSpec);

        HRESULT res = _agentSpec->GetArchive()->Extract(realIndices.ConstData(), realIndices.Size(),
                                                        testMode, extractCallback);

        const HRESULT res2 = ecsCloser.Close();
        if (res == S_OK)
            res = res2;
        return res;
    }

    COM_TRY_END
}

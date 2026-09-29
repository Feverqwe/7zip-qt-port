// Adapted from 7-Zip CPP/7zip/UI/Agent/Agent.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// The CProxyArc folder walk/binding/property control flow is retained. The
// _proxy2 and engine operation branches remain outside this partial port.
#include "Agent.h"
#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "Common/ComTry.h"
#include "Common/Defs.h"
#include "Windows/PropVariant.h"

void CAgentFolder::LoadFolder(unsigned dirIndex) {
    CProxyItem item;
    item.DirIndex = dirIndex;
    const CProxyDir &dir = _proxy->Dirs[dirIndex];
    unsigned i;
    for (i = 0; i < dir.SubDirs.size(); i++) {
        item.Index = i;
        _items.Add(item);
        LoadFolder(dir.SubDirs[i]);
    }
    const unsigned start = dir.SubDirs.size();
    for (i = 0; i < dir.SubFiles.size(); i++) {
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

bool CAgentFolder::IsBound() const { return _proxy && _proxyDirIndex < _proxy->Dirs.size(); }

void CAgentFolder::Init(const CProxyArc *proxy, unsigned proxyDirIndex, CAgent *agent) {
    _proxy = proxy;
    _proxyDirIndex = proxyDirIndex;
    _agent = (IUnknown *)agent;
    _agentSpec = agent;
    _archiveProps = agent->GetArchive();
    _archiveType = agent->ArchiveType;
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
        *numItems = (UInt32)(dir->SubDirs.size() + dir->SubFiles.size());
    }
    return S_OK;
}

void CAgentFolder::GetPrefix(UInt32 index, UString &prefix) const {
    prefix.Empty();
    if (!_flatMode)
        return;
    const unsigned itemDirIndex = _items[index].DirIndex;
    if (itemDirIndex == _proxyDirIndex)
        return;
    QString path;
    _proxy->GetDirPath_as_Prefix_from_Base(itemDirIndex, path, _proxyDirIndex);
    prefix = ToUString(path);
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
        if (realIndex < dir->SubDirs.size()) {
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
                    prop = ToUString(item.Name);
                    break;
                default:
                    if (item.IsLeaf()) {
                        if (!_archiveProps)
                            return E_NOTIMPL;
                        return _archiveProps->GetProperty((unsigned)item.ArcIndex, propID, value);
                    }
                }
        } else {
            const unsigned arcIndex = dir->SubFiles[realIndex - dir->SubDirs.size()];
            switch (propID) {
            case kpidIsDir:
                prop = false;
                break;
            case kpidName:
                prop = ToUString(_proxy->Files[arcIndex].Name);
                break;
            default:
                if (!_archiveProps)
                    return E_NOTIMPL;
                return _archiveProps->GetProperty(arcIndex, propID, value);
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
    if (realIndex >= (UInt32)dir->SubDirs.size())
        return E_INVALIDARG;
    return BindToFolder_Internal(dir->SubDirs[realIndex], resultFolder);
    COM_TRY_END
}

Z7_COM7F_IMF(CAgentFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)) {
    COM_TRY_BEGIN
    *resultFolder = nullptr;
    if (!IsBound())
        return E_FAIL;
    const int index = _proxy->FindSubDir(_proxyDirIndex, ToQString(UString(name)));
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
        prop = ToUString(dir.Name);
        break;
    case kpidPath:
        prop = ToUString(_proxy->GetDirPath_as_Prefix(_proxyDirIndex));
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

CAgent::CAgent() : _proxy(NULL) {}

CAgent::~CAgent() {
    if (_proxy)
        delete _proxy;
}

HRESULT CAgent::Init(const QList<ArchiveEntry> &entries, IFolderFolder *archiveProps,
                     const UString &archiveType) {
    COM_TRY_BEGIN
    if (!archiveProps)
        return E_INVALIDARG;
    // A 7zz listing is immutable. Relisting creates a new owner, so bound folders
    // cannot observe a replaced/deleted proxy through an old borrowed pointer.
    if (_archiveProps)
        return E_FAIL;
    _entries = entries;
    ArchiveType = archiveType;
    _archiveProps = archiveProps;
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
    try {
        _proxy->Load(_entries);
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
    if (!_archiveProps)
        return E_FAIL;
    RINOK(ReadItems())
    CAgentFolder *folderSpec = new CAgentFolder;
    CMyComPtr<IFolderFolder> rootFolder = folderSpec;
    folderSpec->Init(_proxy, k_Proxy_RootDirIndex, this);
    *resultFolder = rootFolder.Detach();
    return S_OK;
    COM_TRY_END
}

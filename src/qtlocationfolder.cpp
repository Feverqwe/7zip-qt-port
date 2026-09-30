#include "qtlocationfolder.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/Common/Wildcard.h"
#include "Common/ComTry.h"
#include "Common/StringConvert.h"
#include "Windows/FileName.h"
#include "qtarchivefileresource.h"
#include "qtpanelfolder.h"
#include <QDir>
#include <QFile>

std::unique_ptr<IQtProgressTransport>
CAgentFolder::CreateProgressTransport(CProgressDialog &dialog) {
    auto *resource = dynamic_cast<QtArchiveFileResource *>(_agentSpec->GetEngineResource());
    return resource ? resource->CreateProgressTransport(dialog) : nullptr;
}

bool CAgentFolder::CanCopyFromFile() const {
    if (!_agentSpec || !_agentSpec->CanUpdate())
        return false;
    auto *resource = dynamic_cast<QtArchiveFileResource *>(_agentSpec->GetEngineResource());
    return resource && resource->HasUpdateService();
}

bool CAgentFolder::CanCopyTo() const {
    if (!_agentSpec)
        return false;
    auto *resource = dynamic_cast<QtArchiveFileResource *>(_agentSpec->GetEngineResource());
    return resource && resource->HasExtractService();
}

// Remaining unavailable operation bodies stay with their source owners.
Z7_COM7F_IMF(CAgentFolder::CreateFile(const wchar_t *, IProgress *)) { return E_NOTIMPL; }
Z7_COM7F_IMF(CAgentFolder::CopyFrom(Int32, const wchar_t *, const wchar_t *const *, UInt32,
                                    IProgress *)) {
    return E_NOTIMPL;
}
Z7_COM7F_IMF(CAgentFolder::SetProperty(UInt32, PROPID, const PROPVARIANT *, IProgress *)) {
    return E_NOTIMPL;
}

int QtArchiveDirectoryIndex(const CProxyArc &proxy, const QString &path) {
    if (proxy.Dirs.IsEmpty())
        return -1;
    unsigned dirIndex = k_Proxy_RootDirIndex;
    UStringVector parts;
    SplitPathToParts(ToUString(path), parts);
    if (path.isEmpty())
        return int(dirIndex);
    FOR_VECTOR(i, parts) {
        const int next = proxy.FindSubDir(dirIndex, parts[i]);
        if (next < 0)
            return -1;
        dirIndex = unsigned(next);
    }
    return int(dirIndex);
}

QString QtFilesystemPath(const FString &path) {
#ifdef _WIN32
    return QDir::fromNativeSeparators(QString::fromWCharArray(path));
#else
    // Native source bytes remain unchanged; Qt supplies its POSIX decoding
    // (including macOS NFD → NFC) at the presentation boundary.
    return QFile::decodeName(path.Ptr());
#endif
}

HRESULT CreateFilesystemFolder(const QString &path, IFolderFolder **resultFolder) {
    COM_TRY_BEGIN
    *resultFolder = nullptr;
    if (path.isEmpty())
        return E_INVALIDARG;
    const auto native = QDir::toNativeSeparators(path).toStdWString();
    FString fullPath;
    if (!NWindows::NFile::NName::GetFullPath(us2fs(native.c_str()), fullPath))
        return E_FAIL;
    NWindows::NFile::NName::NormalizeDirPathPrefix(fullPath);
    auto *folderSpec = new NFsFolder::CFSFolder;
    CMyComPtr<IFolderFolder> folder = folderSpec;
    RINOK(folderSpec->Init(fullPath))
    *resultFolder = folder.Detach();
    return S_OK;
    COM_TRY_END
}

HRESULT CreateArchiveMetadata(const QList<ArchiveEntry> &entries, IFolderFolder **metadata) {
    COM_TRY_BEGIN
    if (!metadata)
        return E_INVALIDARG;
    *metadata = nullptr;
    CObjectVector<CPanelItem> items;
    for (const ArchiveEntry &entry : entries) {
        CPanelItem item;
        item.RelPath = entry.path;
        item.MTimeText = entry.modified;
        item.IsFolder = entry.directory;
        item.SizeDefined = entry.size >= 0;
        if (item.SizeDefined)
            item.Size = UInt64(entry.size);
        item.PackSizeDefined = entry.packedSize >= 0;
        if (item.PackSizeDefined)
            item.PackSize = UInt64(entry.packedSize);
        items.Add(item);
    }
    CMyComPtr<IFolderFolder> props =
        new QtPanelFolder(QString(), items, QtPanelFolder::PropertyCatalog::ArchiveTransport);
    *metadata = props.Detach();
    return S_OK;
    COM_TRY_END
}

HRESULT CreateArchiveFolder(const QList<ArchiveEntry> &entries, const UString &archiveType,
                            IFolderFolder **resultFolder, IUnknown *engineResource) {
    COM_TRY_BEGIN
    if (!resultFolder)
        return E_INVALIDARG;
    *resultFolder = nullptr;
    CMyComPtr<IFolderFolder> props;
    RINOK(CreateArchiveMetadata(entries, &props))
    auto *agentSpec = new CAgent;
    CMyComPtr<IUnknown> agent = agentSpec;
    RINOK(agentSpec->Init(entries, props, archiveType, engineResource))
    agentSpec->AttachEngineResource(engineResource);
    return agentSpec->BindToRootFolder(resultFolder);
    COM_TRY_END
}

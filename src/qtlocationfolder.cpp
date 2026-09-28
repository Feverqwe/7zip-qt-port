#include "qtlocationfolder.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "Common/ComTry.h"
#include "Common/StringConvert.h"
#include "Windows/FileName.h"
#include "qtpanelfolder.h"
#include <QDir>

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

HRESULT CreateArchiveFolder(const QList<ArchiveEntry> &entries, const UString &archiveType,
                            IFolderFolder **resultFolder) {
    COM_TRY_BEGIN
    *resultFolder = nullptr;
    CObjectVector<CPanelItem> items;
    for (const ArchiveEntry &entry : entries) {
        CPanelItem item;
        item.RelPath = entry.path;
        item.IsFolder = entry.directory;
        item.SizeDefined = entry.size >= 0;
        if (item.SizeDefined)
            item.Size = UInt64(entry.size);
        item.PackSizeDefined = entry.packedSize >= 0;
        if (item.PackSizeDefined)
            item.PackSize = UInt64(entry.packedSize);
        items.Add(item);
    }
    CMyComPtr<IFolderFolder> props = new QtPanelFolder(QString(), items);
    auto *agentSpec = new CAgent;
    CMyComPtr<IUnknown> agent = agentSpec;
    RINOK(agentSpec->Init(entries, props, archiveType))
    return agentSpec->BindToRootFolder(resultFolder);
    COM_TRY_END
}

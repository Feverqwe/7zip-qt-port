// Adapted from 7-Zip CPP/7zip/UI/FileManager/FSFolder.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Source enumeration and properties use portable upstream filesystem services.
// Qt mutation services/callbacks remain a narrow integration boundary.
#include "FSFolder.h"
#include "7zip/PropID.h"
#include "CPP/Windows/FileDir.h"
#include "Common/ComTry.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "FSDrives.h"
#include "Windows/FileName.h"
#include "Windows/PropVariant.h"
#include "Windows/TimeUtils.h"
#include <QDir>
#include <QFile>

namespace NFsFolder {
namespace {
FString FsPath(const QString &path) {
    const auto wide = QDir::toNativeSeparators(path).toStdWString();
    return us2fs(wide.c_str());
}
QString QtPath(const FString &path) {
    return QDir::fromNativeSeparators(QString::fromWCharArray(fs2us(path)));
}
} // namespace

CFSFolder::CFSFolder(QString path) {
    FString prefix = FsPath(QDir::cleanPath(std::move(path)));
    NWindows::NFile::NName::NormalizeDirPathPrefix(prefix);
    Init(prefix);
}

HRESULT CFSFolder::Init(const FString &path) {
    _path = path;
#ifdef _WIN32
    _findChangeNotification.FindFirst(_path, false,
                                      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                                          FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                                          FILE_NOTIFY_CHANGE_LAST_WRITE);
    if (!_findChangeNotification.IsHandleAllocated()) {
        const HRESULT lastError = GetLastError_noZero_HRESULT();
        NWindows::NFile::NFind::CFindFile findFile;
        NWindows::NFile::NFind::CFileInfo fi;
        FString path2 = _path;
        path2.Add_Char('*');
        if (!findFile.FindFirst(path2, fi))
            return lastError;
    }
#else
    // The Windows source validates through notifications/FindFirst. POSIX has
    // no equivalent notification handle, so use the unchanged native stat service.
    if (!NWindows::NFile::NFind::DoesDirExist_FollowLink(_path))
        return E_FAIL;
#endif
    return S_OK;
}

HRESULT CFSFolder::LoadSubItems(int dirItem, const FString &relPrefix) {
    const unsigned startIndex = Folders.Size();
    {
        NWindows::NFile::NFind::CEnumerator enumerator;
        enumerator.SetDirPrefix(_path + relPrefix);
        CDirItem fi;
        fi.FolderStat_Defined = false;
        fi.NumFolders = 0;
        fi.NumFiles = 0;
        fi.Parent = dirItem;
        for (;;) {
            bool found;
#ifdef _WIN32
            if (!enumerator.Next(fi, found))
                return E_FAIL;
#else
            NWindows::NFile::NFind::CDirEntry entry;
            if (!enumerator.Next(entry, found))
                return E_FAIL;
            if (found && !enumerator.Fill_FileInfo(entry, fi, false))
                return E_FAIL;
#endif
            if (!found)
                break;
            if (fi.IsDir()) {
                fi.Size = 0;
                if (_flatMode)
                    Folders.Add(relPrefix + fi.Name + FCHAR_PATH_SEPARATOR);
            }
            fi.PackSize_Defined = false;
            fi.PackSize = fi.Size;
            Files.Add(fi);
        }
    }
    if (!_flatMode)
        return S_OK;
    const unsigned endIndex = Folders.Size();
    for (unsigned i = startIndex; i < endIndex; i++)
        RINOK(LoadSubItems((int)i, Folders[i]))
    return S_OK;
}

Z7_COM7F_IMF(CFSFolder::LoadItems()) {
    COM_TRY_BEGIN
    Int32 dummy;
    WasChanged(&dummy);
    Files.Clear();
    Folders.Clear();
    const HRESULT result = LoadSubItems(-1, FString());
    if (result != S_OK) {
        Files.Clear();
        Folders.Clear();
    }
    return result;
    COM_TRY_END
}

Z7_COM7F_IMF(CFSFolder::GetNumberOfItems(UInt32 *numItems)) {
    *numItems = Files.Size();
    return S_OK;
}

Z7_COM7F_IMF(CFSFolder::SetFlatMode(Int32 flatMode)) {
    _flatMode = IntToBool(flatMode);
    return S_OK;
}

Z7_COM7F_IMF(CFSFolder::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    if (index >= Files.Size())
        return E_INVALIDARG;
    NWindows::NCOM::CPropVariant prop;
    CDirItem &fi = Files[index];
    switch (propID) {
    case kpidIsDir:
        prop = fi.IsDir();
        break;
    case kpidIsAltStream:
        prop = false;
        break;
    case kpidName:
        prop = fs2us(fi.Name);
        break;
    case kpidSize:
        if (!fi.IsDir() || fi.FolderStat_Defined)
            prop = fi.Size;
        break;
    case kpidPackSize:
        // Native compressed allocation/links/comments are a remaining source scope.
        return E_NOTIMPL;
    case kpidAttrib:
        prop = (UInt32)fi.GetWinAttrib();
        break;
    case kpidCTime:
        PropVariant_SetFrom_FiTime(prop, fi.CTime);
        break;
    case kpidATime:
        PropVariant_SetFrom_FiTime(prop, fi.ATime);
        break;
    case kpidMTime:
        PropVariant_SetFrom_FiTime(prop, fi.MTime);
        break;
    case kpidComment:
    case kpidChangeTime:
    case kpidINode:
    case kpidLinks:
        return E_NOTIMPL;
    case kpidPrefix:
        if (fi.Parent >= 0)
            prop = fs2us(Folders[fi.Parent]);
        break;
    case kpidNumSubDirs:
        if (fi.IsDir() && fi.FolderStat_Defined)
            prop = fi.NumFolders;
        break;
    case kpidNumSubFiles:
        if (fi.IsDir() && fi.FolderStat_Defined)
            prop = fi.NumFiles;
        break;
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

static const Byte kProps[] = {kpidName,       kpidSize,        kpidMTime,    kpidCTime,
                              kpidATime,      kpidAttrib,      kpidPackSize, kpidComment,
                              kpidNumSubDirs, kpidNumSubFiles, kpidPrefix};

Z7_COM7F_IMF(CFSFolder::GetNumberOfProperties(UInt32 *numProperties)) {
    *numProperties = Z7_ARRAY_SIZE(kProps);
    if (!_flatMode)
        (*numProperties)--;
    return S_OK;
}

IMP_IFolderFolder_GetProp(CFSFolder::GetPropertyInfo, kProps)

    Z7_COM7F_IMF(CFSFolder::GetFolderProperty(PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    NWindows::NCOM::CPropVariant prop;
    switch (propID) {
    case kpidType:
        prop = "FSFolder";
        break;
    case kpidPath:
        prop = fs2us(_path);
        break;
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

HRESULT CFSFolder::BindToFolderSpec(CFSTR name, IFolderFolder **resultFolder) {
    *resultFolder = NULL;
    CFSFolder *folderSpec = new CFSFolder;
    CMyComPtr<IFolderFolder> subFolder = folderSpec;
    RINOK(folderSpec->Init(_path + name + FCHAR_PATH_SEPARATOR))
    *resultFolder = subFolder.Detach();
    return S_OK;
}

Z7_COM7F_IMF(CFSFolder::BindToFolder(UInt32 index, IFolderFolder **resultFolder)) {
    COM_TRY_BEGIN
    *resultFolder = NULL;
    if (index >= Files.Size())
        return E_INVALIDARG;
    const CDirItem &fi = Files[index];
    if (!fi.IsDir())
        return E_INVALIDARG;
    return BindToFolderSpec(GetRelPath(fi), resultFolder);
    COM_TRY_END
}

Z7_COM7F_IMF(CFSFolder::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)) {
    COM_TRY_BEGIN
    return BindToFolderSpec(us2fs(name), resultFolder);
    COM_TRY_END
}

Z7_COM7F_IMF(CFSFolder::BindToParentFolder(IFolderFolder **resultFolder)) {
    COM_TRY_BEGIN
    *resultFolder = NULL;
    if (_path.IsEmpty())
        return E_INVALIDARG;
#ifdef _WIN32
    if (NWindows::NFile::NName::IsDriveRootPath_SuperAllowed(_path)) {
        CFSDrives *drivesFolderSpec = new CFSDrives;
        CMyComPtr<IFolderFolder> drivesFolder = drivesFolderSpec;
        drivesFolderSpec->Init(false, NWindows::NFile::NName::IsSuperPath(_path));
        *resultFolder = drivesFolder.Detach();
        return S_OK;
    }
#else
    if (_path == FSTRING_PATH_SEPARATOR)
        return S_OK;
#endif
    int pos = _path.ReverseFind_PathSepar();
    if (pos < 0 || pos != (int)_path.Len() - 1)
        return E_FAIL;
    FString parentPath = _path.Left((unsigned)pos);
    pos = parentPath.ReverseFind_PathSepar();
    parentPath.DeleteFrom((unsigned)(pos + 1));
#ifdef _WIN32
    if (NWindows::NFile::NName::IsDrivePath_SuperAllowed(parentPath))
#else
    if (!parentPath.IsEmpty() && parentPath[0] == FCHAR_PATH_SEPARATOR)
#endif
    {
        CFSFolder *parentFolderSpec = new CFSFolder;
        CMyComPtr<IFolderFolder> parentFolder = parentFolderSpec;
        if (parentFolderSpec->Init(parentPath) == S_OK) {
            *resultFolder = parentFolder.Detach();
            return S_OK;
        }
    }
    return S_OK;
    COM_TRY_END
}

Z7_COM7F_IMF(CFSFolder::WasChanged(Int32 *wasChanged)) {
    bool wasChangedMain = false;
#ifdef _WIN32
    for (;;) {
        if (!_findChangeNotification.IsHandleAllocated())
            break;
        DWORD waitResult = ::WaitForSingleObject(_findChangeNotification, 0);
        if (waitResult != WAIT_OBJECT_0)
            break;
        _findChangeNotification.FindNext();
        wasChangedMain = true;
    }
#endif
    *wasChanged = BoolToInt(wasChangedMain);
    return S_OK;
}

void CFSFolder::AddPath(const QString &path) {
    const QFileInfo info(path);
    CDirItem item;
    if (!item.Find(FsPath(path), false))
        item.Name = FsPath(info.fileName());
    const QString relative = QDir(QtPath(_path)).relativeFilePath(info.absolutePath());
    if (relative != ".")
        item.Parent = Folders.Add(FsPath(relative + '/'));
    Files.Add(item);
}

FString CFSFolder::GetRelPath(const CDirItem &item) const {
    if (item.Parent < 0)
        return item.Name;
    return Folders[item.Parent] + item.Name;
}

void CFSFolder::GetAbsPath(const QString &name, QString &absPath) const {
    absPath.clear();
    if (!QDir::isAbsolutePath(name))
        absPath += QtPath(_path);
    absPath += name;
}

bool CFSFolder::CreateFolder(const QString &name) {
    QString absPath;
    GetAbsPath(name, absPath);
    if (QDir().mkdir(absPath))
        return true;
    const QFileInfo info(absPath);
    if (!info.exists() && !info.isSymLink())
        if (QDir().mkpath(absPath))
            return true;
    ErrorMessage = QStringLiteral("Не удалось создать папку: %1").arg(absPath);
    return false;
}

bool CFSFolder::CreateFile(const QString &name) {
    QString absPath;
    GetAbsPath(name, absPath);
    QFile outFile(absPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        ErrorMessage = QStringLiteral("Не удалось создать файл: %1").arg(absPath);
        return false;
    }
    return true;
}

bool CFSFolder::Rename(UInt32 index, const QString &newName) {
    if (index >= Files.Size())
        return false;
    const CDirItem &fi = Files[index];
    QString fullPrefix = QtPath(_path);
    if (fi.Parent >= 0)
        fullPrefix += QtPath(Folders[fi.Parent]);
    // Qt rename never replaces a target, including a dangling symlink.
    const QString target = fullPrefix + newName;
    if (QFileInfo(target).isSymLink() || !QDir().rename(fullPrefix + QtPath(fi.Name), target)) {
        ErrorMessage = QStringLiteral("Не удалось переименовать объект: %1").arg(target);
        return false;
    }
    return true;
}

bool CFSFolder::Delete(const UInt32 *indices, UInt32 numItems) {
    if (SetTotal && !SetTotal(numItems))
        return false;
    for (UInt32 i = 0; i < numItems; i++) {
        const UInt32 index = indices[i];
        if (index >= Files.Size())
            return false;
        bool result = true;
        {
            const CDirItem &fi = Files[index];
            const QString fullPath = QtPath(_path + GetRelPath(fi));
            if (CheckBreak && !CheckBreak(fullPath))
                return false;
            // Keep the existing panel policy for a directly selected link.
            // Links found inside a directory are unlinked without traversal.
            if (QFileInfo(fullPath).isSymLink())
                result = false;
            else if (fi.IsDir())
                result = NWindows::NFile::NDir::RemoveDirWithSubItems(fullPath, CheckBreak);
            else
                result = NWindows::NFile::NDir::DeleteFileAlways(fullPath);
            if (!result) {
                ErrorMessage = QStringLiteral("Не удалось удалить объект: %1").arg(fullPath);
                return false;
            }
        }
        const quint64 completed = i;
        if (SetCompleted && !SetCompleted(completed))
            return false;
    }
    return true;
}
} // namespace NFsFolder

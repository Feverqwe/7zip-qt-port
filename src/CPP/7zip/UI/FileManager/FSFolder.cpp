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

#ifdef _WIN32
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0501
#ifdef _APISETFILE_
EXTERN_C_BEGIN
WINBASEAPI DWORD WINAPI GetCompressedFileSizeW(LPCWSTR lpFileName, LPDWORD lpFileSizeHigh);
EXTERN_C_END
#endif
#endif
#endif

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

#ifndef UNDER_CE
bool MyGetCompressedFileSizeW(CFSTR path, UInt64 &size) {
#ifdef _WIN32
    using namespace NWindows::NFile::NName;
    DWORD highPart;
    DWORD lowPart = INVALID_FILE_SIZE;
    IF_USE_MAIN_PATH {
        lowPart = ::GetCompressedFileSizeW(fs2us(path), &highPart);
        if (lowPart != INVALID_FILE_SIZE || ::GetLastError() == NO_ERROR) {
            size = ((UInt64)highPart << 32) | lowPart;
            return true;
        }
    }
#ifdef Z7_LONG_PATH
    if (USE_SUPER_PATH) {
        UString superPath;
        if (GetSuperPath(path, superPath, USE_MAIN_PATH)) {
            lowPart = ::GetCompressedFileSizeW(superPath, &highPart);
            if (lowPart != INVALID_FILE_SIZE || ::GetLastError() == NO_ERROR) {
                size = ((UInt64)highPart << 32) | lowPart;
                return true;
            }
        }
    }
#endif
#else
    // The source Windows compressed-allocation API has no POSIX equivalent.
    // Report unavailable so its original fi.Size fallback remains authoritative.
    (void)path;
    (void)size;
#endif
    return false;
}
#endif

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
            if (CheckBreak && !CheckBreak(QtPath(_path + relPrefix)))
                return E_ABORT;
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
                bool recurse = _flatMode;
#ifdef _WIN32
                // Preserve the Qt workflow's no-follow recursive listing policy.
                recurse = recurse && !fi.Has_Attrib_ReparsePoint();
#endif
                if (recurse)
                    Folders.Add(relPrefix + fi.Name + FCHAR_PATH_SEPARATOR);
            }
            fi.PackSize_Defined = false;
            fi.PackSize = fi.Size;
            Files.Add(fi);
            if (SetCompleted && !SetCompleted(Files.Size()))
                return E_ABORT;
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
    Clear();
    HRESULT result;
    try {
        result = LoadSubItems(-1, FString());
    } catch (...) {
        Clear();
        throw;
    }
    if (result != S_OK)
        Clear();
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
#ifdef UNDER_CE
        prop = fi.Size;
#else
        if (!fi.PackSize_Defined) {
            fi.PackSize_Defined = true;
            if (fi.IsDir() || !MyGetCompressedFileSizeW(_path + GetRelPath(fi), fi.PackSize))
                fi.PackSize = fi.Size;
        }
        prop = fi.PackSize;
#endif
        break;
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
    FString sourcePath;
    GetAbsPath(name.toStdWString().c_str(), sourcePath);
    absPath = QtPath(sourcePath);
}

bool CFSFolder::CreateFolder(const QString &name) {
    if (CreateFolder(name.toStdWString().c_str(), nullptr) == S_OK)
        return true;
    QString absPath;
    GetAbsPath(name, absPath);
    ErrorMessage = QStringLiteral("Не удалось создать папку: %1").arg(absPath);
    return false;
}

bool CFSFolder::CreateFile(const QString &name) {
    if (CreateFile(name.toStdWString().c_str(), nullptr) != S_OK) {
        QString absPath;
        GetAbsPath(name, absPath);
        ErrorMessage = QStringLiteral("Не удалось создать файл: %1").arg(absPath);
        return false;
    }
    return true;
}

bool CFSFolder::Rename(UInt32 index, const QString &newName) {
    if (Rename(index, newName.toStdWString().c_str(), nullptr) != S_OK) {
        ErrorMessage = QStringLiteral("Не удалось переименовать объект: %1").arg(newName);
        return false;
    }
    return true;
}

class CMutationProgress final : public IProgress, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IProgress)
    CFSFolder &folder_;

  public:
    explicit CMutationProgress(CFSFolder &folder) : folder_(folder) {}
    Z7_COM7F_IMF(SetTotal(UInt64 total)) override {
        return !folder_.SetTotal || folder_.SetTotal(total) ? S_OK : E_ABORT;
    }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *completed)) override {
        return !folder_.SetCompleted || !completed || folder_.SetCompleted(*completed) ? S_OK
                                                                                       : E_ABORT;
    }
};

bool CFSFolder::Delete(const UInt32 *indices, UInt32 numItems) {
    ErrorMessage.clear();
    CMyComPtr<IProgress> progress = new CMutationProgress(*this);
    if (Delete(indices, numItems, progress) == S_OK)
        return true;
    if (ErrorMessage.isEmpty())
        ErrorMessage = QStringLiteral("Не удалось удалить выбранные объекты.");
    return false;
}

void CFSFolder::GetAbsPath(const wchar_t *name, FString &absPath) const {
    absPath.Empty();
    if (!NWindows::NFile::NName::IsAbsolutePath(name))
        absPath += _path;
    absPath += us2fs(name);
}

Z7_COM7F_IMF(CFSFolder::CreateFolder(const wchar_t *name, IProgress * /* progress */)) {
    FString absPath;
    GetAbsPath(name, absPath);
    if (NWindows::NFile::NDir::CreateDir(absPath))
        return S_OK;
#ifdef _WIN32
    if (::GetLastError() != ERROR_ALREADY_EXISTS)
#else
    if (errno != EEXIST)
#endif
        if (NWindows::NFile::NDir::CreateComplexDir(absPath))
            return S_OK;
    return GetLastError_noZero_HRESULT();
}

Z7_COM7F_IMF(CFSFolder::CreateFile(const wchar_t *name, IProgress * /* progress */)) {
    FString absPath;
    GetAbsPath(name, absPath);
    NWindows::NFile::NIO::COutFile outFile;
    if (!outFile.Create_NEW(absPath))
        return GetLastError_noZero_HRESULT();
    return S_OK;
}

Z7_COM7F_IMF(CFSFolder::Rename(UInt32 index, const wchar_t *newName, IProgress * /* progress */)) {
    if (index >= Files.Size())
        return E_NOTIMPL;
    const CDirItem &fi = Files[index];
    FString fullPrefix = _path;
    if (fi.Parent >= 0)
        fullPrefix += Folders[fi.Parent];
    const QFileInfo target(QtPath(fullPrefix + us2fs(newName)));
    if (target.exists() || target.isSymLink())
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    if (!NWindows::NFile::NDir::MyMoveFile(fullPrefix + fi.Name, fullPrefix + us2fs(newName)))
        return GetLastError_noZero_HRESULT();
    return S_OK;
}

Z7_COM7F_IMF(CFSFolder::Delete(const UInt32 *indices, UInt32 numItems, IProgress *progress)) {
    if (!progress)
        return E_INVALIDARG;
    RINOK(progress->SetTotal(numItems))
    for (UInt32 i = 0; i < numItems; i++) {
        UInt32 index = indices[i];
        if (index >= Files.Size())
            return E_INVALIDARG;
        bool result = true;
        {
            const CDirItem &fi = Files[index];
            const FString fullPath = _path + GetRelPath(fi);
            // Existing Qt transport refuses directly selected symbolic links.
            // Source filesystem service retains recursive no-follow deletion.
            const QString path = QtPath(fullPath);
            if (QFileInfo(path).isSymLink())
                return E_NOTIMPL;
            if (CheckBreak && !CheckBreak(path))
                return E_ABORT;
            if (fi.IsDir()) {
                bool canceled = false;
                result =
                    NWindows::NFile::NDir::RemoveDirWithSubItems(path, [&](const QString &item) {
                        if (CheckBreak && !CheckBreak(item)) {
                            canceled = true;
                            return false;
                        }
                        return true;
                    });
                if (canceled)
                    return E_ABORT;
            } else
                result = NWindows::NFile::NDir::DeleteFileAlways(path);
        }
        if (!result) {
            ErrorMessage =
                QStringLiteral("Не удалось удалить: ") + QtPath(_path + GetRelPath(Files[index]));
            return GetLastError_noZero_HRESULT();
        }
        const UInt64 completed = i;
        RINOK(progress->SetCompleted(&completed))
    }
    return S_OK;
}

// Partial interface: full CopyTo/SetProperty consumers are mandatory P1-03/
// P1-04. The current Qt transport continues to own those workflows; native
// callers receive an explicit unavailable result instead of a fake success.
Z7_COM7F_IMF(CFSFolder::CopyTo(Int32, const UInt32 *, UInt32, Int32, Int32, const wchar_t *,
                               IFolderOperationsExtractCallback *)) {
    return E_NOTIMPL;
}
Z7_COM7F_IMF(CFSFolder::SetProperty(UInt32, PROPID, const PROPVARIANT *, IProgress *)) {
    return E_NOTIMPL;
}
// These two unavailable bodies are also the original upstream implementation.
Z7_COM7F_IMF(CFSFolder::CopyFrom(Int32, const wchar_t *, const wchar_t *const *, UInt32,
                                 IProgress *)) {
    return E_NOTIMPL;
}
Z7_COM7F_IMF(CFSFolder::CopyFromFile(UInt32, const wchar_t *, IProgress *)) { return E_NOTIMPL; }
} // namespace NFsFolder

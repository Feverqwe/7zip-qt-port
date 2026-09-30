// Adapted from 7-Zip CPP/7zip/UI/FileManager/FSDrives.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Read-only logical-drive/mount branch; physical-volume operations remain required.
#include "FSDrives.h"
#include "7zip/PropID.h"
#include "Common/ComTry.h"
#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "FSFolder.h"
#include "Windows/FileName.h"
#include "Windows/FileSystem.h"
#include "Windows/PropVariant.h"
#include <QCoreApplication>
#include <QFileInfo>

using namespace NWindows;
using namespace NFile;
static const char *const kVolPrefix = "\\\\.\\";
static const char *const kSuperPrefix = "\\\\?\\";
#ifndef _WIN32
static constexpr UINT DRIVE_CDROM = 5; // Win32 type value for the copied AddExt body.
#endif
static const Byte kProps[] = {kpidName,       kpidTotalSize,  kpidFreeSpace,  kpidType,
                              kpidVolumeName, kpidFileSystem, kpidClusterSize};
static const char *const kDriveTypes[] = {"Unknown", "No Root Dir", "Removable", "Fixed",
                                          "Remote",  "CD-ROM",      "RAM disk"};

Z7_COM7F_IMF(CFSDrives::LoadItems()) {
    COM_TRY_BEGIN
    _drives.Clear();
    if (_volumeMode)
        return E_NOTIMPL;
#ifdef _WIN32
    FStringVector driveStrings;
    NFind::MyGetLogicalDriveStrings(driveStrings);
    FOR_VECTOR(i, driveStrings) {
        CDriveInfo di;
        const FString &driveName = driveStrings[i];
        di.FullSystemName = driveName;
        if (!driveName.IsEmpty())
            di.Name.SetFrom(driveName, driveName.Len() - 1);
        di.ClusterSize = 0;
        di.DriveSize = 0;
        di.FreeSpace = 0;
        di.DriveType = NSystem::MyGetDriveType(driveName);
        bool needRead = true;
        if (di.DriveType == DRIVE_CDROM || di.DriveType == DRIVE_REMOVABLE)
            needRead = false;
        if (needRead) {
            DWORD volumeSerialNumber, maximumComponentLength, fileSystemFlags;
            NSystem::MyGetVolumeInformation(driveName, di.VolumeName, &volumeSerialNumber,
                                            &maximumComponentLength, &fileSystemFlags,
                                            di.FileSystemName);
            NSystem::MyGetDiskFreeSpace(driveName, di.ClusterSize, di.DriveSize, di.FreeSpace);
            di.KnownSizes = true;
            di.KnownSize = true;
        }
        _drives.Add(di);
    }
    return S_OK;
#else
    for (const QtVolumeMount &mount : _mountProvider()) {
        const std::wstring path = mount.path.toStdWString();
        const std::wstring name = mount.name.toStdWString();
        const std::wstring fileSystem = mount.fileSystem.toStdWString();
        CDriveInfo di;
        di.FullSystemName = us2fs(path.c_str());
        di.Name = us2fs(name.c_str());
        di.VolumeName = name.c_str();
        di.FileSystemName = fileSystem.c_str();
        di.DriveType = 3;
        di.ClusterSize = 0;
        if (mount.totalBytes >= 0) {
            di.DriveSize = UInt64(mount.totalBytes);
            di.KnownSize = true;
        }
        if (mount.availableBytes >= 0) {
            di.FreeSpace = UInt64(mount.availableBytes);
            di.KnownSizes = true;
        }
        _drives.Add(di);
    }
    return S_OK;
#endif
    COM_TRY_END
}

Z7_COM7F_IMF(CFSDrives::GetNumberOfItems(UInt32 *numItems)) {
    *numItems = _drives.Size();
    return S_OK;
}

Z7_COM7F_IMF(CFSDrives::GetProperty(UInt32 itemIndex, PROPID propID, PROPVARIANT *value)) {
    if (itemIndex >= _drives.Size())
        return E_INVALIDARG;
    NCOM::CPropVariant prop;
    const CDriveInfo &di = _drives[itemIndex];
    switch (propID) {
    case kpidIsDir:
        prop = !_volumeMode;
        break;
    case kpidName:
        prop = fs2us(di.Name);
        break;
    case kpidOutName:
        if (!di.Name.IsEmpty() && di.Name.Back() == ':') {
            FString s = di.Name;
            s.DeleteBack();
            AddExt(s, itemIndex);
            prop = fs2us(s);
        }
        break;
    case kpidTotalSize:
        if (di.KnownSize)
            prop = di.DriveSize;
        break;
    case kpidFreeSpace:
        if (di.KnownSizes)
            prop = di.FreeSpace;
        break;
    case kpidClusterSize:
        if (di.KnownSizes)
            prop = di.ClusterSize;
        break;
    case kpidType:
        if (di.DriveType < Z7_ARRAY_SIZE(kDriveTypes))
            prop = kDriveTypes[di.DriveType];
        break;
    case kpidVolumeName:
        prop = di.VolumeName;
        break;
    case kpidFileSystem:
        prop = di.FileSystemName;
        break;
    }
    prop.Detach(value);
    return S_OK;
}

HRESULT CFSDrives::BindToFolderSpec(CFSTR name, IFolderFolder **resultFolder) {
    *resultFolder = NULL;
    if (_volumeMode)
        return S_OK;
#ifndef _WIN32
    const QString mountPath = QString::fromWCharArray(fs2us(name));
    bool mounted = false;
    for (const QtVolumeMount &mount : _mountProvider())
        if (mount.path == mountPath) {
            mounted = true;
            break;
        }
    if (!mounted || !QFileInfo(mountPath).isDir() || !QFileInfo(mountPath).isReadable())
        return E_FAIL;
#endif
    NFsFolder::CFSFolder *fsFolderSpec = new NFsFolder::CFSFolder;
    CMyComPtr<IFolderFolder> subFolder = fsFolderSpec;
    FString path;
    if (_superMode)
        path = kSuperPrefix;
    path += name;
    RINOK(fsFolderSpec->Init(path))
    *resultFolder = subFolder.Detach();
    return S_OK;
}

Z7_COM7F_IMF(CFSDrives::BindToFolder(UInt32 index, IFolderFolder **resultFolder)) {
    COM_TRY_BEGIN
    *resultFolder = NULL;
    if (index >= _drives.Size())
        return E_INVALIDARG;
    const CDriveInfo &di = _drives[index];
    return BindToFolderSpec(di.FullSystemName, resultFolder);
    COM_TRY_END
}

Z7_COM7F_IMF(CFSDrives::BindToFolder(const wchar_t *name, IFolderFolder **resultFolder)) {
    COM_TRY_BEGIN
    return BindToFolderSpec(us2fs(name), resultFolder);
    COM_TRY_END
}

Z7_COM7F_IMF(CFSDrives::BindToParentFolder(IFolderFolder **resultFolder)) {
    *resultFolder = NULL;
    return S_OK;
}

IMP_IFolderFolder_Props(CFSDrives)

    Z7_COM7F_IMF(CFSDrives::GetFolderProperty(PROPID propID, PROPVARIANT *value)) {
    COM_TRY_BEGIN
    NCOM::CPropVariant prop;
    switch (propID) {
    case kpidType:
        prop = "FSDrives";
        break;
    case kpidPath:
#ifndef _WIN32
    {
        const auto name = QCoreApplication::translate("SevenzipDesktop", "Тома").toStdWString();
        prop = UString(name.c_str()) + WCHAR_PATH_SEPARATOR;
    } break;
#endif
        if (_volumeMode)
            prop = kVolPrefix;
        else if (_superMode)
            prop = kSuperPrefix;
        else {
            const auto name =
                QCoreApplication::translate("SevenzipDesktop", "Компьютер").toStdWString();
            prop = UString(name.c_str()) + WCHAR_PATH_SEPARATOR;
        }
        break;
    }
    prop.Detach(value);
    return S_OK;
    COM_TRY_END
}

void CFSDrives::AddExt(FString &s, unsigned index) const {
    s.Add_Dot();
    const CDriveInfo &di = _drives[index];
    UString n = di.FileSystemName;
    n.MakeLower_Ascii();
    const char *ext;
    if (di.DriveType == DRIVE_CDROM)
        ext = "iso";
    else {
        unsigned i;
        for (i = 0; i < n.Len(); i++) {
            const wchar_t c = n[i];
            if (c < 'a' || c > 'z')
                break;
        }
        if (i != 0) {
            n.DeleteFrom(i);
            s += us2fs(n);
            return;
        }
        ext = "img";
    }
    s += ext;
}

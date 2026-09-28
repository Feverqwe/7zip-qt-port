// Adapted from 7-Zip CPP/7zip/UI/FileManager/FSDrives.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "7zip/UI/FileManager/IFolder.h"
#include "Common/MyCom.h"
#include "Common/MyString.h"

struct CDriveInfo {
    FString Name;
    FString FullSystemName;
    UInt64 DriveSize;
    UInt64 FreeSpace;
    UInt64 ClusterSize;
    UString VolumeName;
    UString FileSystemName;
    UINT DriveType;
    bool KnownSize;
    bool KnownSizes;
    bool IsPhysicalDrive;
    CDriveInfo() : KnownSize(false), KnownSizes(false), IsPhysicalDrive(false) {}
};

class CFSDrives final : public IFolderFolder, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderFolder)
  public:
    Z7_IFACE_COM7_IMP(IFolderFolder)
    void Init(bool volMode = false, bool superMode = false) {
        _volumeMode = volMode;
        _superMode = superMode;
    }

  private:
    CObjectVector<CDriveInfo> _drives;
    bool _volumeMode = false;
    bool _superMode = false;
    HRESULT BindToFolderSpec(CFSTR name, IFolderFolder **resultFolder);
    void AddExt(FString &s, unsigned index) const;
};

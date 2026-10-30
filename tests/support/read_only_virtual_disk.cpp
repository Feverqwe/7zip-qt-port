#include "support/read_only_virtual_disk.h"
#include <QDir>
#include <initguid.h>
#include <virtdisk.h>

ReadOnlyVirtualDisk::ReadOnlyVirtualDisk(const QString &path) {
    // Use only a privilege already held by this test process; restore its old
    // state after detach. No account rights, UAC or machine policy are changed.
    if (!token_.OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY)) {
        error_ = GetLastError();
        return;
    }
    TOKEN_PRIVILEGES privilege{};
    privilege.PrivilegeCount = 1;
    if (!LookupPrivilegeValueW(nullptr, L"SeManageVolumePrivilege",
                               &privilege.Privileges[0].Luid)) {
        error_ = GetLastError();
        return;
    }
    privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    DWORD previousBytes = sizeof(previousPrivilege_);
    SetLastError(ERROR_SUCCESS);
    if (!token_.AdjustPrivileges(false, &privilege, previousBytes, &previousPrivilege_,
                                 &previousBytes)) {
        error_ = GetLastError();
        return;
    }
    privilegeAdjusted_ = true;
    error_ = GetLastError();
    if (error_ != ERROR_SUCCESS)
        return;
    const std::wstring native = QDir::toNativeSeparators(path).toStdWString();
    VIRTUAL_STORAGE_TYPE storage{VIRTUAL_STORAGE_TYPE_DEVICE_VHD,
                                 VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT};
    CREATE_VIRTUAL_DISK_PARAMETERS create{};
    create.Version = CREATE_VIRTUAL_DISK_VERSION_1;
    create.Version1.MaximumSize = Size;
    create.Version1.SectorSizeInBytes = 512;
    // No source/parent path: the native provider creates an isolated blank disk.
    error_ = CreateVirtualDisk(&storage, native.c_str(), VIRTUAL_DISK_ACCESS_ALL, nullptr,
                               CREATE_VIRTUAL_DISK_FLAG_FULL_PHYSICAL_ALLOCATION, 0, &create,
                               nullptr, &disk_);
    if (error_ != ERROR_SUCCESS)
        return;
    ATTACH_VIRTUAL_DISK_PARAMETERS attach{};
    attach.Version = ATTACH_VIRTUAL_DISK_VERSION_1;
    error_ = AttachVirtualDisk(disk_, nullptr,
                               ATTACH_VIRTUAL_DISK_FLAG(ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY |
                                                        ATTACH_VIRTUAL_DISK_FLAG_NO_DRIVE_LETTER),
                               0, &attach, nullptr);
    if (error_ != ERROR_SUCCESS)
        return;
    attached_ = true;
    wchar_t device[MAX_PATH]{};
    ULONG bytes = sizeof(device);
    error_ = GetVirtualDiskPhysicalPath(disk_, &bytes, device);
    if (error_ == ERROR_SUCCESS)
        device_ = QString::fromWCharArray(device);
}

ReadOnlyVirtualDisk::~ReadOnlyVirtualDisk() {
    detach();
    if (disk_ != INVALID_HANDLE_VALUE)
        CloseHandle(disk_);
    if (privilegeAdjusted_)
        token_.AdjustPrivileges(&previousPrivilege_);
}

DWORD ReadOnlyVirtualDisk::detach() {
    if (!attached_)
        return ERROR_SUCCESS;
    const DWORD error = DetachVirtualDisk(disk_, DETACH_VIRTUAL_DISK_FLAG_NONE, 0);
    if (error == ERROR_SUCCESS) {
        attached_ = false;
        device_.clear();
    }
    return error;
}

QByteArray ReadOnlyVirtualDisk::readDevice() const {
    if (!valid())
        return {};
    // Resolve from this live owned VHD handle again; never accept a caller's
    // physical-device path or a previously detached device number.
    wchar_t device[MAX_PATH]{};
    ULONG bytes = sizeof(device);
    if (GetVirtualDiskPhysicalPath(disk_, &bytes, device) != ERROR_SUCCESS ||
        QString::fromWCharArray(device) != device_)
        return {};
    HANDLE input = CreateFileW(device, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                               OPEN_EXISTING, 0, nullptr);
    if (input == INVALID_HANDLE_VALUE)
        return {};
    QByteArray data;
    data.reserve(qsizetype(Size));
    char buffer[64 * 1024];
    while (quint64(data.size()) < Size) {
        DWORD read = 0;
        if (!ReadFile(input, buffer, sizeof(buffer), &read, nullptr) || !read) {
            data.clear();
            break;
        }
        data.append(buffer, read);
    }
    CloseHandle(input);
    return data;
}

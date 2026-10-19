#include "deviceaccess_win.h"
#include "Windows/Handle.h"
#include <QCoreApplication>
#include <QSet>
#include <algorithm>
#include <cfgmgr32.h>
#include <cstddef>
#include <cstring>
#include <initguid.h>

#include <devpkey.h>
#include <setupapi.h>
#include <winioctl.h>

namespace {
struct Candidate {
    ExternalDevice Device;
    DWORD Number = 0;
};

template <typename T> bool Query(HANDLE handle, DWORD control, T &value) {
    DWORD bytes = 0;
    return DeviceIoControl(handle, control, nullptr, 0, &value, sizeof(value), &bytes, nullptr) &&
           bytes >= sizeof(value);
}

QString StorageText(const QByteArray &data, DWORD offset) {
    if (!offset || offset >= DWORD(data.size()))
        return {};
    const auto end = data.indexOf('\0', qsizetype(offset));
    return end < 0 ? QString()
                   : QString::fromLocal8Bit(data.constData() + offset, end - offset).trimmed();
}

bool Inspect(HDEVINFO devices, SP_DEVINFO_DATA &info, const QString &path, Candidate &result) {
    const auto name = path.toStdWString();
    HANDLE raw = CreateFileW(name.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                             OPEN_EXISTING, 0, nullptr);
    if (raw == INVALID_HANDLE_VALUE)
        return false;
    NWindows::CHandle handle;
    handle.Attach(raw);
    QByteArray geometry(sizeof(DISK_GEOMETRY_EX) + 256, Qt::Uninitialized);
    DWORD bytes = 0;
    if (!DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0, geometry.data(),
                         DWORD(geometry.size()), &bytes, nullptr) ||
        bytes < offsetof(DISK_GEOMETRY_EX, Data))
        return false;
    const auto *disk = reinterpret_cast<const DISK_GEOMETRY_EX *>(geometry.constData());
    STORAGE_DEVICE_NUMBER number{};
    if (!Query(handle, IOCTL_STORAGE_GET_DEVICE_NUMBER, number) ||
        number.DeviceType != FILE_DEVICE_DISK || !disk->Geometry.BytesPerSector ||
        disk->DiskSize.QuadPart <= 0)
        return false;
    STORAGE_PROPERTY_QUERY query{};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    QByteArray storage(4096, Qt::Uninitialized);
    if (!DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
                         storage.data(), DWORD(storage.size()), &bytes, nullptr) ||
        bytes < offsetof(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties))
        return false;
    storage.resize(bytes);
    const auto *descriptor =
        reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR *>(storage.constData());
    STORAGE_HOTPLUG_INFO hotplug{};
    Query(handle, IOCTL_STORAGE_GET_HOTPLUG_INFO, hotplug);
    if (!(descriptor->RemovableMedia || hotplug.DeviceHotplug ||
          descriptor->BusType == BusTypeUsb || descriptor->BusType == BusType1394))
        return false;
    FILETIME arrival{};
    DEVPROPTYPE type = 0;
    if (!SetupDiGetDevicePropertyW(devices, &info, &DEVPKEY_Device_LastArrivalDate, &type,
                                   reinterpret_cast<PBYTE>(&arrival), sizeof(arrival), nullptr,
                                   0) ||
        type != DEVPROP_TYPE_FILETIME)
        return false;
    wchar_t instance[MAX_DEVICE_ID_LEN]{};
    if (!SetupDiGetDeviceInstanceIdW(devices, &info, instance, MAX_DEVICE_ID_LEN, nullptr))
        return false;
    DWORD changes = 0;
    const bool verified = Query(handle, IOCTL_STORAGE_CHECK_VERIFY2, changes);
    if (descriptor->RemovableMedia && !verified)
        return false;
    result.Number = number.DeviceNumber;
    auto &device = result.Device;
    device.id = path;
    device.name = (StorageText(storage, descriptor->VendorIdOffset) + ' ' +
                   StorageText(storage, descriptor->ProductIdOffset))
                      .trimmed();
    if (device.name.isEmpty())
        device.name = QCoreApplication::translate("deviceaccess", "External disk");
    device.identity = QStringLiteral("%1:%2:%3:%4:%5")
                          .arg(QString::fromWCharArray(instance),
                               StorageText(storage, descriptor->SerialNumberOffset))
                          .arg((quint64(arrival.dwHighDateTime) << 32) | arrival.dwLowDateTime)
                          .arg(result.Number)
                          .arg(verified ? QString::number(changes) : QStringLiteral("fixed"));
    device.size = quint64(disk->DiskSize.QuadPart);
    device.sectorSize = disk->Geometry.BytesPerSector;
    return device.sectorSize >= 512 && device.sectorSize <= 65536 &&
           !(device.sectorSize & (device.sectorSize - 1)) && device.size % device.sectorSize == 0;
}

QSet<DWORD> MountedDisks(bool &unknown) {
    unknown = false;
    QSet<DWORD> result;
    wchar_t volume[MAX_PATH]{};
    HANDLE iterator = FindFirstVolumeW(volume, MAX_PATH);
    if (iterator == INVALID_HANDLE_VALUE) {
        unknown = GetLastError() != ERROR_NO_MORE_FILES;
        return result;
    }
    do {
        // Unsupported/raw filesystems have no mounted Windows filesystem.
        if (!GetVolumeInformationW(volume, nullptr, 0, nullptr, nullptr, nullptr, nullptr, 0)) {
            const DWORD error = GetLastError();
            if (error != ERROR_UNRECOGNIZED_VOLUME && error != ERROR_NOT_READY &&
                error != ERROR_NO_MEDIA_IN_DRIVE)
                unknown = true;
            continue;
        }
        QString name = QString::fromWCharArray(volume);
        if (name.endsWith('\\'))
            name.chop(1);
        HANDLE raw = CreateFileW(name.toStdWString().c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 nullptr, OPEN_EXISTING, 0, nullptr);
        if (raw == INVALID_HANDLE_VALUE) {
            unknown = true;
            continue;
        }
        NWindows::CHandle handle;
        handle.Attach(raw);
        QByteArray data(64 * 1024, Qt::Uninitialized);
        DWORD bytes = 0;
        if (!DeviceIoControl(handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, data.data(),
                             DWORD(data.size()), &bytes, nullptr)) {
            unknown = true;
            continue;
        }
        if (bytes < offsetof(VOLUME_DISK_EXTENTS, Extents)) {
            unknown = true;
            continue;
        }
        const auto *extents = reinterpret_cast<const VOLUME_DISK_EXTENTS *>(data.constData());
        const quint64 needed = offsetof(VOLUME_DISK_EXTENTS, Extents) +
                               quint64(extents->NumberOfDiskExtents) * sizeof(DISK_EXTENT);
        if (needed > bytes) {
            unknown = true;
            continue;
        }
        for (DWORD i = 0; i < extents->NumberOfDiskExtents; ++i)
            result.insert(extents->Extents[i].DiskNumber);
    } while (FindNextVolumeW(iterator, volume, MAX_PATH));
    if (GetLastError() != ERROR_NO_MORE_FILES)
        unknown = true;
    FindVolumeClose(iterator);
    return result;
}

QList<Candidate> Snapshot(QString &error) {
    error.clear();
    QList<Candidate> result;
    HDEVINFO devices = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_DISK, nullptr, nullptr,
                                            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devices == INVALID_HANDLE_VALUE) {
        error = QCoreApplication::translate("deviceaccess", "Could not inspect attached devices.");
        return result;
    }
    bool unknown = false;
    const auto mounted = MountedDisks(unknown);
    SP_DEVICE_INTERFACE_DATA item{};
    item.cbSize = sizeof(item);
    for (DWORD index = 0;
         SetupDiEnumDeviceInterfaces(devices, nullptr, &GUID_DEVINTERFACE_DISK, index, &item);
         ++index) {
        DWORD size = 0;
        SetupDiGetDeviceInterfaceDetailW(devices, &item, nullptr, 0, &size, nullptr);
        if (size < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) || size > 65536)
            continue;
        QByteArray buffer(size, Qt::Uninitialized);
        auto *detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W *>(buffer.data());
        detail->cbSize = sizeof(*detail);
        SP_DEVINFO_DATA info{};
        info.cbSize = sizeof(info);
        if (!SetupDiGetDeviceInterfaceDetailW(devices, &item, detail, size, nullptr, &info))
            continue;
        Candidate candidate;
        if (Inspect(devices, info, QString::fromWCharArray(detail->DevicePath), candidate)) {
            candidate.Device.mounted = unknown || mounted.contains(candidate.Number);
            result.append(candidate);
        }
    }
    SetupDiDestroyDeviceInfoList(devices);
    return result;
}

bool Same(const ExternalDevice &left, const ExternalDevice &right) {
    return left.id.compare(right.id, Qt::CaseInsensitive) == 0 && left.identity == right.identity &&
           left.size == right.size && left.sectorSize == right.sectorSize;
}
} // namespace

QList<ExternalDevice> EnumerateExternalDevices(QString &error) {
    QList<ExternalDevice> result;
    for (const auto &candidate : Snapshot(error))
        result.append(candidate.Device);
    return result;
}

HANDLE WindowsOpenExternalDevice(const ExternalDevice &device, DWORD &error) {
    QString message;
    const auto candidates = Snapshot(message);
    const auto found = std::find_if(candidates.cbegin(), candidates.cend(),
                                    [&](const auto &entry) { return Same(device, entry.Device); });
    if (found == candidates.cend()) {
        error = ERROR_DEVICE_NOT_CONNECTED;
        return INVALID_HANDLE_VALUE;
    }
    if (found->Device.mounted) {
        error = ERROR_BUSY;
        return INVALID_HANDLE_VALUE;
    }
    HANDLE handle = CreateFileW(device.id.toStdWString().c_str(), GENERIC_READ, FILE_SHARE_READ,
                                nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        return handle;
    }
    STORAGE_DEVICE_NUMBER number{};
    GET_LENGTH_INFORMATION length{};
    const auto after = Snapshot(message);
    const bool same = std::any_of(after.cbegin(), after.cend(), [&](const auto &entry) {
        return Same(device, entry.Device) && !entry.Device.mounted;
    });
    if (!same || !Query(handle, IOCTL_STORAGE_GET_DEVICE_NUMBER, number) ||
        number.DeviceNumber != found->Number ||
        !Query(handle, IOCTL_DISK_GET_LENGTH_INFO, length) ||
        quint64(length.Length.QuadPart) != device.size) {
        CloseHandle(handle);
        error = ERROR_DEVICE_NOT_CONNECTED;
        return INVALID_HANDLE_VALUE;
    }
    error = ERROR_SUCCESS;
    return handle;
}

DeviceAccessResult OpenExternalDevice(const ExternalDevice &device, bool authorize,
                                      const std::function<bool()> &cancelled) {
    DeviceAccessResult result;
    if (cancelled && cancelled()) {
        result.status = DeviceAccessStatus::Cancelled;
        return result;
    }
    DWORD error = ERROR_SUCCESS;
    HANDLE handle = WindowsOpenExternalDevice(device, error);
    if (handle != INVALID_HANDLE_VALUE) {
        result.handle = AdoptWindowsReadHandle(handle);
        result.status = result.handle ? DeviceAccessStatus::Ready : DeviceAccessStatus::Failed;
        if (cancelled && cancelled()) {
            result.handle.reset();
            result.status = DeviceAccessStatus::Cancelled;
        }
        return result;
    }
    if (cancelled && cancelled())
        result.status = DeviceAccessStatus::Cancelled;
    else if (error == ERROR_BUSY || error == ERROR_SHARING_VIOLATION)
        result.status = DeviceAccessStatus::Busy;
    else if (error == ERROR_ACCESS_DENIED || error == ERROR_PRIVILEGE_NOT_HELD) {
        if (authorize)
            return WindowsAuthorizeExternalDevice(device, cancelled);
        result.status = DeviceAccessStatus::PermissionRequired;
    }
    result.error =
        QCoreApplication::translate("deviceaccess", "Could not open the selected device.");
    return result;
}

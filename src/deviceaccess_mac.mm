#include "deviceaccess_mac.h"
#include "deviceaccess_posix.h"
#import <DiskArbitration/DiskArbitration.h>
#import <Foundation/Foundation.h>
#import <IOKit/IOBSD.h>
#import <IOKit/storage/IOMedia.h>
#include <QCoreApplication>
#include <QFile>
#include <QSet>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <sys/disk.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
QString Text(CFDictionaryRef properties, CFStringRef key) {
    CFTypeRef value = CFDictionaryGetValue(properties, key);
    return value && CFGetTypeID(value) == CFStringGetTypeID()
               ? QString::fromCFString(static_cast<CFStringRef>(value))
               : QString();
}
quint64 Number(CFDictionaryRef properties, CFStringRef key) {
    CFTypeRef value = CFDictionaryGetValue(properties, key);
    qint64 result = 0;
    if (!value || CFGetTypeID(value) != CFNumberGetTypeID() ||
        !CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberSInt64Type, &result) ||
        result < 0)
        return 0;
    return quint64(result);
}
} // namespace

QList<ExternalDevice> EnumerateExternalDevices(QString &error) {
    error.clear();
    QList<ExternalDevice> devices;
    DASessionRef session = DASessionCreate(kCFAllocatorDefault);
    if (!session) {
        error = QCoreApplication::translate("deviceaccess", "Could not inspect attached devices.");
        return devices;
    }
    io_iterator_t iterator = IO_OBJECT_NULL;
    const kern_return_t result = IOServiceGetMatchingServices(
        kIOMainPortDefault, IOServiceMatching(kIOMediaClass), &iterator);
    if (result != KERN_SUCCESS) {
        CFRelease(session);
        error = QCoreApplication::translate("deviceaccess", "Could not inspect attached devices.");
        return devices;
    }
    QSet<QString> mounted;
    io_object_t media;
    while ((media = IOIteratorNext(iterator))) {
        DADiskRef disk = DADiskCreateFromIOMedia(kCFAllocatorDefault, session, media);
        CFDictionaryRef description = disk ? DADiskCopyDescription(disk) : nullptr;
        if (description) {
            CFTypeRef internal =
                CFDictionaryGetValue(description, kDADiskDescriptionDeviceInternalKey);
            CFTypeRef whole = CFDictionaryGetValue(description, kDADiskDescriptionMediaWholeKey);
            const QString name = Text(description, kDADiskDescriptionMediaBSDNameKey);
            if (CFDictionaryContainsKey(description, kDADiskDescriptionVolumePathKey)) {
                DADiskRef parent = DADiskCopyWholeDisk(disk);
                if (parent) {
                    const char *bsdName = DADiskGetBSDName(parent);
                    if (bsdName)
                        mounted.insert(QString::fromUtf8(bsdName));
                    CFRelease(parent);
                }
            }
            if (internal == kCFBooleanFalse && whole == kCFBooleanTrue && !name.isEmpty()) {
                uint64_t identity = 0;
                const quint64 size = Number(description, kDADiskDescriptionMediaSizeKey);
                const quint64 blockSize = Number(description, kDADiskDescriptionMediaBlockSizeKey);
                if (IORegistryEntryGetRegistryEntryID(media, &identity) == KERN_SUCCESS && size &&
                    blockSize && blockSize <= std::numeric_limits<quint32>::max()) {
                    ExternalDevice device;
                    device.id = name;
                    device.identity = QString::number(identity);
                    device.name = Text(description, kDADiskDescriptionDeviceModelKey).trimmed();
                    if (device.name.isEmpty())
                        device.name = Text(description, kDADiskDescriptionMediaNameKey);
                    if (device.name.isEmpty())
                        device.name = name;
                    device.size = size;
                    device.sectorSize = quint32(blockSize);
                    devices.append(device);
                }
            }
            CFRelease(description);
        }
        if (disk)
            CFRelease(disk);
        IOObjectRelease(media);
    }
    IOObjectRelease(iterator);
    CFRelease(session);
    for (auto &device : devices)
        device.mounted = mounted.contains(device.id);
    return devices;
}

int MacOpenExternalDeviceDescriptor(const ExternalDevice &device, QString &error) {
    const auto current = EnumerateExternalDevices(error);
    const auto found = std::find_if(current.cbegin(), current.cend(), [&](const auto &entry) {
        return entry.id == device.id && entry.identity == device.identity &&
               entry.size == device.size && entry.sectorSize == device.sectorSize;
    });
    if (found == current.cend()) {
        error = QCoreApplication::translate("deviceaccess",
                                            "The selected device is no longer available.");
        errno = ENODEV;
        return -1;
    }
    if (found->mounted) {
        error = QCoreApplication::translate(
            "deviceaccess",
            "The device is in use. Unmount its volumes before reading it directly.");
        errno = EBUSY;
        return -1;
    }
    const QByteArray path = QFile::encodeName(QStringLiteral("/dev/") + found->id);
    const int descriptor = ::open(path.constData(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (descriptor < 0) {
        error = QString::fromLocal8Bit(std::strerror(errno));
        return -1;
    }
    struct stat info{};
    struct stat named{};
    uint64_t count = 0;
    uint32_t blockSize = 0;
    const bool valid = ::fstat(descriptor, &info) == 0 && S_ISBLK(info.st_mode) &&
                       ::stat(path.constData(), &named) == 0 && info.st_rdev == named.st_rdev &&
                       ::ioctl(descriptor, DKIOCGETBLOCKCOUNT, &count) == 0 &&
                       ::ioctl(descriptor, DKIOCGETBLOCKSIZE, &blockSize) == 0 &&
                       blockSize == device.sectorSize && count == device.size / blockSize &&
                       device.size % blockSize == 0;
    // The registry identity must still name this device after opening. A reused
    // BSD name after unplug/replug never authorizes a different medium.
    QString recheckError;
    const auto after = valid ? EnumerateExternalDevices(recheckError) : QList<ExternalDevice>();
    const bool same = std::any_of(after.cbegin(), after.cend(), [&](const auto &entry) {
        return entry.id == device.id && entry.identity == device.identity && !entry.mounted;
    });
    if (!valid || !same) {
        ::close(descriptor);
        error = QCoreApplication::translate("deviceaccess",
                                            "The device changed or became unavailable.");
        errno = ENODEV;
        return -1;
    }
    return descriptor;
}

DeviceAccessResult OpenExternalDevice(const ExternalDevice &device, bool authorize,
                                      const std::function<bool()> &cancelled) {
    DeviceAccessResult result;
    if (cancelled && cancelled()) {
        result.status = DeviceAccessStatus::Cancelled;
        return result;
    }
    const int descriptor = MacOpenExternalDeviceDescriptor(device, result.error);
    if (descriptor >= 0) {
        result.handle = AdoptReadOnlyDeviceDescriptor(descriptor);
        result.status = result.handle ? DeviceAccessStatus::Ready : DeviceAccessStatus::Failed;
        if (cancelled && cancelled()) {
            result.handle.reset();
            result.status = DeviceAccessStatus::Cancelled;
        }
        return result;
    }
    if (errno == EBUSY)
        result.status = DeviceAccessStatus::Busy;
    else if (errno == EACCES || errno == EPERM) {
        if (authorize)
            return MacAuthorizeExternalDevice(device, cancelled);
        result.status = DeviceAccessStatus::PermissionRequired;
    }
    return result;
}

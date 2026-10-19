#include "deviceaccess.h"
#include "deviceaccess_posix.h"
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCall>
#include <QDBusUnixFileDescriptor>
#include <QDir>
#include <QFile>
#include <QThread>
#include <algorithm>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

namespace {
using Interfaces = QMap<QString, QVariantMap>;
using Objects = QMap<QDBusObjectPath, Interfaces>;
const QString kService = QStringLiteral("org.freedesktop.UDisks2");
const QString kBlock = kService + QStringLiteral(".Block");
const QString kDrive = kService + QStringLiteral(".Drive");
const QString kFilesystem = kService + QStringLiteral(".Filesystem");

struct Candidate {
    ExternalDevice Device;
    quint64 Number = 0;
};

QString KernelPath(quint64 number) {
    return QStringLiteral("/sys/dev/block/%1:%2")
        .arg(major(dev_t(number)))
        .arg(minor(dev_t(number)));
}

bool InUse(const Objects &objects, const QString &drive) {
    for (const auto &interfaces : objects) {
        const auto block = interfaces.value(kBlock);
        if (qdbus_cast<QDBusObjectPath>(block.value("Drive")).path() != drive)
            continue;
        const auto mounts =
            qdbus_cast<QList<QByteArray>>(interfaces.value(kFilesystem).value("MountPoints"));
        if (!mounts.isEmpty())
            return true;
        const QDir holders(KernelPath(block.value("DeviceNumber").toULongLong()) + "/holders");
        if (!holders.entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty())
            return true;
    }
    return false;
}

QList<Candidate> Snapshot(QString &error) {
    error.clear();
    qDBusRegisterMetaType<Interfaces>();
    qDBusRegisterMetaType<Objects>();
    const auto request = QDBusMessage::createMethodCall(
        kService, QStringLiteral("/org/freedesktop/UDisks2"),
        QStringLiteral("org.freedesktop.DBus.ObjectManager"), QStringLiteral("GetManagedObjects"));
    const auto reply = QDBusConnection::systemBus().call(request, QDBus::Block, 5000);
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().size() != 1) {
        error = QCoreApplication::translate("deviceaccess",
                                            "Could not inspect devices through UDisks2.");
        return {};
    }
    const auto objects = qdbus_cast<Objects>(reply.arguments().front());
    QList<Candidate> result;
    for (auto item = objects.cbegin(); item != objects.cend(); ++item) {
        if (!item->contains(kBlock) || item->contains(kService + ".Partition"))
            continue;
        const auto block = item->value(kBlock);
        const QString drivePath = qdbus_cast<QDBusObjectPath>(block.value("Drive")).path();
        const auto drive = objects.value(QDBusObjectPath(drivePath)).value(kDrive);
        const QString bus = drive.value("ConnectionBus").toString();
        if (!drive.value("MediaAvailable").toBool() ||
            !(drive.value("Removable").toBool() || drive.value("MediaRemovable").toBool() ||
              bus == "usb" || bus == "ieee1394" || bus == "thunderbolt"))
            continue;
        Candidate candidate;
        candidate.Number = block.value("DeviceNumber").toULongLong();
        auto &device = candidate.Device;
        QFile sector(KernelPath(candidate.Number) + "/queue/logical_block_size");
        if (!sector.open(QIODevice::ReadOnly))
            continue;
        bool validSector = false;
        device.sectorSize = sector.readAll().trimmed().toUInt(&validSector);
        device.size = block.value("Size").toULongLong();
        if (!validSector || !device.size || device.sectorSize < 512 || device.sectorSize > 65536 ||
            (device.sectorSize & (device.sectorSize - 1)) || device.size % device.sectorSize)
            continue;
        device.id = item.key().path();
        device.identity = QStringLiteral("%1:%2:%3:%4:%5")
                              .arg(block.value("Id").toString(), drive.value("Id").toString())
                              .arg(candidate.Number)
                              .arg(drive.value("TimeDetected").toULongLong())
                              .arg(drive.value("TimeMediaDetected").toULongLong());
        device.name =
            (drive.value("Vendor").toString() + ' ' + drive.value("Model").toString()).trimmed();
        if (device.name.isEmpty())
            device.name = drive.value("Id").toString();
        device.mounted = InUse(objects, drivePath);
        result.append(candidate);
    }
    return result;
}

bool Same(const ExternalDevice &left, const ExternalDevice &right) {
    return left.id == right.id && left.identity == right.identity && left.size == right.size &&
           left.sectorSize == right.sectorSize;
}

DeviceAccessResult Failure(const QDBusMessage &reply) {
    DeviceAccessResult result;
    result.error = reply.errorMessage();
    const QString name = reply.errorName();
    if (name.endsWith("NotAuthorizedDismissed"))
        result.status = DeviceAccessStatus::Cancelled;
    else if (name.contains("NotAuthorized"))
        result.status = DeviceAccessStatus::PermissionRequired;
    else if (name.endsWith("DeviceBusy"))
        result.status = DeviceAccessStatus::Busy;
    if (result.error.isEmpty())
        result.error =
            QCoreApplication::translate("deviceaccess", "Could not open the selected device.");
    return result;
}

bool ValidDescriptor(int descriptor, const Candidate &candidate) {
    struct stat status{};
    quint64 size = 0;
    int sector = 0;
    const int flags = ::fcntl(descriptor, F_GETFL);
    return flags >= 0 && (flags & O_ACCMODE) == O_RDONLY && ::fstat(descriptor, &status) == 0 &&
           S_ISBLK(status.st_mode) && quint64(status.st_rdev) == candidate.Number &&
           ::ioctl(descriptor, BLKGETSIZE64, &size) == 0 &&
           ::ioctl(descriptor, BLKSSZGET, &sector) == 0 && size == candidate.Device.size &&
           sector == int(candidate.Device.sectorSize);
}
} // namespace

QList<ExternalDevice> EnumerateExternalDevices(QString &error) {
    QList<ExternalDevice> result;
    for (const auto &candidate : Snapshot(error))
        result.append(candidate.Device);
    return result;
}

DeviceAccessResult OpenExternalDevice(const ExternalDevice &device, bool authorize,
                                      const std::function<bool()> &cancelled) {
    DeviceAccessResult result;
    const auto stopped = [&] { return cancelled && cancelled(); };
    if (stopped()) {
        result.status = DeviceAccessStatus::Cancelled;
        return result;
    }
    const auto candidates = Snapshot(result.error);
    if (stopped()) {
        result.status = DeviceAccessStatus::Cancelled;
        return result;
    }
    const auto found = std::find_if(candidates.cbegin(), candidates.cend(),
                                    [&](const auto &entry) { return Same(device, entry.Device); });
    if (found == candidates.cend()) {
        if (result.error.isEmpty())
            result.error = QCoreApplication::translate(
                "deviceaccess", "The selected device is no longer available.");
        return result;
    }
    if (found->Device.mounted) {
        result.status = DeviceAccessStatus::Busy;
        result.error = QCoreApplication::translate(
            "deviceaccess",
            "The device is in use. Unmount its volumes before reading it directly.");
        return result;
    }
    auto request = QDBusMessage::createMethodCall(kService, device.id, kBlock, "OpenDevice");
    QVariantMap options;
    options.insert("flags", O_EXCL | O_CLOEXEC);
    options.insert("auth.no_user_interaction", !authorize);
    request << QStringLiteral("r") << options;
    auto pending = QDBusConnection::systemBus().asyncCall(request, 120000);
    while (!pending.isFinished()) {
        if (stopped()) {
            result.status = DeviceAccessStatus::Cancelled;
            return result;
        }
        QThread::msleep(10);
    }
    if (stopped()) {
        result.status = DeviceAccessStatus::Cancelled;
        return result;
    }
    const QDBusMessage reply = pending.reply();
    if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().size() != 1)
        return Failure(reply);
    const auto received = qdbus_cast<QDBusUnixFileDescriptor>(reply.arguments().front());
    if (!received.isValid())
        return Failure(reply);
    const int descriptor = received.fileDescriptor();
    if (!ValidDescriptor(descriptor, *found)) {
        result.error = QCoreApplication::translate("deviceaccess",
                                                   "The device changed or became unavailable.");
        return result;
    }
    const auto after = Snapshot(result.error);
    const bool same = std::any_of(after.cbegin(), after.cend(), [&](const auto &entry) {
        return Same(device, entry.Device) && !entry.Device.mounted;
    });
    if (!same || stopped()) {
        result.status = stopped() ? DeviceAccessStatus::Cancelled : DeviceAccessStatus::Failed;
        if (result.error.isEmpty() && !stopped())
            result.error = QCoreApplication::translate("deviceaccess",
                                                       "The device changed or became unavailable.");
        return result;
    }
    result.handle = AdoptReadOnlyDeviceDescriptor(::fcntl(descriptor, F_DUPFD_CLOEXEC, 0));
    result.status = result.handle ? DeviceAccessStatus::Ready : DeviceAccessStatus::Failed;
    return result;
}

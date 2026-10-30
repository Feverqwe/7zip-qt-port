#include "devices/deviceaccess.h"
#include "support/private_bus.h"
#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusUnixFileDescriptor>
#include <QDBusVirtualObject>
#include <QFile>
#include <QTemporaryFile>
#include <QtTest>
#include <atomic>
#include <fcntl.h>
#include <future>
#include <sys/sysmacros.h>

namespace {
using Interfaces = QMap<QString, QVariantMap>;
using Objects = QMap<QDBusObjectPath, Interfaces>;
const QString kRoot = QStringLiteral("/org/freedesktop/UDisks2");
const QString kBlock = QStringLiteral("org.freedesktop.UDisks2.Block");
const QString kDrive = QStringLiteral("org.freedesktop.UDisks2.Drive");
const QString kFs = QStringLiteral("org.freedesktop.UDisks2.Filesystem");
const QDBusObjectPath kDisk(kRoot + "/block_devices/test_external");
const QDBusObjectPath kDrivePath(kRoot + "/drives/test_external");

class UDisksFixture final : public QDBusVirtualObject {
  public:
    Objects Data;
    QString Mode;
    int Opens = 0;
    QDBusMessage Pending;
    QDBusMessage InventoryPending;
    bool HoldInventory = false;
    QTemporaryFile File;
    QFile ReadOnly;
    QString introspect(const QString &) const override { return {}; }
    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override {
        if (message.member() == "GetManagedObjects" && HoldInventory) {
            InventoryPending = message;
            return true;
        }
        if (message.member() == "GetManagedObjects")
            return connection.send(message.createReply(QVariant::fromValue(Data)));
        if (message.member() != "OpenDevice")
            return false;
        ++Opens;
        const auto arguments = message.arguments();
        const auto options =
            arguments.size() == 2 ? qdbus_cast<QVariantMap>(arguments[1]) : QVariantMap();
        if (arguments.size() != 2 || arguments[0].toString() != "r" ||
            !(options.value("flags").toInt() & O_EXCL) ||
            !options.value("auth.no_user_interaction").toBool())
            return connection.send(message.createErrorReply("test.UnsafeOpen", "Unsafe request"));
        if (Mode == "pending") {
            Pending = message;
            return true;
        }
        if (Mode != "regular-file")
            return connection.send(message.createErrorReply("org.freedesktop.UDisks2.Error." + Mode,
                                                            "Fixture authorization result"));
        return connection.send(
            message.createReply(QVariant::fromValue(QDBusUnixFileDescriptor(ReadOnly.handle()))));
    }
};

template <typename T> bool Await(std::future<T> &pending) {
    QElapsedTimer timer;
    timer.start();
    while (pending.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready &&
           timer.elapsed() < 8000)
        QTest::qWait(10);
    return pending.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
}

quint64 KernelDeviceNumber() {
    // Only kernel metadata is used to supply a realistic sector size. The
    // fixture never opens a /dev node; OpenDevice returns a private regular file.
    for (const auto &name : QDir("/sys/block").entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QFile file("/sys/block/" + name + "/dev");
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const auto parts = file.readAll().trimmed().split(':');
        if (parts.size() == 2)
            return quint64(makedev(parts[0].toUInt(), parts[1].toUInt()));
    }
    return 0;
}
} // namespace

class UDisksDeviceTests final : public QObject {
    Q_OBJECT
    CPrivateBus bus_;
    std::unique_ptr<QDBusConnection> service_;
    UDisksFixture fixture_;
    quint64 number_ = 0;

    std::future<QList<ExternalDevice>> enumerate() {
        return std::async(std::launch::async, [] {
            QString error;
            return EnumerateExternalDevices(error);
        });
    }

  private slots:
    void initTestCase() {
        number_ = KernelDeviceNumber();
        if (!number_)
            QSKIP("Kernel block metadata unavailable; UDisks protocol integration unverified");
        QVERIFY(bus_.Start());
        qputenv("DBUS_SYSTEM_BUS_ADDRESS", bus_.Address.toUtf8());
        qDBusRegisterMetaType<Interfaces>();
        qDBusRegisterMetaType<Objects>();
        qDBusRegisterMetaType<QList<QByteArray>>();
        service_ = std::make_unique<QDBusConnection>(bus_.Connect());
        QVERIFY(service_->registerService("org.freedesktop.UDisks2"));
        QVERIFY(service_->registerVirtualObject(kRoot, &fixture_, QDBusConnection::SubPath));
        QVERIFY(fixture_.File.open());
        QCOMPARE(fixture_.File.write(QByteArray(4096, 'x')), qint64(4096));
        fixture_.File.close();
        fixture_.ReadOnly.setFileName(fixture_.File.fileName());
        QVERIFY(fixture_.ReadOnly.open(QIODevice::ReadOnly));
    }
    void init() {
        fixture_.Data.clear();
        fixture_.Opens = 0;
        fixture_.Mode = "NotAuthorized";
        fixture_.HoldInventory = false;
        fixture_.InventoryPending = QDBusMessage();
        fixture_.Data[kDrivePath][kDrive] = {
            {"MediaAvailable", true},     {"Removable", true},
            {"ConnectionBus", "usb"},     {"Id", "test-drive"},
            {"Vendor", "Fixture"},        {"Model", "USB 空"},
            {"TimeDetected", quint64(1)}, {"TimeMediaDetected", quint64(1)}};
        fixture_.Data[kDisk][kBlock] = {{"Drive", QVariant::fromValue(kDrivePath)},
                                        {"DeviceNumber", number_},
                                        {"Size", quint64(8 * 1024 * 1024)},
                                        {"Id", "test-medium"}};
    }
    void filtersInternalDevicesAndPartitionsAndRefusesMountedDisk() {
        const QDBusObjectPath internalDrive(kRoot + "/drives/internal");
        fixture_.Data[internalDrive][kDrive] = {{"MediaAvailable", true}, {"ConnectionBus", "ata"}};
        fixture_.Data[QDBusObjectPath(kRoot + "/block_devices/internal")][kBlock] = {
            {"Drive", QVariant::fromValue(internalDrive)},
            {"DeviceNumber", number_},
            {"Size", quint64(8 * 1024 * 1024)}};
        const QDBusObjectPath partition(kRoot + "/block_devices/test_partition");
        fixture_.Data[partition][kBlock] = fixture_.Data[kDisk][kBlock];
        fixture_.Data[partition]["org.freedesktop.UDisks2.Partition"] = {};
        fixture_.Data[partition][kFs]["MountPoints"] =
            QVariant::fromValue(QList<QByteArray>{QByteArray("/fixture\0", 9)});
        auto pending = enumerate();
        QVERIFY(Await(pending));
        const auto devices = pending.get();
        QCOMPARE(devices.size(), 1);
        QCOMPARE(devices[0].name, QStringLiteral("Fixture USB 空"));
        QVERIFY(devices[0].mounted);
        auto opened =
            std::async(std::launch::async, [&] { return OpenExternalDevice(devices[0], true); });
        QVERIFY(Await(opened));
        QCOMPARE(opened.get().status, DeviceAccessStatus::Busy);
        QCOMPARE(fixture_.Opens, 0);
    }
    void staleMediumNeverReachesAuthorization() {
        auto pending = enumerate();
        QVERIFY(Await(pending));
        const auto devices = pending.get();
        QCOMPARE(devices.size(), 1);
        fixture_.Data[kDrivePath][kDrive]["TimeMediaDetected"] = quint64(2);
        auto opened =
            std::async(std::launch::async, [&] { return OpenExternalDevice(devices[0], true); });
        QVERIFY(Await(opened));
        QCOMPARE(opened.get().status, DeviceAccessStatus::Failed);
        QCOMPARE(fixture_.Opens, 0);
    }
    void mapsPolicyErrorsAndRejectsNonDeviceDescriptor_data() {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<DeviceAccessStatus>("expected");
        QTest::newRow("denied") << "NotAuthorized" << DeviceAccessStatus::PermissionRequired;
        QTest::newRow("cancelled") << "NotAuthorizedDismissed" << DeviceAccessStatus::Cancelled;
        QTest::newRow("busy") << "DeviceBusy" << DeviceAccessStatus::Busy;
        QTest::newRow("wrong descriptor") << "regular-file" << DeviceAccessStatus::Failed;
    }
    void mapsPolicyErrorsAndRejectsNonDeviceDescriptor() {
        QFETCH(QString, mode);
        QFETCH(DeviceAccessStatus, expected);
        fixture_.Mode = mode;
        auto pending = enumerate();
        QVERIFY(Await(pending));
        const auto devices = pending.get();
        QCOMPARE(devices.size(), 1);
        auto opened =
            std::async(std::launch::async, [&] { return OpenExternalDevice(devices[0], false); });
        QVERIFY(Await(opened));
        const auto result = opened.get();
        QCOMPARE(result.status, expected);
        QVERIFY(!result.handle);
        QCOMPARE(fixture_.Opens, 1);
    }
    void cancellationDuringPreflightDoesNotStartAuthorization() {
        auto pending = enumerate();
        QVERIFY(Await(pending));
        const auto devices = pending.get();
        QCOMPARE(devices.size(), 1);
        fixture_.HoldInventory = true;
        std::atomic<bool> cancelled{false};
        auto opened = std::async(std::launch::async, [&] {
            return OpenExternalDevice(devices[0], false, [&] { return cancelled.load(); });
        });
        QTRY_VERIFY(fixture_.InventoryPending.type() != QDBusMessage::InvalidMessage);
        cancelled.store(true);
        fixture_.HoldInventory = false;
        QVERIFY(service_->send(
            fixture_.InventoryPending.createReply(QVariant::fromValue(fixture_.Data))));
        QVERIFY(Await(opened));
        QCOMPARE(opened.get().status, DeviceAccessStatus::Cancelled);
        QTest::qWait(20);
        QCOMPARE(fixture_.Opens, 0);
    }
    void cancellationDiscardsPendingAuthorization() {
        fixture_.Mode = "pending";
        auto pending = enumerate();
        QVERIFY(Await(pending));
        const auto devices = pending.get();
        QCOMPARE(devices.size(), 1);
        std::atomic<bool> cancelled{false};
        auto opened = std::async(std::launch::async, [&] {
            return OpenExternalDevice(devices[0], false, [&] { return cancelled.load(); });
        });
        QTRY_COMPARE(fixture_.Opens, 1);
        cancelled.store(true);
        QVERIFY(Await(opened));
        QCOMPARE(opened.get().status, DeviceAccessStatus::Cancelled);
        QVERIFY(service_->send(fixture_.Pending.createErrorReply(
            "org.freedesktop.UDisks2.Error.NotAuthorizedDismissed", "Late cancellation")));
    }
};

QTEST_GUILESS_MAIN(UDisksDeviceTests)
#include "udisks_device_tests.moc"

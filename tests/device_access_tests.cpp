#include "deviceaccess.h"
#include "deviceaccess_posix.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

class DeviceAccessTests final : public QObject {
    Q_OBJECT

  private slots:
    void nativeReadOnlyDescriptorReadsAndCloses() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("данные 空 с пробелом"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        const QByteArray bytes("native device stream bytes");
        QCOMPARE(file.write(bytes), qint64(bytes.size()));
        file.close();
        const int descriptor = ::open(QFile::encodeName(path).constData(), O_RDONLY);
        QVERIFY(descriptor >= 0);
        auto handle = AdoptReadOnlyDeviceDescriptor(descriptor);
        QVERIFY(handle);
        char data[9];
        quint32 read = 0;
        QCOMPARE(handle->readAt(7, data, sizeof(data), read), qint32(0));
        QCOMPARE(QByteArray(data, read), bytes.mid(7, sizeof(data)));
        QVERIFY((::fcntl(descriptor, F_GETFD) & FD_CLOEXEC) != 0);
        handle.reset();
        QCOMPARE(::fcntl(descriptor, F_GETFD), -1);
        QCOMPARE(errno, EBADF);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), bytes);
    }
    void refusesWritableDescriptorAndReleasesIt() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QByteArray path = QFile::encodeName(directory.filePath("writable"));
        const int descriptor = ::open(path.constData(), O_CREAT | O_RDWR, 0600);
        QVERIFY(descriptor >= 0);
        QVERIFY(!AdoptReadOnlyDeviceDescriptor(descriptor));
        QCOMPARE(::fcntl(descriptor, F_GETFD), -1);
        QCOMPARE(errno, EBADF);
    }
    void staleOrForgedSelectionNeverRequestsElevation() {
        ExternalDevice device;
        device.id = QStringLiteral("/not-a-device/../../etc/passwd");
        device.identity = QStringLiteral("not-an-attached-medium");
        device.size = 4096;
        device.sectorSize = 512;
        for (bool authorize : {false, true}) {
            const auto result = OpenExternalDevice(device, authorize);
            QCOMPARE(result.status, DeviceAccessStatus::Failed);
            QVERIFY(!result.handle);
            QVERIFY(!result.error.isEmpty());
        }
        const auto cancelled = OpenExternalDevice(device, true, [] { return true; });
        QCOMPARE(cancelled.status, DeviceAccessStatus::Cancelled);
        QVERIFY(!cancelled.handle);
    }
};

QTEST_GUILESS_MAIN(DeviceAccessTests)
#include "device_access_tests.moc"

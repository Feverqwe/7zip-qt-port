#include "Common/Common0.h"
#include "Windows/Handle.h"
#include "devices/deviceaccess_win.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>
#include <future>

class DeviceAccessWinTests final : public QObject {
    Q_OBJECT

  private slots:
    void nativeReadAtIsIndependentAndReleasesFile() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("данные 空 с пробелом.bin"));
        QByteArray bytes(16384, Qt::Uninitialized);
        for (qsizetype i = 0; i < bytes.size(); ++i)
            bytes[i] = char(i % 251);
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(bytes), qint64(bytes.size()));
        file.close();
        HANDLE raw = CreateFileW(path.toStdWString().c_str(), GENERIC_READ, FILE_SHARE_READ,
                                 nullptr, OPEN_EXISTING, 0, nullptr);
        QVERIFY(raw != INVALID_HANDLE_VALUE);
        auto reader = AdoptWindowsReadHandle(raw);
        QVERIFY(reader);
        auto check = [&](quint64 offset) {
            for (unsigned i = 0; i < 100; ++i) {
                char output[37];
                quint32 count = 0;
                if (reader->readAt(offset, output, sizeof(output), count) != 0 ||
                    QByteArray(output, count) != bytes.mid(qsizetype(offset), sizeof(output)))
                    return false;
            }
            return true;
        };
        auto first = std::async(std::launch::async, check, 11);
        auto second = std::async(std::launch::async, check, 1234);
        QVERIFY(first.get());
        QVERIFY(second.get());
        QVERIFY(!QFile::rename(path, directory.filePath("renamed")));
        reader.reset();
        QVERIFY(QFile::rename(path, directory.filePath("renamed")));
    }
    void writeOnlyHandleCannotBecomeReader() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("write-only");
        HANDLE raw = CreateFileW(path.toStdWString().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                 0, nullptr);
        QVERIFY(raw != INVALID_HANDLE_VALUE);
        QVERIFY(!AdoptWindowsReadHandle(raw));
        QVERIFY(QFile::remove(path));
    }
    void staleSelectionAndCancellationNeverElevate() {
        ExternalDevice device;
        device.id = QStringLiteral("not-a-device/../../private-file");
        device.identity = "not-an-attached-medium";
        device.size = 4096;
        device.sectorSize = 512;
        for (bool authorize : {false, true}) {
            const auto result = OpenExternalDevice(device, authorize);
            QCOMPARE(result.status, DeviceAccessStatus::Failed);
            QVERIFY(!result.handle);
        }
        const auto result = OpenExternalDevice(device, true, [] { return true; });
        QCOMPARE(result.status, DeviceAccessStatus::Cancelled);
        QVERIFY(!result.handle);
    }
};

QTEST_GUILESS_MAIN(DeviceAccessWinTests)
#include "device_access_win_tests.moc"

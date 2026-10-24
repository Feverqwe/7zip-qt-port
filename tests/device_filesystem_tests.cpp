#include "../third_party/7zip/C/CpuArch.h"
#include "7zip/PropID.h"
#include "Windows/PropVariant.h"
#include "archivecodecprobe.h"
#include "archivedevice.h"
#include "archivenativeopen.h"
#include "device_test_support.h"
#include "exfat_fixture.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>

class DeviceFilesystemTests final : public QObject {
    Q_OBJECT
    std::shared_ptr<const ArchiveCodecCatalog> catalog_;
    QString engine_;

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable");
#else
        const QString module = QStringLiteral(SEVENZIP_CODEC_MODULE);
        engine_ = QFileInfo(module).dir().filePath(
#ifdef Q_OS_WIN
            "7z.exe"
#else
            "7z"
#endif
        );
        ArchiveCodecProbe probe;
        bool done = false;
        QVERIFY(probe.Start(module, [&](auto result) {
            catalog_ = result.catalog;
            done = true;
        }));
        QTRY_VERIFY(done);
        QVERIFY(catalog_);
        QVERIFY2(catalog_->result() == S_OK, qPrintable(catalog_->error()));
#endif
    }
    void explainsUnreadableExfatRoot() {
        ExfatFixture fixture;
        fixture.recoveryFault("unreadable-root");
        fixture.image.resize(fixture.image.size() / 512 * 512);
        CMyComPtr<IInStream> raw = new CDeviceInput(fixture.image), direct;
        QCOMPARE(DeviceInput(raw, quint64(fixture.image.size()), &direct, 512), S_OK);
        COpenUi ui;
        QString error;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QVERIFY(OpenDeviceFilesystem(catalog_, direct, "USB 空", &ui, reader, &error) != S_OK);
        QVERIFY(!reader);
        QVERIFY2(error.contains("root directory"), qPrintable(error));
        QVERIFY2(error.contains("No readable entries"), qPrintable(error));
    }
    void readsFilesystemThroughDevice_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<QString>("format");
        QTest::addColumn<QString>("layout");
        for (const auto &entry :
             {qMakePair("apfs", "APFS"), qMakePair("fat12", "FAT"), qMakePair("fat16", "FAT"),
              qMakePair("fat32", "FAT"), qMakePair("exfat", "exFAT"), qMakePair("texfat", "exFAT"),
              qMakePair("exfat-dirty", "exFAT"), qMakePair("ntfs-dirty", "NTFS"),
              qMakePair("fat-dirty", "FAT"), qMakePair("ntfs", "NTFS"), qMakePair("hfsplus", "HFS"),
              qMakePair("udf", "Udf"), qMakePair("iso", "Iso"), qMakePair("squashfs", "SquashFS"),
              qMakePair("erofs", "EROFS")})
            for (const QString &layout : {"volume", "mbr", "apm"})
                QTest::newRow(qPrintable(QString::fromLatin1(entry.first) + '-' + layout))
                    << QString::fromLatin1(entry.first) << QString::fromLatin1(entry.second)
                    << layout;
    }
    void readsFilesystemThroughDevice() {
        QFETCH(QString, kind);
        QFETCH(QString, format);
        QFETCH(QString, layout);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QByteArray disk, expected;
        QString path;
        if (kind.startsWith("exfat") || kind == "texfat") {
            const ExfatFixture fixture = kind == "texfat" ? TexfatFixture(1) : ExfatFixture();
            disk = fixture.image;
            if (kind.endsWith("-dirty"))
                disk[106] = char(2);
            path = fixture.fragmentedName;
            expected = QByteArray(512, 'A') + QByteArray(188, 'B');
        } else {
            QProcess generator;
            const QString filename = temporary.filePath("filesystem.img");
            generator.start(QStringLiteral(DEVICE_PYTHON),
                            {QStringLiteral(DEVICE_GENERATOR), kind, filename});
            QVERIFY(generator.waitForFinished(10000));
            QVERIFY2(generator.exitCode() == 0, generator.readAllStandardError().constData());
            const auto record = QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
            path = record["path"].toString();
            expected = QByteArray::fromBase64(record["bytes"].toString().toLatin1());
            QFile file(filename);
            QVERIFY(file.open(QIODevice::ReadOnly));
            disk = file.readAll();
        }
        QVERIFY(!disk.isEmpty());
        QVERIFY(!path.isEmpty());
        // A 4Kn handle must also serve sub-sector filesystem reads.
        disk.append(QByteArray((4096 - disk.size() % 4096) % 4096, '\0'));
        const UInt64 offset = layout == "volume" ? 0 : 1024 * 1024;
        const UInt64 volumeSize = UInt64(disk.size());
        if (offset) {
            disk.prepend(QByteArray(qsizetype(offset), '\0'));
        }
        if (layout == "mbr") {
            auto *entry = reinterpret_cast<Byte *>(disk.data()) + 446;
            entry[4] = 0x83;
            SetUi32(entry + 8, UInt32(offset / 4096));
            SetUi32(entry + 12, UInt32(volumeSize / 4096));
            disk[510] = 0x55;
            disk[511] = char(0xaa);
        } else if (layout == "apm") {
            auto *header = reinterpret_cast<Byte *>(disk.data());
            qToBigEndian<quint16>(0x4552, header);
            qToBigEndian<quint16>(4096, header + 2);
            SetBe32(header + 4, UInt32(disk.size() / 4096));
            for (unsigned i = 1; i <= 3; ++i) {
                auto *entry = header + i * 4096;
                qToBigEndian<quint16>(0x504d, entry);
                SetBe32(entry + 4, 3);
                SetBe32(entry + 8, i == 1 ? 1 : i == 2 ? UInt32(offset / 4096) : 0);
                SetBe32(entry + 12, i == 1 ? 3 : i == 2 ? UInt32(volumeSize / 4096) : 0);
                memcpy(entry + 16, i == 1 ? "map" : "data", i == 1 ? 3 : 4);
                const QByteArray type = i == 1   ? "Apple_partition_map"
                                        : i == 2 ? "Apple_HFS"
                                                 : "Apple_Void";
                memcpy(entry + 48, type.constData(), size_t(type.size()));
                SetBe32(entry + 88, 0x33);
            }
        }
        auto *source = new CDeviceInput(disk);
        CMyComPtr<IInStream> raw = source, direct;
        QCOMPARE(DeviceInput(raw, quint64(disk.size()), &direct), S_OK);
        ArchiveDevice device(catalog_);
        COpenUi ui;
        QCOMPARE(device.inspect(direct, UInt64(disk.size()), QStringLiteral("USB диск 空"), &ui),
                 S_OK);
        QCOMPARE(device.partitions().size(), layout == "apm" ? 2 : 1);
        const int partition = layout == "apm" ? 1 : 0;
        QCOMPARE(device.partitions()[partition].offset, offset);
        QCOMPARE(device.partitions()[partition].size, volumeSize);
        std::shared_ptr<ArchiveNativeOpen> reader;
        QString error;
        source->Disconnected = true;
        QVERIFY(device.openPartition(partition, &ui, reader) != S_OK);
        QVERIFY(!reader);
        source->Disconnected = false;
        if (kind.endsWith("-dirty")) {
            QCOMPARE(device.openPartition(partition, &ui, reader, &error),
                     kDeviceReadLimitationsRequired);
            QVERIFY(!reader);
            QVERIFY(!error.isEmpty());
            CMyComPtr<IInStream> selected, limited;
            QCOMPARE(device.partitionInput(partition, &selected), S_OK);
            QCOMPARE(CreateDeviceInputAllowingIncompleteRead(selected, &limited), S_OK);
            QCOMPARE(OpenDeviceFilesystem(catalog_, limited, "USB 空", &ui, reader, &error), S_OK);
            std::shared_ptr<ArchiveNativeOpen> rejected;
            QCOMPARE(device.openPartition(partition, &ui, rejected),
                     kDeviceReadLimitationsRequired);
            QVERIFY(!rejected);
        } else {
            QCOMPARE(device.openPartition(partition, &ui, reader, &error), S_OK);
        }
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(reader->resolvedOpenType(), format);
        UInt32 count = 0, index = 0;
        QCOMPARE(reader->arc().Archive->GetNumberOfItems(&count), S_OK);
        for (; index < count; ++index) {
            NWindows::NCOM::CPropVariant value;
            QCOMPARE(reader->arc().Archive->GetProperty(index, kpidPath, &value), S_OK);
            if (value.vt == VT_BSTR &&
                QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal)) == path)
                break;
        }
        QVERIFY2(index < count, qPrintable(path));
        auto *output = new CExtractUi;
        CMyComPtr<IArchiveExtractCallback> callback = output;
        QCOMPARE(reader->arc().Archive->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(output->ItemResult, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(output->bytes(), expected);
        reader.reset();
        ui.Cancel = true;
        QCOMPARE(device.openPartition(partition, &ui, reader), E_ABORT);
        QVERIFY(!reader);
        QVERIFY(source->FarthestRead <= UInt64(disk.size()));
    }
    void rejectsOrdinaryArchiveAsDeviceFilesystem() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile payload(directory.filePath("payload.txt"));
        QVERIFY(payload.open(QIODevice::WriteOnly));
        QCOMPARE(payload.write("ordinary archive"), qint64(16));
        payload.close();
        const QString filename = directory.filePath("ordinary.zip");
        QProcess producer;
        producer.start(engine_, {"a", "-tzip", filename, payload.fileName()});
        QVERIFY(producer.waitForFinished(10000));
        QCOMPARE(producer.exitCode(), 0);
        QFile file(filename);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QByteArray bytes = file.readAll();
        bytes.append(QByteArray((4096 - bytes.size() % 4096) % 4096, '\0'));
        CMyComPtr<IInStream> raw = new CDeviceInput(bytes), direct;
        QCOMPARE(DeviceInput(raw, quint64(bytes.size()), &direct), S_OK);
        ArchiveDevice device(catalog_);
        QCOMPARE(device.inspect(direct, UInt64(bytes.size()), "USB.zip", nullptr), S_OK);
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(device.openPartition(0, nullptr, reader), S_FALSE);
        QVERIFY(!reader);
    }
};

QTEST_GUILESS_MAIN(DeviceFilesystemTests)
#include "device_filesystem_tests.moc"

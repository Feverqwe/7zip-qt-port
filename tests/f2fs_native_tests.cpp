// SPDX-License-Identifier: MIT
#include "Windows/PropVariant.h"
#include "archivedevice.h"
#include "archivenativeopen.h"
#include "device_test_support.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtEndian>
#include <QtTest>

class F2fsNativeTests final : public QObject {
    Q_OBJECT
  private slots:
    void imageAndDevice() {
        const QString module = QString::fromUtf8(SEVENZIP_F2FS_RUNTIME) +
#ifdef Q_OS_WIN
                               "/7z.dll";
#else
                               "/7z.so";
#endif
        auto catalog = std::make_shared<ArchiveCodecCatalog>(
            module, 0, ArchiveCodecCatalog::Scope::Deployment);
        QVERIFY2(catalog->result() == S_OK, qPrintable(catalog->error()));
        QVERIFY(catalog->formatIndex("F2FS") >= 0);
        QVERIFY(!catalog->canUpdate("F2FS"));
        QFile fixture(QString::fromUtf8(SEVENZIP_F2FS_FIXTURES) + "/modern.f2fs.zlib");
        QVERIFY(fixture.open(QIODevice::ReadOnly));
        QByteArray compressed(4, '\0');
        qToBigEndian(quint32(128 * 1024 * 1024), compressed.data());
        compressed += fixture.readAll();
        const QByteArray bytes = qUncompress(compressed);
        QVERIFY(!bytes.isEmpty());
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath(QString::fromUtf8("Android образ 空.f2fs"));
        QFile image(path);
        QVERIFY(image.open(QIODevice::WriteOnly));
        QCOMPARE(image.write(bytes), qint64(bytes.size()));
        image.close();
        COpenUi ui;
        auto opened = std::make_shared<ArchiveNativeOpen>(catalog);
        QCOMPARE(opened->Open(path, {}, &ui), S_OK);
        QCOMPARE(opened->resolvedOpenType(), QString("F2FS"));
        opened.reset();

        auto *source = new CDeviceInput(bytes);
        CMyComPtr<IInStream> raw = source, direct;
        QCOMPARE(DeviceInput(raw, bytes.size(), &direct, 4096), S_OK);
        ArchiveDevice device(catalog);
        QCOMPARE(device.inspect(direct, bytes.size(), QString::fromUtf8("Flash 空"), &ui), S_OK);
        QCOMPARE(device.partitions().size(), 1);
        QCOMPARE(device.openPartition(0, &ui, opened), S_OK);
        QCOMPARE(opened->resolvedOpenType(), QString("F2FS"));
        UInt32 count = 0, index = 0;
        QCOMPARE(opened->arc().Archive->GetNumberOfItems(&count), S_OK);
        for (; index < count; ++index) {
            NWindows::NCOM::CPropVariant value;
            QCOMPARE(opened->arc().Archive->GetProperty(index, kpidPath, &value), S_OK);
            if (value.vt == VT_BSTR &&
                QString::fromWCharArray(value.bstrVal) == QString::fromUtf8("папка 空/tiny.txt"))
                break;
        }
        QVERIFY(index < count);
        auto *output = new CExtractUi;
        CMyComPtr<IArchiveExtractCallback> callback = output;
        QCOMPARE(opened->arc().Archive->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(output->ItemResult, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(output->bytes(), QByteArray("inline F2FS\n"));
        QVERIFY(source->FarthestRead <= UInt64(bytes.size()));
        QVERIFY(image.open(QIODevice::ReadOnly));
        QCOMPARE(image.readAll(), bytes);
    }
};
QTEST_GUILESS_MAIN(F2fsNativeTests)
#include "f2fs_native_tests.moc"

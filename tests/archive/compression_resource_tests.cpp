#include "7zip/UI/GUI/CompressDialogRes.h"
#include "archive/backend/archivebackend.h"
#include "archive/support/source_creation_fixture.h"
#include "dialogs/compress/createarchivedialog.h"
#include <QLocale>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class CompressionResourceTests final : public QObject {
    Q_OBJECT
  private slots:
    void displayedMemoryIncludesLimitAndRam() {
        const QLocale originalLocale;
        const auto restore = qScopeGuard([&] { QLocale::setDefault(originalLocale); });
        QLocale::setDefault(QLocale::c());
        NCompressDialog::CSystemInfo system;
        system.RamSizeDefined = true;
        system.RamSize = UInt64(8) << 30;
        system.RamSizeReduced = UInt64(4) << 30;
        system.RamUsageAuto = UInt64(3) << 30;
        NCompression::CMemUse limit;
        QCOMPARE(CCompressDialog::PrintMemUsage(IDT_COMPRESS_MEMORY_VALUE, UInt64(32) << 20, limit,
                                                system),
                 QString("32 MB / 3072 MB / 8192 MB"));
        limit.Parse("50%");
        QCOMPARE(CCompressDialog::PrintMemUsage(IDT_COMPRESS_MEMORY_VALUE, UInt64(32) << 20, limit,
                                                system),
                 QString("32 MB / 2048 MB / 8192 MB"));
        QCOMPARE(CCompressDialog::PrintMemUsage(IDT_COMPRESS_MEMORY_DE_VALUE, UInt64(32) << 20,
                                                limit, system),
                 QString("32 MB"));
        system.RamSizeDefined = false;
        QCOMPARE(CCompressDialog::PrintMemUsage(IDT_COMPRESS_MEMORY_VALUE, UInt64(32) << 20, limit,
                                                system),
                 QString("32 MB / 2048 MB"));
        limit = {};
        QCOMPARE(CCompressDialog::PrintMemUsage(IDT_COMPRESS_MEMORY_VALUE, UInt64(32) << 20, limit,
                                                system),
                 QString("32 MB"));
        QCOMPARE(
            CCompressDialog::PrintMemUsage(IDT_COMPRESS_MEMORY_VALUE, UInt64(-1), limit, system),
            QString("?"));
    }
    void algorithmLimits_data() {
        QTest::addColumn<QString>("formatName");
        QTest::addColumn<int>("methodID");
        QTest::addColumn<int>("maximum");
        QTest::newRow("LZMA") << QString("7z") << int(NCompressDialog::kLZMA) << 2;
        QTest::newRow("LZMA2") << QString("7z") << int(NCompressDialog::kLZMA2) << 512;
        QTest::newRow("BZip2") << QString("7z") << int(NCompressDialog::kBZip2) << 64;
        QTest::newRow("PPMd") << QString("7z") << int(NCompressDialog::kPPMd) << 1;
        QTest::newRow("Copy") << QString("7z") << int(NCompressDialog::kCopy) << 1;
        QTest::newRow("ZIP") << QString("zip") << int(NCompressDialog::kDeflate)
                             << (sizeof(size_t) == 8 ? 128 : 32);
    }
    void algorithmLimits() {
        QFETCH(QString, formatName);
        QFETCH(int, methodID);
        QFETCH(int, maximum);
        NCompressDialog::CSystemInfo system;
        system.NumCPUs = 256;
        system.NumHardwareThreads = 256;
        const auto options = NCompressDialog::SetNumThreads2(
            formatName, NCompressDialog::EMethodID(methodID), 5, 1 << 20, UInt32(-1), system);
        QCOMPARE(options.AutoNumThreads, UInt32(qMin(256, maximum)));
        if (maximum == 1) {
            QVERIFY(options.Values.isEmpty());
        } else {
            QCOMPARE(options.Values.first(), 1u);
            QCOMPARE(options.Values.last(), UInt32(maximum));
        }
        system.NumCPUs = 2;
        system.NumHardwareThreads = 4;
        const auto restricted = NCompressDialog::SetNumThreads2(
            formatName, NCompressDialog::EMethodID(methodID), 5, 1 << 20, UInt32(-1), system);
        QCOMPARE(restricted.AutoNumThreads, UInt32(qMin(2, maximum)));
        if (maximum != 1)
            QCOMPARE(restricted.Values.last(), UInt32(qMin(8, maximum)));
    }
    void memoryReducesAutomaticThreads() {
        NCompressDialog::CSystemInfo system;
        system.NumCPUs = 16;
        system.NumHardwareThreads = 16;
        system.RamSizeDefined = true;
        system.RamUsageAuto = 64 << 20;
        auto options = NCompressDialog::SetNumThreads2(
            QStringLiteral("7z"), NCompressDialog::kLZMA2, 5, 1 << 20, UInt32(-1), system);
        QCOMPARE(options.AutoNumThreads, 4u);
        UInt64 decompress;
        QCOMPARE(NCompressDialog::GetMemoryUsage_Threads_Dict_DecompMem(
                     QStringLiteral("7z"), NCompressDialog::kPPMd, 5, 1, 1 << 20, UInt32(-1),
                     decompress),
                 UInt64(3 << 20));
        QCOMPARE(decompress, UInt64(3 << 20));
        QCOMPARE(NCompressDialog::GetMemoryUsage_Threads_Dict_DecompMem(
                     QStringLiteral("zip"), NCompressDialog::kDeflate, 5, 1, 32 << 10, UInt32(-1),
                     decompress),
                 UInt64(4 << 20));
        QCOMPARE(decompress, UInt64(2 << 20));
        QCOMPARE(NCompressDialog::GetMemoryUsage_Threads_Dict_DecompMem(
                     QStringLiteral("7z"), NCompressDialog::kCopy, 0, 1, UInt64(-1), UInt32(-1),
                     decompress),
                 UInt64(1 << 20));
    }
    void dialogUpdatesResourceEstimate() {
        QTemporaryDir dir;
        CreateArchiveDialog dialog(dir.path());
        auto *format = dialog.findChild<QComboBox *>("archiveFormat");
        auto *method = dialog.findChild<QComboBox *>("compressionMethod");
        auto *dictionary = dialog.findChild<QComboBox *>("compressionDictionary");
        auto *threads = dialog.findChild<QComboBox *>("compressionThreads");
        auto *memory = dialog.findChild<QLabel *>("compressionMemoryUsage");
        auto *decompress = dialog.findChild<QLabel *>("compressionDecompressionMemory");
        auto *hardware = dialog.findChild<QLabel *>("compressionHardwareThreads");
        QVERIFY(threads && memory && decompress && hardware);
        QVERIFY(hardware->text().startsWith("/ "));
        QVERIFY(threads->itemText(0).startsWith("*  "));
        threads->setCurrentIndex(threads->findData(2u));
        QCOMPARE(dialog.compressionOptions().NumThreads, 2);
        method->setCurrentText("BZip2");
        QCOMPARE(dialog.compressionOptions().NumThreads, UInt32(-1));
        method->setCurrentText("PPMd");
        QVERIFY(!threads->isEnabled());
        QCOMPARE(dialog.compressionOptions().NumThreads, UInt32(-1));
        QVERIFY(memory->text().startsWith(QStringLiteral("18 MB / ")));
        dictionary->setCurrentIndex(dictionary->findData(QVariant::fromValue(quint64(1 << 20))));
        QVERIFY(memory->text().startsWith(QStringLiteral("3 MB / ")));
        QCOMPARE(decompress->text(), QStringLiteral("3 MB"));
        format->setCurrentText("zip");
        method->setCurrentIndex(method->findData(-1));
        QVERIFY(threads->isEnabled());
        threads->setCurrentIndex(threads->findData(1u));
        QVERIFY(memory->text().startsWith(QStringLiteral("4 MB / ")));
        QCOMPARE(decompress->text(), QStringLiteral("2 MB"));
    }
    void operations_data() {
        QTest::addColumn<QString>("formatName");
        QTest::addColumn<QString>("methodName");
        QTest::addColumn<int>("threads");
        QTest::addColumn<quint64>("dictionary");
        QTest::newRow("LZMA2") << QString("7z") << QString("LZMA2") << 2 << quint64(1 << 20);
        QTest::newRow("PPMd") << QString("7z") << QString("PPMd") << 0 << quint64(1 << 20);
        QTest::newRow("BZip2") << QString("7z") << QString("BZip2") << 2 << quint64(100 << 10);
        QTest::newRow("ZIP-Deflate") << QString("zip") << QString("Deflate") << 2 << quint64(0);
    }
    void operations() {
        QFETCH(QString, formatName);
        QFETCH(QString, methodName);
        QFETCH(int, threads);
        QFETCH(quint64, dictionary);
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("данные с пробелом.txt"));
        QFile file(source);
        const QByteArray contents(8192, 'x');
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(contents), qint64(contents.size()));
        file.close();
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        CompressionOptions options;
        options.Method = methodName.toStdWString().c_str();
        options.NumThreads = threads ? UInt32(threads) : UInt32(-1);
        options.Dict64 = dictionary ? dictionary : UInt64(-1);
        const QString target = dir.filePath("threads." + formatName);
        QCOMPARE(SourceCreationFixture::Create(backend, target, {source}, {}, false, options),
                 S_OK);
        done.clear();
        QProcess process;
        process.start("7z", {"t", "--", target});
        QVERIFY(process.waitForFinished(15000));
        QCOMPARE(process.exitCode(), 0);
        const QString output = dir.filePath("out");
        process.start("7z", {"x", "-o" + output, "--", target});
        QVERIFY(process.waitForFinished(15000));
        QCOMPARE(process.exitCode(), 0);
        QFile restored(QDir(output).filePath(QFileInfo(source).fileName()));
        QVERIFY(restored.open(QIODevice::ReadOnly));
        QCOMPARE(restored.readAll(), contents);
        const QString invalid = dir.filePath("invalid." + formatName);
        options.NumThreads = 999999;
        QVERIFY(SourceCreationFixture::Create(backend, invalid, {source}, {}, false, options) !=
                S_OK);
        QVERIFY(!QFileInfo::exists(invalid));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.resize(8 << 20));
        file.close();
        options.NumThreads = threads ? UInt32(threads) : UInt32(-1);
        QCOMPARE(
            SourceCreationFixture::Create(backend, invalid, {source}, {}, false, options, true),
            E_ABORT);
        QVERIFY(!QFileInfo::exists(invalid));
        QVERIFY(QDir(dir.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
};
QTEST_MAIN(CompressionResourceTests)
#include "compression_resource_tests.moc"

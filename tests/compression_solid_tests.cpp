#include "archivebackend.h"
#include "createarchivedialog.h"
#include <QScrollArea>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class CompressionSolidTests final : public QObject {
    Q_OBJECT
  private slots:
    void upstreamBlockChoices() {
        QTemporaryDir dir;
        CreateArchiveDialog dialog(dir.path());
        auto *block = dialog.findChild<QComboBox *>("compressionSolidBlock");
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        auto *format = dialog.findChild<QComboBox *>("archiveFormat");
        QCOMPARE(block->count(), 20);
        QCOMPARE(block->itemData(0).toUInt(), UInt32(-1));
        QCOMPARE(block->itemData(1).toUInt(), 0u);
        QCOMPARE(block->itemData(2).toUInt(), 20u);
        QCOMPARE(block->itemData(18).toUInt(), 36u);
        QCOMPARE(block->itemData(19).toUInt(), 64u);
        block->setCurrentIndex(block->findData(25u));
        QVERIFY(dialog.compressionOptions().SolidIsSpecified);
        QCOMPARE(dialog.compressionOptions().SolidBlockSize, quint64(32 << 20));
        level->setCurrentIndex(level->findData(9u));
        QCOMPARE(dialog.compressionOptions().SolidBlockSize, quint64(32 << 20));
        block->setCurrentIndex(block->findData(64u));
        QCOMPARE(dialog.compressionOptions().SolidBlockSize, quint64(-1));
        auto *dictionary = dialog.findChild<QComboBox *>("compressionDictionary");
        dictionary->setCurrentIndex(dictionary->findData(QVariant::fromValue(quint64(1 << 20))));
        QCOMPARE(dialog.compressionOptions().SolidBlockSize, quint64(-1));
        dialog.show();
        const QString screenshots = QDir::currentPath() + "/ui-screenshots";
        QVERIFY(QDir().mkpath(screenshots));
        for (UInt32 mode : {UInt32(-1), UInt32(0), UInt32(64)}) {
            block->setCurrentIndex(block->findData(mode));
            for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
                dialog.resize(size);
                QApplication::processEvents();
                dialog.findChild<QScrollArea *>("archiveOptionsScrollArea")
                    ->ensureWidgetVisible(block, 0, 80);
                QApplication::processEvents();
                QVERIFY(dialog.grab().save(
                    screenshots + QString("/solid-mode-%1-%2.png").arg(mode).arg(size.width())));
            }
        }
        block->setCurrentIndex(block->findData(0u));
        QVERIFY(block->isEnabled());
        QVERIFY(dialog.compressionOptions().SolidIsSpecified);
        QCOMPARE(dialog.compressionOptions().SolidBlockSize, quint64(0));
        format->setCurrentText("zip");
        QVERIFY(!block->isEnabled());
        QVERIFY(!dialog.compressionOptions().SolidIsSpecified);
        format->setCurrentText("7z");
        level->setCurrentIndex(level->findData(0u));
        QVERIFY(!block->isEnabled());
        QVERIFY(!dialog.compressionOptions().SolidIsSpecified);
    }
    void automaticBlockCalculation() {
        QTemporaryDir dir;
        CreateArchiveDialog dialog(dir.path());
        auto *block = dialog.findChild<QComboBox *>("compressionSolidBlock");
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        auto *method = dialog.findChild<QComboBox *>("compressionMethod");
        auto *dictionary = dialog.findChild<QComboBox *>("compressionDictionary");
        QCOMPARE(block->itemText(0), QStringLiteral("Автоматически (8 ГиБ)"));
        level->setCurrentIndex(level->findData(9u));
        QCOMPARE(block->itemText(0), QStringLiteral("Автоматически (16 ГиБ)"));
        method->setCurrentText("LZMA");
        QCOMPARE(block->itemText(0), QStringLiteral("Автоматически (4 ГиБ)"));
        dictionary->setCurrentIndex(dictionary->findData(QVariant::fromValue(quint64(1 << 20))));
        QCOMPARE(block->itemText(0), QStringLiteral("Автоматически (128 МиБ)"));
        method->setCurrentText("LZMA2");
        QCOMPARE(block->itemText(0), QStringLiteral("Автоматически (256 МиБ)"));
        method->setCurrentText("PPMd");
        QCOMPARE(block->itemText(0), QStringLiteral("Автоматически (128 МиБ)"));
        method->setCurrentText("BZip2");
        QCOMPARE(block->itemText(0), QStringLiteral("Автоматически (112500 КиБ)"));
        dictionary->setCurrentIndex(1);
        QCOMPARE(block->itemText(0), QStringLiteral("Автоматически (16 МиБ)"));
        QVERIFY(!dialog.compressionOptions().SolidIsSpecified);
    }
    void createsWithBlockModes_data() {
        QTest::addColumn<bool>("specified");
        QTest::addColumn<quint64>("blockSize");
        QTest::addColumn<int>("blocks");
        QTest::newRow("Auto") << false << quint64(0) << 1;
        QTest::newRow("Non-solid") << true << quint64(0) << 2;
        QTest::newRow("1 MiB") << true << quint64(1 << 20) << 2;
        QTest::newRow("Full-solid") << true << quint64(-1) << 1;
    }
    void createsWithBlockModes() {
        QFETCH(bool, specified);
        QFETCH(quint64, blockSize);
        QFETCH(int, blocks);
        QTemporaryDir dir;
        QStringList sources;
        const QByteArray contents((1 << 20) + 1, 'x');
        for (QString name :
             {QStringLiteral("данные с пробелом.txt"), QStringLiteral("второй файл.txt")}) {
            const QString source = dir.filePath(name);
            QFile file(source);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(contents), qint64(contents.size()));
            sources.append(source);
        }
        ArchiveBackend backend;
        CompressionOptions options;
        options.method = "LZMA2";
        options.dictionaryBytes = 1 << 20;
        options.SolidIsSpecified = specified;
        options.SolidBlockSize = blockSize;
        const QString target = dir.filePath("block.7z");
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        backend.create(target, sources, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        QProcess process;
        process.start("7zz", {"l", "-slt", "--", target});
        QVERIFY(process.waitForFinished(15000));
        QCOMPARE(process.exitCode(), 0);
        const QString listing = QString::fromUtf8(process.readAllStandardOutput()).remove('\r');
        QVERIFY2(listing.contains(QString("\nBlocks = %1\n").arg(blocks)), qPrintable(listing));
        process.start("7zz", {"t", "--", target});
        QVERIFY(process.waitForFinished(15000));
        QCOMPARE(process.exitCode(), 0);
        const QString extracted = dir.filePath("out");
        process.start("7zz", {"x", "-o" + extracted, "--", target});
        QVERIFY(process.waitForFinished(15000));
        QCOMPARE(process.exitCode(), 0);
        for (const QString &source : sources) {
            QFile restored(QDir(extracted).filePath(QFileInfo(source).fileName()));
            QVERIFY(restored.open(QIODevice::ReadOnly));
            QCOMPARE(restored.readAll(), contents);
        }
    }
    void rejectsInvalidAndCancels() {
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("источник space.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.resize(8 << 20));
        file.close();
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        CompressionOptions options;
        options.method = "LZMA2";
        options.dictionaryBytes = 1 << 20;
        options.SolidIsSpecified = true;
        options.SolidBlockSize = 3 << 20;
        const QString target = dir.filePath("invalid.7z");
        backend.create(target, {source}, {}, false, options);
        QCOMPARE(done.size(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        QVERIFY(!QFileInfo::exists(target));
        options.SolidBlockSize = quint64(-1);
        options.level = 0;
        options.dictionaryBytes = 0;
        backend.create(target, {source}, {}, false, options);
        QCOMPARE(done.size(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        options.level = 5;
        options.method = "Deflate";
        const QString zip = dir.filePath("invalid.zip");
        backend.create(zip, {source}, {}, false, options);
        QCOMPARE(done.size(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        QVERIFY(!QFileInfo::exists(zip));
        options.method = "LZMA2";
        backend.create(target, {source}, {}, false, options);
        QVERIFY(backend.busy());
        backend.cancel();
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY(!done.first()[0].toBool());
        QVERIFY(done.first()[1].toString().contains("отменена"));
        QVERIFY(!QFileInfo::exists(target));
        QVERIFY(QDir(dir.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
};
QTEST_MAIN(CompressionSolidTests)
#include "compression_solid_tests.moc"

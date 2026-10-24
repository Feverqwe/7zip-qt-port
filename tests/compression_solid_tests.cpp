#include "archivebackend.h"
#include "createarchivedialog.h"
#include "fixtures/archive_sections.h"
#include "layout_inspection.h"
#include "source_creation_fixture.h"
#include <QScrollArea>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QToolButton>
#include <QtTest>

class CompressionSolidTests final : public QObject {
    Q_OBJECT
  private slots:
    void blockSelectionAndReset() {
        QTemporaryDir dir;
        CreateArchiveDialog dialog(dir.path());
        auto *block = dialog.findChild<QComboBox *>("compressionSolidBlock");
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        auto *format = dialog.findChild<QComboBox *>("archiveFormat");
        block->setCurrentIndex(block->findData(25u));
        QVERIFY(dialog.compressionOptions().SolidIsSpecified);
        QCOMPARE(dialog.compressionOptions().SolidBlockSize, quint64(32 << 20));
        level->setCurrentIndex(level->findData(9u));
        QVERIFY(!dialog.compressionOptions().SolidIsSpecified);
        block->setCurrentIndex(block->findData(64u));
        QCOMPARE(dialog.compressionOptions().SolidBlockSize, quint64(-1));
        auto *dictionary = dialog.findChild<QComboBox *>("compressionDictionary");
        dictionary->setCurrentIndex(dictionary->findData(QVariant::fromValue(quint64(1 << 20))));
        QCOMPARE(dialog.compressionOptions().SolidBlockSize, quint64(-1));
        ExpandArchiveSections(dialog);
        dialog.show();
        for (UInt32 mode : {UInt32(-1), UInt32(0), UInt32(64)}) {
            block->setCurrentIndex(block->findData(mode));
            for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
                dialog.resize(size);
                QApplication::processEvents();
                dialog.findChild<QScrollArea *>("archiveOptionsScrollArea")
                    ->ensureWidgetVisible(block, 0, 80);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*block, dialog));
                QVERIFY(UiInspection::writeReport(
                    dialog, QString("solid-mode-%1-%2").arg(mode).arg(size.width())));
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
        QCOMPARE(block->itemText(0), QStringLiteral("*  8 GB"));
        level->setCurrentIndex(level->findData(9u));
        QCOMPARE(block->itemText(0), QStringLiteral("*  16 GB"));
        method->setCurrentText("LZMA");
        QCOMPARE(block->itemText(0), QStringLiteral("*  4 GB"));
        dictionary->setCurrentIndex(dictionary->findData(QVariant::fromValue(quint64(1 << 20))));
        QCOMPARE(block->itemText(0), QStringLiteral("*  128 MB"));
        method->setCurrentIndex(method->findData(-1));
        dictionary->setCurrentIndex(dictionary->findData(QVariant::fromValue(quint64(1 << 20))));
        QCOMPARE(block->itemText(0), QStringLiteral("*  256 MB"));
        method->setCurrentText("PPMd");
        dictionary->setCurrentIndex(dictionary->findData(QVariant::fromValue(quint64(1 << 20))));
        QCOMPARE(block->itemText(0), QStringLiteral("*  128 MB"));
        method->setCurrentText("BZip2");
        QCOMPARE(block->itemText(0), QStringLiteral("*  112500 KB"));
        dictionary->setCurrentIndex(1);
        QCOMPARE(block->itemText(0), QStringLiteral("*  16 MB"));
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
        options.Method = "LZMA2";
        options.Dict64 = 1 << 20;
        options.SolidIsSpecified = specified;
        options.SolidBlockSize = blockSize;
        const QString target = dir.filePath("block.7z");
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        QCOMPARE(SourceCreationFixture::Create(backend, target, sources, {}, false, options), S_OK);
        QProcess process;
        process.start("7z", {"l", "-slt", "--", target});
        QVERIFY(process.waitForFinished(15000));
        QCOMPARE(process.exitCode(), 0);
        const QString listing = QString::fromUtf8(process.readAllStandardOutput()).remove('\r');
        QVERIFY2(listing.contains(QString("\nBlocks = %1\n").arg(blocks)), qPrintable(listing));
        process.start("7z", {"t", "--", target});
        QVERIFY(process.waitForFinished(15000));
        QCOMPARE(process.exitCode(), 0);
        const QString extracted = dir.filePath("out");
        process.start("7z", {"x", "-o" + extracted, "--", target});
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
        options.Method = "LZMA2";
        options.Dict64 = 1 << 20;
        options.SolidIsSpecified = true;
        options.SolidBlockSize = quint64(-1);
        const QString target = dir.filePath("invalid.7z");
        options.Level = 0;
        options.Dict64 = UInt64(-1);
        QVERIFY(SourceCreationFixture::Create(backend, target, {source}, {}, false, options) !=
                S_OK);
        options.Level = 5;
        options.Method = "Deflate";
        const QString zip = dir.filePath("invalid.zip");
        QVERIFY(SourceCreationFixture::Create(backend, zip, {source}, {}, false, options) != S_OK);
        QVERIFY(!QFileInfo::exists(zip));
        options.Method = "LZMA2";
        QCOMPARE(SourceCreationFixture::Create(backend, target, {source}, {}, false, options, true),
                 E_ABORT);
        QVERIFY(!QFileInfo::exists(target));
        QVERIFY(QDir(dir.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
};
QTEST_MAIN(CompressionSolidTests)
#include "compression_solid_tests.moc"

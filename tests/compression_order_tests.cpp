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

class CompressionOrderTests final : public QObject {
    Q_OBJECT
  private slots:
    void upstreamLzmaWordChoices() {
        QTemporaryDir dir;
        CreateArchiveDialog dialog(dir.path());
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        auto *method = dialog.findChild<QComboBox *>("compressionMethod");
        auto *word = dialog.findChild<QComboBox *>("compressionWordSize");
        QVERIFY(word->itemText(0).contains("32"));
        word->setCurrentIndex(word->findData(48));
        QCOMPARE(dialog.compressionOptions().Order, 48);
        level->setCurrentIndex(level->findData(9u));
        QCOMPARE(dialog.compressionOptions().Order, UInt32(-1));
        QVERIFY(word->itemText(0).contains("64"));
        word->setCurrentIndex(0);
        ExpandArchiveSections(dialog);
        dialog.show();
        for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
            dialog.resize(size);
            QApplication::processEvents();
            dialog.findChild<QScrollArea *>("archiveOptionsScrollArea")
                ->ensureWidgetVisible(word, 0, 80);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*word, dialog));
            QVERIFY(UiInspection::writeReport(dialog,
                                              QString("compression-order-%1").arg(size.width())));
        }
        method->setCurrentText("BZip2");
        QVERIFY(!word->isEnabled());
        QCOMPARE(dialog.compressionOptions().Order, UInt32(-1));
        method->setCurrentText("LZMA");
        QVERIFY(word->isEnabled());
    }
    void upstreamDeflateWordChoices_data() {
        QTest::addColumn<QString>("methodName");
        QTest::addColumn<int>("maximum");
        QTest::newRow("Deflate") << QString("Deflate") << 258;
        QTest::newRow("Deflate64") << QString("Deflate64") << 257;
    }
    void upstreamDeflateWordChoices() {
        QFETCH(QString, methodName);
        QFETCH(int, maximum);
        QTemporaryDir dir;
        CreateArchiveDialog dialog(dir.path());
        dialog.findChild<QComboBox *>("archiveFormat")->setCurrentText("zip");
        dialog.findChild<QComboBox *>("compressionMethod")->setCurrentText(methodName);
        auto *word = dialog.findChild<QComboBox *>("compressionWordSize");
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        QVERIFY(word->isEnabled());
        QVERIFY(word->itemText(0).contains("32"));
        word->setCurrentIndex(word->findData(maximum));
        QCOMPARE(dialog.compressionOptions().Order, UInt32(maximum));
        level->setCurrentIndex(level->findData(7u));
        dialog.findChild<QComboBox *>("compressionMethod")->setCurrentText(methodName);
        QVERIFY(word->itemText(0).contains("64"));
        level->setCurrentIndex(level->findData(9u));
        dialog.findChild<QComboBox *>("compressionMethod")->setCurrentText(methodName);
        QVERIFY(word->itemText(0).contains("128"));
        QCOMPARE(dialog.compressionOptions().Order, UInt32(-1));
        ExpandArchiveSections(dialog);
        dialog.show();
        for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
            dialog.resize(size);
            QApplication::processEvents();
            dialog.findChild<QScrollArea *>("archiveOptionsScrollArea")
                ->ensureWidgetVisible(word, 0, 80);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*word, dialog));
            QVERIFY(UiInspection::writeReport(
                dialog, QString("compression-%1-%2").arg(methodName).arg(size.width())));
        }
        level->setCurrentIndex(level->findData(0u));
        QVERIFY(!word->isEnabled());
        QCOMPARE(dialog.compressionOptions().Order, UInt32(-1));
    }
    void upstreamPpmdOrderChoices_data() {
        QTest::addColumn<QString>("formatName");
        QTest::newRow("7z") << QString("7z");
        QTest::newRow("zip") << QString("zip");
    }
    void upstreamPpmdOrderChoices() {
        QFETCH(QString, formatName);
        QTemporaryDir dir;
        CreateArchiveDialog dialog(dir.path());
        dialog.findChild<QComboBox *>("archiveFormat")->setCurrentText(formatName);
        auto *method = dialog.findChild<QComboBox *>("compressionMethod");
        method->setCurrentText("PPMd");
        auto *word = dialog.findChild<QComboBox *>("compressionWordSize");
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        QVERIFY(word->isEnabled());
        word->setCurrentIndex(word->findData(12));
        QCOMPARE(dialog.compressionOptions().Order, 12);
        for (int value : {1, 5, 7, 9}) {
            level->setCurrentIndex(level->findData(unsigned(value)));
            method->setCurrentText("PPMd");
            const int autoOrder = formatName == "zip" ? value + 3
                                  : value == 9        ? 32
                                  : value == 7        ? 16
                                  : value == 5        ? 6
                                                      : 4;
            QCOMPARE(word->itemText(0), QStringLiteral("*  %1").arg(autoOrder));
            QCOMPARE(dialog.compressionOptions().Order, UInt32(-1));
        }
        word->setCurrentIndex(word->count() - 1);
        ExpandArchiveSections(dialog);
        dialog.show();
        for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
            dialog.resize(size);
            QApplication::processEvents();
            dialog.findChild<QScrollArea *>("archiveOptionsScrollArea")
                ->ensureWidgetVisible(word, 0, 80);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*word, dialog));
            QVERIFY(UiInspection::writeReport(
                dialog, QString("compression-PPMd-%1-%2").arg(formatName).arg(size.width())));
        }
        method->setCurrentText("BZip2");
        QVERIFY(!word->isEnabled());
        QCOMPARE(dialog.compressionOptions().Order, UInt32(-1));
    }
    void createsWithPortedWordSizes_data() {
        QTest::addColumn<int>("word");
        QTest::addColumn<QString>("methodName");
        QTest::addColumn<QString>("suffix");
        QTest::newRow("8") << 8 << QString("LZMA2") << QString("7z");
        QTest::newRow("48") << 48 << QString("LZMA2") << QString("7z");
        QTest::newRow("192") << 192 << QString("LZMA2") << QString("7z");
        QTest::newRow("PPMd-7z-min") << 2 << QString("PPMd") << QString("7z");
        QTest::newRow("PPMd-7z-max") << 32 << QString("PPMd") << QString("7z");
        QTest::newRow("PPMd-zip-min") << 2 << QString("PPMd") << QString("zip");
        QTest::newRow("PPMd-zip-max") << 16 << QString("PPMd") << QString("zip");
        QTest::newRow("Deflate-max") << 258 << QString("Deflate") << QString("zip");
        QTest::newRow("Deflate64-max") << 257 << QString("Deflate64") << QString("zip");
    }
    void createsWithPortedWordSizes() {
        QFETCH(int, word);
        QFETCH(QString, methodName);
        QFETCH(QString, suffix);
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("данные с пробелом.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(QByteArray(8192, 'x')), qint64(8192));
        file.close();
        ArchiveBackend backend;
        CompressionOptions options;
        options.Method = methodName.toStdWString().c_str();
        options.Dict64 = methodName == "LZMA2" ? UInt64(1 << 20) : UInt64(-1);
        options.Order = word;
        const QString target = dir.filePath("word." + suffix);
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        QCOMPARE(SourceCreationFixture::Create(backend, target, {source}, {}, false, options),
                 S_OK);
        QProcess test;
        test.start("7z", {"t", "--", target});
        QVERIFY(test.waitForFinished(15000));
        QCOMPARE(test.exitCode(), 0);
    }
    void invalidWordAndCancelPreserveDestination_data() {
        QTest::addColumn<QString>("methodName");
        QTest::addColumn<QString>("suffix");
        QTest::addColumn<int>("invalidWord");
        QTest::addColumn<int>("validWord");
        QTest::newRow("Deflate64") << QString("Deflate64") << QString("zip") << 258 << 48;
        QTest::newRow("PPMd-7z") << QString("PPMd") << QString("7z") << 999999 << 32;
        QTest::newRow("PPMd-zip") << QString("PPMd") << QString("zip") << 17 << 16;
    }
    void invalidWordAndCancelPreserveDestination() {
        QFETCH(QString, methodName);
        QFETCH(QString, suffix);
        QFETCH(int, invalidWord);
        QFETCH(int, validWord);
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("источник space.bin"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.resize(8 << 20));
        file.close();
        ArchiveBackend backend;
        CompressionOptions options;
        options.Method = methodName.toStdWString().c_str();
        options.Order = invalidWord;
        const QString target = dir.filePath("cancel." + suffix);
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        QVERIFY(SourceCreationFixture::Create(backend, target, {source}, {}, false, options) !=
                S_OK);
        QVERIFY(!QFileInfo::exists(target));
        options.Order = validWord;
        QCOMPARE(SourceCreationFixture::Create(backend, target, {source}, {}, false, options, true),
                 E_ABORT);
        QVERIFY(!QFileInfo::exists(target));
        QVERIFY(QDir(dir.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
};
QTEST_MAIN(CompressionOrderTests)
#include "compression_order_tests.moc"

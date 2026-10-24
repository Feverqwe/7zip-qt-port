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

class CompressionDictionaryTests final : public QObject {
    Q_OBJECT
  private slots:
    void choicesAndAutomaticSize() {
        QTemporaryDir dir;
        CreateArchiveDialog dialog(dir.path());
        auto *dictionary = dialog.findChild<QComboBox *>("compressionDictionary");
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        auto *method = dialog.findChild<QComboBox *>("compressionMethod");
        QVERIFY(dictionary->itemText(0).contains("32 MB"));
        dictionary->setCurrentIndex(dictionary->findData(QVariant::fromValue(quint64(3 << 20))));
        QCOMPARE(dialog.compressionOptions().Dict64, quint64(3 << 20));
        level->setCurrentIndex(level->findData(9u));
        QCOMPARE(dialog.compressionOptions().Dict64, UInt64(-1));
        QVERIFY(dictionary->itemText(0).contains(sizeof(size_t) == 8 ? "256 MB" : "64 MB"));
        level->setCurrentIndex(level->findData(1u));
        QVERIFY(dictionary->itemText(0).contains("256 KB"));
        method->setCurrentText("LZMA");
        QCOMPARE(dialog.compressionOptions().Dict64, UInt64(-1));
        ExpandArchiveSections(dialog);
        dialog.show();
        for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
            dialog.resize(size);
            QApplication::processEvents();
            dialog.findChild<QScrollArea *>("archiveOptionsScrollArea")
                ->ensureWidgetVisible(dictionary, 0, 80);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*dictionary, dialog));
            QVERIFY(UiInspection::writeReport(
                dialog, QString("compression-dictionary-%1").arg(size.width())));
        }
        level->setCurrentIndex(level->findData(0u));
        QVERIFY(!dictionary->isEnabled());
        QCOMPARE(dialog.compressionOptions().Dict64, UInt64(-1));
    }
    void createsAndChecksArchive_data() {
        QTest::addColumn<quint64>("size");
        QTest::addColumn<QString>("methodName");
        QTest::addColumn<QString>("suffix");
        QTest::newRow("64 KiB") << quint64(64 << 10) << QString("LZMA2") << QString("7z");
        QTest::newRow("256 KiB") << quint64(256 << 10) << QString("LZMA") << QString("zip");
        QTest::newRow("3 MiB") << quint64(3 << 20) << QString("LZMA") << QString("7z");
    }
    void createsAndChecksArchive() {
        QFETCH(quint64, size);
        QFETCH(QString, methodName);
        QFETCH(QString, suffix);
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("данные space.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(QByteArray(8192, 'x')), qint64(8192));
        file.close();
        CompressionOptions options;
        options.Method = methodName.toStdWString().c_str();
        options.Dict64 = size;
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        const QString target = dir.filePath("dictionary." + suffix);
        QCOMPARE(SourceCreationFixture::Create(backend, target, {source}, {}, false, options),
                 S_OK);
        QProcess test;
        test.start("7z", {"t", "--", target});
        QVERIFY(test.waitForFinished(15000));
        QCOMPARE(test.exitCode(), 0);
    }
};
QTEST_MAIN(CompressionDictionaryTests)
#include "compression_dictionary_tests.moc"

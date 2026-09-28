#include "archivebackend.h"
#include "createarchivedialog.h"
#include <QScrollArea>
#include <QSignalSpy>
#include <QTemporaryDir>
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
        QCOMPARE(dictionary->itemData(1).toULongLong(), quint64(64 << 10));
        QCOMPARE(dictionary->itemData(2).toULongLong(), quint64(256 << 10));
        QCOMPARE(dictionary->itemData(3).toULongLong(), quint64(1 << 20));
        QCOMPARE(dictionary->itemData(4).toULongLong(), quint64(2 << 20));
        QCOMPARE(dictionary->itemData(5).toULongLong(), quint64(3 << 20));
        QCOMPARE(dictionary->itemData(dictionary->count() - 1).toULongLong(),
                 sizeof(size_t) == 8 ? quint64(3840) << 20 : quint64(64) << 20);
        QCOMPARE(dictionary->count(), sizeof(size_t) == 8 ? 27 : 15);
        QList<quint64> expected = {0, 64 << 10, 256 << 10};
        for (int mib : {1,  2,   3,   4,   6,   8,   12,  16,   24,   32,   48,   64,
                        96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 3840}) {
            if (sizeof(size_t) == 4 && mib > 64)
                break;
            expected.append(quint64(mib) << 20);
        }
        QList<quint64> actual;
        for (int i = 0; i < dictionary->count(); ++i)
            actual.append(dictionary->itemData(i).toULongLong());
        QCOMPARE(actual, expected);
        QVERIFY(dictionary->itemText(0).contains("32 МиБ"));
        dictionary->setCurrentIndex(dictionary->findData(QVariant::fromValue(quint64(3 << 20))));
        level->setCurrentIndex(level->findData(9u));
        QCOMPARE(dialog.compressionOptions().dictionaryBytes, quint64(0));
        QVERIFY(dictionary->itemText(0).contains(sizeof(size_t) == 8 ? "256 МиБ" : "64 МиБ"));
        level->setCurrentIndex(level->findData(1u));
        QVERIFY(dictionary->itemText(0).contains("256 КиБ"));
        method->setCurrentText("LZMA");
        QCOMPARE(dialog.compressionOptions().dictionaryBytes, quint64(0));
        dialog.show();
        const QString screenshots = QDir::currentPath() + "/ui-screenshots";
        QVERIFY(QDir().mkpath(screenshots));
        for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
            dialog.resize(size);
            QApplication::processEvents();
            dialog.findChild<QScrollArea *>("archiveOptionsScrollArea")
                ->ensureWidgetVisible(dictionary, 0, 80);
            QApplication::processEvents();
            QVERIFY(dialog.grab().save(
                screenshots + QString("/compression-dictionary-%1.png").arg(size.width())));
        }
        level->setCurrentIndex(level->findData(0u));
        QVERIFY(!dictionary->isEnabled());
        QCOMPARE(dialog.compressionOptions().dictionaryBytes, quint64(0));
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
        options.method = methodName;
        options.dictionaryBytes = size;
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        const QString target = dir.filePath("dictionary." + suffix);
        backend.create(target, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        QProcess test;
        test.start("7zz", {"t", "--", target});
        QVERIFY(test.waitForFinished(15000));
        QCOMPARE(test.exitCode(), 0);
    }
    void invalidAndCancelledLeaveNoOutput() {
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("источник space.bin"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.resize(8 << 20));
        file.close();
        CompressionOptions options;
        options.method = "LZMA2";
        options.dictionaryBytes = 128 << 10;
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        const QString target = dir.filePath("invalid.7z");
        backend.create(target, {source}, {}, false, options);
        QCOMPARE(done.size(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        QVERIFY(!QFileInfo::exists(target));
        options.dictionaryBytes = 3 << 20;
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
QTEST_MAIN(CompressionDictionaryTests)
#include "compression_dictionary_tests.moc"

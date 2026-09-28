#include "archivebackend.h"
#include "createarchivedialog.h"
#include <QScrollArea>
#include <QSignalSpy>
#include <QTemporaryDir>
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
        const QList<int> expected = {0, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 273};
        QList<int> actual;
        for (int i = 0; i < word->count(); ++i)
            actual << word->itemData(i).toInt();
        QCOMPARE(actual, expected);
        QVERIFY(word->itemText(0).contains("32"));
        word->setCurrentIndex(word->findData(48));
        QCOMPARE(dialog.compressionOptions().wordSize, 48);
        level->setCurrentIndex(level->findData(9u));
        QCOMPARE(dialog.compressionOptions().wordSize, 48);
        QVERIFY(word->itemText(0).contains("64"));
        word->setCurrentIndex(0);
        dialog.show();
        const QString screenshots = QDir::currentPath() + "/ui-screenshots";
        QVERIFY(QDir().mkpath(screenshots));
        for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
            dialog.resize(size);
            QApplication::processEvents();
            dialog.findChild<QScrollArea *>("archiveOptionsScrollArea")
                ->ensureWidgetVisible(word, 0, 80);
            QApplication::processEvents();
            QVERIFY(dialog.grab().save(
                screenshots + QStringLiteral("/compression-order-%1.png").arg(size.width())));
        }
        method->setCurrentText("PPMd");
        QVERIFY(!word->isEnabled());
        QCOMPARE(dialog.compressionOptions().wordSize, 0);
        method->setCurrentText("LZMA");
        QVERIFY(word->isEnabled());
        QCOMPARE(word->count(), expected.size());
    }
    void createsWithPortedWordSizes_data() {
        QTest::addColumn<int>("word");
        QTest::newRow("8") << 8;
        QTest::newRow("48") << 48;
        QTest::newRow("192") << 192;
    }
    void createsWithPortedWordSizes() {
        QFETCH(int, word);
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("данные с пробелом.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(QByteArray(8192, 'x')), qint64(8192));
        file.close();
        ArchiveBackend backend;
        CompressionOptions options;
        options.method = "LZMA2";
        options.dictionaryMiB = 1;
        options.wordSize = word;
        const QString target = dir.filePath("word.7z");
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        backend.create(target, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.first()[0].toBool(), qPrintable(done.first()[1].toString()));
        QProcess test;
        test.start("7zz", {"t", "--", target});
        QVERIFY(test.waitForFinished(15000));
        QCOMPARE(test.exitCode(), 0);
    }
    void invalidWordAndCancelPreserveDestination() {
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("источник space.bin"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.resize(8 << 20));
        file.close();
        ArchiveBackend backend;
        CompressionOptions options;
        options.method = "LZMA2";
        options.wordSize = 11;
        const QString target = dir.filePath("cancel.7z");
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        backend.create(target, {source}, {}, false, options);
        QCOMPARE(done.size(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        QVERIFY(!QFileInfo::exists(target));
        options.wordSize = 48;
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
QTEST_MAIN(CompressionOrderTests)
#include "compression_order_tests.moc"

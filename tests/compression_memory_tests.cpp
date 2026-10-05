#include "archivebackend.h"
#include "createarchivedialog.h"
#include "layout_inspection.h"
#include <QScrollArea>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class CompressionMemoryTests final : public QObject {
    Q_OBJECT
  private slots:
    void choices_data() {
        QTest::addColumn<QString>("formatName");
        QTest::addColumn<QString>("methodName");
        QTest::addColumn<QList<quint64>>("expected");
        QList<quint64> ppmd = {0};
        for (int mib :
             {1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024}) {
            if (sizeof(size_t) == 4 && mib > 512)
                break;
            ppmd.append(quint64(mib) << 20);
        }
        QTest::newRow("PPMd-7z") << QString("7z") << QString("PPMd") << ppmd;
        QTest::newRow("PPMd-zip") << QString("zip") << QString("PPMd")
                                  << QList<quint64>{0,         1 << 20,  2 << 20,  4 << 20,
                                                    8 << 20,   16 << 20, 32 << 20, 64 << 20,
                                                    128 << 20, 256 << 20};
        const QList<quint64> bzip = {0,         100 << 10, 200 << 10, 300 << 10, 400 << 10,
                                     500 << 10, 600 << 10, 700 << 10, 800 << 10, 900 << 10};
        QTest::newRow("BZip2-7z") << QString("7z") << QString("BZip2") << bzip;
        QTest::newRow("BZip2-zip") << QString("zip") << QString("BZip2") << bzip;
    }
    void choices() {
        QFETCH(QString, formatName);
        QFETCH(QString, methodName);
        QFETCH(QList<quint64>, expected);
        QTemporaryDir dir;
        CreateArchiveDialog dialog(dir.path());
        dialog.findChild<QComboBox *>("archiveFormat")->setCurrentText(formatName);
        dialog.findChild<QComboBox *>("compressionMethod")->setCurrentText(methodName);
        auto *dictionary = dialog.findChild<QComboBox *>("compressionDictionary");
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        auto *method = dialog.findChild<QComboBox *>("compressionMethod");
        QVERIFY(dictionary->isEnabled());
        QList<quint64> actual;
        for (int i = 0; i < dictionary->count(); ++i)
            actual.append(dictionary->itemData(i).toULongLong());
        QCOMPARE(actual, expected);
        const bool bzip = methodName == "BZip2";
        QCOMPARE(dictionary->itemText(0),
                 bzip ? QStringLiteral("*  900 KB") : QStringLiteral("*  16 MB"));
        dictionary->setCurrentIndex(1);
        level->setCurrentIndex(level->findData(9u));
        method->setCurrentText(methodName);
        QCOMPARE(dictionary->itemText(0),
                 bzip ? QStringLiteral("*  900 KB") : QStringLiteral("*  256 MB"));
        if (bzip) {
            level->setCurrentIndex(level->findData(1u));
            method->setCurrentText(methodName);
            QCOMPARE(dictionary->itemText(0), QStringLiteral("*  100 KB"));
            level->setCurrentIndex(level->findData(3u));
            method->setCurrentText(methodName);
            QCOMPARE(dictionary->itemText(0), QStringLiteral("*  500 KB"));
        }
        QCOMPARE(dialog.compressionOptions().Dict64, UInt64(-1));
        dialog.show();
        dictionary->setCurrentIndex(0);
        for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
            dialog.resize(size);
            QApplication::processEvents();
            dialog.findChild<QScrollArea *>("archiveOptionsScrollArea")
                ->ensureWidgetVisible(dictionary, 0, 80);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*dictionary, dialog));
            QVERIFY(UiInspection::writeReport(
                dialog, QString("memory-%1-%2-%3").arg(methodName, formatName).arg(size.width())));
        }
        QCOMPARE(dialog.compressionOptions().Dict64, UInt64(-1));
        level->setCurrentIndex(level->findData(0u));
        QVERIFY(!dictionary->isEnabled());
        QCOMPARE(dialog.compressionOptions().Dict64, UInt64(-1));
    }
    void fixedDictionary_data() {
        QTest::addColumn<QString>("methodName");
        QTest::addColumn<QString>("sizeText");
        QTest::newRow("Deflate") << QString("Deflate") << QString("32 KB");
        QTest::newRow("Deflate64") << QString("Deflate64") << QString("64 KB");
    }
    void fixedDictionary() {
        QFETCH(QString, methodName);
        QFETCH(QString, sizeText);
        QTemporaryDir dir;
        CreateArchiveDialog dialog(dir.path());
        dialog.findChild<QComboBox *>("archiveFormat")->setCurrentText("zip");
        dialog.findChild<QComboBox *>("compressionMethod")->setCurrentText(methodName);
        auto *dictionary = dialog.findChild<QComboBox *>("compressionDictionary");
        auto *level = dialog.findChild<QComboBox *>("compressionLevel");
        auto *method = dialog.findChild<QComboBox *>("compressionMethod");
        QCOMPARE(dictionary->count(), 1);
        QVERIFY(!dictionary->isEnabled());
        QCOMPARE(dictionary->itemText(0), QStringLiteral("*  %1").arg(sizeText));
        for (int value : {1, 3, 5, 7, 9}) {
            level->setCurrentIndex(level->findData(unsigned(value)));
            method->setCurrentText(methodName);
            QCOMPARE(dictionary->itemText(0), QStringLiteral("*  %1").arg(sizeText));
            QCOMPARE(dialog.compressionOptions().Dict64, UInt64(-1));
        }
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
                screenshots + QString("/memory-%1-zip-%2.png").arg(methodName).arg(size.width())));
        }
    }
    void operations_data() {
        QTest::addColumn<QString>("formatName");
        QTest::addColumn<QString>("methodName");
        QTest::addColumn<quint64>("validSize");
        QTest::addColumn<quint64>("invalidSize");
        QTest::newRow("PPMd-7z") << QString("7z") << QString("PPMd") << quint64(1 << 20)
                                 << quint64(5 << 20);
        QTest::newRow("PPMd-zip") << QString("zip") << QString("PPMd") << quint64(1 << 20)
                                  << quint64(3 << 20);
        QTest::newRow("BZip2-7z-small")
            << QString("7z") << QString("BZip2") << quint64(100 << 10) << quint64(150 << 10);
        QTest::newRow("BZip2-zip-large")
            << QString("zip") << QString("BZip2") << quint64(900 << 10) << quint64(150 << 10);
        QTest::newRow("Deflate") << QString("zip") << QString("Deflate") << quint64(0)
                                 << quint64(32 << 10);
        QTest::newRow("Deflate64")
            << QString("zip") << QString("Deflate64") << quint64(0) << quint64(64 << 10);
    }
    void operations() {
        QFETCH(QString, formatName);
        QFETCH(QString, methodName);
        QFETCH(quint64, validSize);
        QFETCH(quint64, invalidSize);
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("данные с пробелом.txt"));
        QFile file(source);
        const QByteArray contents(8192, 'x');
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(contents), qint64(contents.size()));
        file.close();
        CompressionOptions options;
        options.Method = methodName.toStdWString().c_str();
        options.Dict64 = validSize ? validSize : UInt64(-1);
        ArchiveBackend backend;
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        const QString target = dir.filePath("memory." + formatName);
        backend.create(target, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.takeFirst()[0].toBool(), "Archive creation failed");
        QProcess process;
        process.start("7zz", {"t", "--", target});
        QVERIFY(process.waitForFinished(15000));
        QCOMPARE(process.exitCode(), 0);
        const QString extracted = dir.filePath("unpacked");
        process.start("7zz", {"x", "-o" + extracted, "--", target});
        QVERIFY(process.waitForFinished(15000));
        QCOMPARE(process.exitCode(), 0);
        QFile restored(QDir(extracted).filePath(QFileInfo(source).fileName()));
        QVERIFY(restored.open(QIODevice::ReadOnly));
        QCOMPARE(restored.readAll(), contents);
        const QString invalid = dir.filePath("invalid." + formatName);
        options.Dict64 = invalidSize;
        backend.create(invalid, {source}, {}, false, options);
        QCOMPARE(done.size(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        QVERIFY(!QFileInfo::exists(invalid));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.resize(8 << 20));
        file.close();
        options.Dict64 = validSize ? validSize : UInt64(-1);
        backend.create(invalid, {source}, {}, false, options);
        QVERIFY(backend.busy());
        backend.cancel();
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY(!done.first()[0].toBool());
        QCOMPARE(done.first()[1].toString(), QString("Operation canceled."));
        QCOMPARE(backend.lastResult(), E_ABORT);
        QVERIFY(!QFileInfo::exists(invalid));
        QVERIFY(QFileInfo::exists(target));
        QVERIFY(QDir(dir.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
};
QTEST_MAIN(CompressionMemoryTests)
#include "compression_memory_tests.moc"

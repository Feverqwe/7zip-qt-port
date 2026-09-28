#include "archivebackend.h"
#include "createarchivedialog.h"
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
        QVERIFY(dictionary->isEnabled());
        QList<quint64> actual;
        for (int i = 0; i < dictionary->count(); ++i)
            actual.append(dictionary->itemData(i).toULongLong());
        QCOMPARE(actual, expected);
        QCOMPARE(dictionary->itemText(0), QStringLiteral("Автоматически (16 МиБ)"));
        dictionary->setCurrentIndex(1);
        level->setCurrentIndex(level->findData(9u));
        QCOMPARE(dictionary->itemText(0), QStringLiteral("Автоматически (256 МиБ)"));
        QCOMPARE(dialog.compressionOptions().dictionaryBytes, expected[1]);
        dialog.show();
        const QString screenshots = QDir::currentPath() + "/ui-screenshots";
        QVERIFY(QDir().mkpath(screenshots));
        dictionary->setCurrentIndex(0);
        for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
            dialog.resize(size);
            QApplication::processEvents();
            dialog.findChild<QScrollArea *>("archiveOptionsScrollArea")
                ->ensureWidgetVisible(dictionary, 0, 80);
            QApplication::processEvents();
            QVERIFY(dialog.grab().save(
                screenshots +
                QString("/memory-%1-%2-%3.png").arg(methodName, formatName).arg(size.width())));
        }
        QCOMPARE(dialog.compressionOptions().dictionaryBytes, quint64(0));
        level->setCurrentIndex(level->findData(0u));
        QVERIFY(!dictionary->isEnabled());
        QCOMPARE(dialog.compressionOptions().dictionaryBytes, quint64(0));
    }
    void operations_data() {
        QTest::addColumn<QString>("formatName");
        QTest::addColumn<QString>("methodName");
        QTest::addColumn<quint64>("validSize");
        QTest::addColumn<quint64>("invalidSize");
        QTest::newRow("PPMd-7z") << QString("7z") << QString("PPMd") << quint64(1 << 20)
                                 << quint64(5 << 20);
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
        options.method = methodName;
        options.dictionaryBytes = validSize;
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
        options.dictionaryBytes = invalidSize;
        backend.create(invalid, {source}, {}, false, options);
        QCOMPARE(done.size(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        QVERIFY(!QFileInfo::exists(invalid));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.resize(8 << 20));
        file.close();
        options.dictionaryBytes = validSize;
        backend.create(invalid, {source}, {}, false, options);
        QVERIFY(backend.busy());
        backend.cancel();
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY(!done.first()[0].toBool());
        QVERIFY(done.first()[1].toString().contains("отменена"));
        QVERIFY(!QFileInfo::exists(invalid));
        QVERIFY(QFileInfo::exists(target));
        QVERIFY(QDir(dir.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
};
QTEST_MAIN(CompressionMemoryTests)
#include "compression_memory_tests.moc"

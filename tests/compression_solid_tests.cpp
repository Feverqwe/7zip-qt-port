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
        auto *solid = dialog.findChild<QCheckBox *>("solidArchiveOption");
        auto *format = dialog.findChild<QComboBox *>("archiveFormat");
        QCOMPARE(block->count(), 18);
        QCOMPARE(block->itemData(1).toInt(), 1);
        QCOMPARE(block->itemData(2).toInt(), 2);
        QCOMPARE(block->itemData(11).toInt(), 1024);
        QCOMPARE(block->itemData(17).toInt(), 65536);
        block->setCurrentIndex(block->findData(32));
        QCOMPARE(dialog.compressionOptions().solidBlockMiB, 32);
        level->setCurrentIndex(level->findData(9u));
        QCOMPARE(dialog.compressionOptions().solidBlockMiB, 32);
        solid->setChecked(false);
        QVERIFY(!block->isEnabled());
        QCOMPARE(dialog.compressionOptions().solidBlockMiB, 0);
        solid->setChecked(true);
        block->setCurrentIndex(block->findData(65536));
        dialog.show();
        const QString screenshots = QDir::currentPath() + "/ui-screenshots";
        QVERIFY(QDir().mkpath(screenshots));
        for (QSize size : {QSize(760, 720), QSize(640, 480)}) {
            dialog.resize(size);
            QApplication::processEvents();
            dialog.findChild<QScrollArea *>("archiveOptionsScrollArea")
                ->ensureWidgetVisible(block, 0, 80);
            QApplication::processEvents();
            QVERIFY(dialog.grab().save(
                screenshots + QStringLiteral("/compression-solid-%1.png").arg(size.width())));
        }
        format->setCurrentText("zip");
        QVERIFY(!block->isEnabled());
        QCOMPARE(dialog.compressionOptions().solidBlockMiB, 0);
    }
    void createsWithSmallBlockAndRejectsInvalid() {
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
        options.solidBlockMiB = 1;
        const QString target = dir.filePath("block.7z");
        QSignalSpy done(&backend, &ArchiveBackend::finished);
        backend.create(target, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY2(done.takeFirst()[0].toBool(), "Creation with a 1 MiB block failed");
        QProcess test;
        test.start("7zz", {"t", "--", target});
        QVERIFY(test.waitForFinished(15000));
        QCOMPARE(test.exitCode(), 0);
        options.solidBlockMiB = 3;
        const QString invalid = dir.filePath("invalid.7z");
        backend.create(invalid, {source}, {}, false, options);
        QCOMPARE(done.size(), 1);
        QVERIFY(!done.takeFirst()[0].toBool());
        QVERIFY(!QFileInfo::exists(invalid));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.resize(8 << 20));
        file.close();
        options.solidBlockMiB = 32;
        backend.create(invalid, {source}, {}, false, options);
        QVERIFY(backend.busy());
        backend.cancel();
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 15000);
        QVERIFY(!done.first()[0].toBool());
        QVERIFY(done.first()[1].toString().contains("отменена"));
        QVERIFY(!QFileInfo::exists(invalid));
        QVERIFY(QDir(dir.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
};
QTEST_MAIN(CompressionSolidTests)
#include "compression_solid_tests.moc"

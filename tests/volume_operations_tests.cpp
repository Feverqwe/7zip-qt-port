#include "volumedialog.h"
#include "volumeoperations.h"

#include <QFile>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class VolumeOperationsTests : public QObject {
    Q_OBJECT

  private slots:
    void parsesSizes() {
        QCOMPARE(VolumeOperation::parseSize("10M"), qint64(10) << 20);
        QCOMPARE(VolumeOperation::parseSize("650M - CD"), qint64(650) << 20);
        QCOMPARE(VolumeOperation::parseSize("23040M - BD"), qint64(23040) << 20);
        QCOMPARE(VolumeOperation::parseSize("1B"), qint64(1));
        QCOMPARE(VolumeOperation::parseSize("10M 20M"), qint64(0));
        QCOMPARE(VolumeOperation::parseSize("18446744073709551616"), qint64(0));
        QCOMPARE(VolumeOperation::parseSize("9223372036854775808"), qint64(0));
        QCOMPARE(VolumeOperation::parseSize(" 2 k "), qint64(2048));
        QCOMPARE(VolumeOperation::parseSize("0M"), qint64(0));
        QCOMPARE(VolumeOperation::parseSize("999999999999T"), qint64(0));
    }
    void splitsAndCombinesUnicodeFile() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString source = dir.filePath("архив space.bin");
        const QByteArray bytes = QByteArray("тест") + QByteArray(250000, 'x');
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(bytes), bytes.size());
        file.close();
        VolumeOperation split(VolumeOperation::Mode::Split, source, dir.path(), 100000);
        QSignalSpy splitResult(&split, &VolumeOperation::result);
        split.start();
        QVERIFY(split.wait());
        QCOMPARE(splitResult.size(), 1);
        QVERIFY(splitResult.first().first().toBool());
        const QString first = source + ".001";
        QString error;
        QCOMPARE(VolumeOperation::discoverParts(first, &error).size(), 3);
        const QString output = dir.filePath("restored.bin");
        VolumeOperation combine(VolumeOperation::Mode::Combine, first, output);
        QSignalSpy combineResult(&combine, &VolumeOperation::result);
        combine.start();
        QVERIFY(combine.wait());
        QVERIFY(combineResult.first().first().toBool());
        QFile result(output);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), bytes);
    }
    void rejectsCollisionAndMissingPart() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString source = dir.filePath("source.bin");
        QFile sourceFile(source);
        QVERIFY(sourceFile.open(QIODevice::WriteOnly));
        sourceFile.write(QByteArray(30, 'a'));
        sourceFile.close();
        QFile collision(source + ".001");
        QVERIFY(collision.open(QIODevice::WriteOnly));
        collision.write("existing");
        collision.close();
        VolumeOperation split(VolumeOperation::Mode::Split, source, dir.path(), 10);
        QSignalSpy spy(&split, &VolumeOperation::result);
        split.start();
        QVERIFY(split.wait());
        QVERIFY(!spy.first().first().toBool());
        QVERIFY(collision.open(QIODevice::ReadOnly));
        QCOMPARE(collision.readAll(), QByteArray("existing"));
        collision.close();
        QVERIFY(!QFileInfo::exists(source + ".002"));
        QFile second(source + ".002");
        QVERIFY(second.open(QIODevice::WriteOnly));
        second.write("second");
        second.close();
        QFile third(source + ".004");
        QVERIFY(third.open(QIODevice::WriteOnly));
        third.write("third");
        third.close();
        QString error;
        QVERIFY(VolumeOperation::discoverParts(source + ".001", &error).isEmpty());
        QVERIFY(error.contains("пропуск"));
    }
    void cancellationRemovesPartialOutput() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString source = dir.filePath("large.bin");
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.resize(8 << 20));
        file.close();
        VolumeOperation split(VolumeOperation::Mode::Split, source, dir.path(), 1 << 20);
        connect(
            &split, &VolumeOperation::advanced, &split,
            [&split](qint64 bytes) {
                if (bytes >= (1 << 20))
                    split.cancel();
            },
            Qt::DirectConnection);
        QSignalSpy spy(&split, &VolumeOperation::result);
        split.start();
        QVERIFY(split.wait());
        QVERIFY(!spy.first().first().toBool());
        QVERIFY(!QFileInfo::exists(source + ".001"));
        QCOMPARE(QDir(dir.path()).entryList({"*.sevenzip-*"}, QDir::Files).size(), 0);
    }
    void splitDialogHasDestinationAndSize() {
        QTemporaryDir dir;
        VolumeDialog dialog(VolumeOperation::Mode::Split, dir.filePath("source.bin"));
        QCOMPARE(dialog.destination(), dir.path());
        QCOMPARE(dialog.volumeSize(), qint64(10) << 20);
        QVERIFY(dialog.findChild<QLineEdit *>("volumeDestination"));
        VolumeDialog combine(VolumeOperation::Mode::Combine, dir.filePath("source.bin.001"));
        QVERIFY(!combine.findChild<QComboBox *>("volumeSize"));
    }
};

QTEST_MAIN(VolumeOperationsTests)
#include "volume_operations_tests.moc"

#include "CPP/7zip/UI/FileManager/PanelSplitFile.h"
#include "volumeoperations.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
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
    void parsesSizeSequences() {
        QCOMPARE(VolumeOperation::parseSizes(" 10M 20M "),
                 (QList<qint64>{qint64(10) << 20, qint64(20) << 20}));
        QCOMPARE(VolumeOperation::parseSizes("10 20 30B"), (QList<qint64>{10, 20, 30}));
        QVERIFY(VolumeOperation::parseSizes("10M 0M").isEmpty());
        QVERIFY(VolumeOperation::parseSizes("10M 9223372036854775808").isEmpty());
        QVERIFY(VolumeOperation::parseSizes(" ").isEmpty());
        QVERIFY(VolumeOperation::parseSizes("wrong").isEmpty());
    }
    void splitsWithSizeSequence_data() {
        QTest::addColumn<QList<qint64>>("sizes");
        QTest::addColumn<QList<qint64>>("expected");
        QTest::newRow("repeat-last") << QList<qint64>{10, 20} << QList<qint64>{10, 20, 20, 20, 5};
        QTest::newRow("three-sizes") << QList<qint64>{10, 20, 30} << QList<qint64>{10, 20, 30, 15};
        QTest::newRow("unused-tail") << QList<qint64>{10, 100, 30} << QList<qint64>{10, 65};
    }
    void splitsWithSizeSequence() {
        QFETCH(QList<qint64>, sizes);
        QFETCH(QList<qint64>, expected);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString source = dir.filePath("последовательность space.bin");
        QByteArray bytes;
        for (int i = 0; i < 75; ++i)
            bytes.append(char(i));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(bytes), qint64(75));
        file.close();
        VolumeOperation split(VolumeOperation::Mode::Split, source, dir.path(), sizes);
        QSignalSpy spy(&split, &VolumeOperation::result);
        split.start();
        QVERIFY(split.wait());
        QCOMPARE(spy.size(), 1);
        QVERIFY(spy.first().first().toBool());
        QByteArray restored;
        for (qsizetype i = 0; i < expected.size(); ++i) {
            QFile part(source + QStringLiteral(".%1").arg(i + 1, 3, 10, QLatin1Char('0')));
            QVERIFY(part.open(QIODevice::ReadOnly));
            QCOMPARE(part.size(), expected[i]);
            restored += part.readAll();
        }
        QCOMPARE(restored, bytes);
        QVERIFY(!QFileInfo::exists(
            source + QStringLiteral(".%1").arg(expected.size() + 1, 3, 10, QLatin1Char('0'))));
        const QString invalidDestination = dir.filePath("invalid");
        QVERIFY(QDir().mkpath(invalidDestination));
        VolumeOperation invalid(VolumeOperation::Mode::Split, source, invalidDestination,
                                QList<qint64>{10, -1});
        QSignalSpy failed(&invalid, &VolumeOperation::result);
        invalid.start();
        QVERIFY(invalid.wait());
        QVERIFY(!failed.first().first().toBool());
        QVERIFY(QDir(invalidDestination).entryList(QDir::Files | QDir::Hidden).isEmpty());
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
        VolumeOperation split(VolumeOperation::Mode::Split, source, dir.path(),
                              QList<qint64>{1 << 20, 2 << 20});
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
    void combineRollback_data() {
        QTest::addColumn<int>("failure");
        QTest::newRow("cancel") << 0;
        QTest::newRow("remove-next-part") << 1;
        QTest::newRow("grow-current-part") << 2;
        QTest::newRow("occupied-output") << 3;
    }
    void combineRollback() {
        QFETCH(int, failure);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString base = dir.filePath(QStringLiteral("части с пробелом.bin"));
        for (const QString &suffix : {".001", ".002"}) {
            QFile part(base + suffix);
            QVERIFY(part.open(QIODevice::WriteOnly));
            QCOMPARE(part.write(QByteArray(2 << 20, 'x')), qint64(2 << 20));
        }
        if (failure == 3) {
            QFile occupied(base);
            QVERIFY(occupied.open(QIODevice::WriteOnly));
            QCOMPARE(occupied.write("existing"), qint64(8));
        }
        VolumeOperation combine(VolumeOperation::Mode::Combine, base + ".001", base);
        bool triggered = false;
        bool changed = false;
        connect(
            &combine, &VolumeOperation::advanced, &combine,
            [&](qint64 bytes) {
                if (bytes == 0 || triggered)
                    return;
                triggered = true;
                if (failure == 0)
                    combine.cancel();
                else if (failure == 1)
                    changed = QFile::remove(base + ".002");
                else if (failure == 2) {
                    QFile current(base + ".001");
                    changed = current.open(QIODevice::Append) && current.write("extra") == 5;
                }
            },
            Qt::DirectConnection);
        QSignalSpy result(&combine, &VolumeOperation::result);
        combine.start();
        QVERIFY(combine.wait());
        QCOMPARE(result.size(), 1);
        QVERIFY(!result.first().first().toBool());
        if (failure == 1 || failure == 2)
            QVERIFY(changed);
        if (failure == 0)
            QCOMPARE(result.first()[1].toString(), QStringLiteral("Операция отменена."));
        if (failure == 3) {
            QFile occupied(base);
            QVERIFY(occupied.open(QIODevice::ReadOnly));
            QCOMPARE(occupied.readAll(), QByteArray("existing"));
        } else
            QVERIFY(!QFileInfo::exists(base));
        QVERIFY(QDir(dir.path()).entryList({"*.sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }

    void splitRejectsSourceChange_data() {
        QTest::addColumn<bool>("grow");
        QTest::newRow("shrink") << false;
        QTest::newRow("grow") << true;
    }
    void splitRejectsSourceChange() {
        QFETCH(bool, grow);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString source = dir.filePath(QStringLiteral("изменённый источник.bin"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.resize(3 << 20));
        file.close();
        VolumeOperation split(VolumeOperation::Mode::Split, source, dir.path(), 1 << 20);
        bool changed = false;
        connect(
            &split, &VolumeOperation::advanced, &split,
            [&](qint64 bytes) {
                if (bytes != (1 << 20))
                    return;
                QFile input(source);
                changed =
                    input.open(QIODevice::ReadWrite) && input.resize(grow ? (4 << 20) : (1 << 20));
            },
            Qt::DirectConnection);
        QSignalSpy result(&split, &VolumeOperation::result);
        split.start();
        QVERIFY(split.wait());
        QVERIFY(changed);
        QCOMPARE(result.size(), 1);
        QVERIFY(!result.first().first().toBool());
        QVERIFY(result.first()[1].toString().contains("изменился"));
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden),
                 QStringList{QFileInfo(source).fileName()});
    }

    void combineSummary() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString base = dir.filePath(QStringLiteral("части с пробелами.bin"));
        for (int i = 1; i <= 5; ++i) {
            QFile file(base + QStringLiteral(".%1").arg(i, 3, 10, QLatin1Char('0')));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(QByteArray(i, 'x')), qint64(i));
        }
        QString error;
        const auto parts = VolumeOperation::inspectParts(base + ".001", &error);
        QCOMPARE(parts.paths.size(), 5);
        QCOMPARE(parts.totalSize, qint64(15));
        CThreadCombine combiner;
        combiner.InputDirPrefix = dir.path() + '/';
        combiner.TotalSize = quint64(parts.totalSize);
        for (const QString &part : parts.paths)
            combiner.Names << QFileInfo(part).fileName();
        const QString info = combiner.GetInfo();
        QVERIFY(info.contains("Файлов: 5"));
        QVERIFY(info.contains("15 байт"));
        QVERIFY(info.contains("части с пробелами.bin.001"));
        QVERIFY(info.contains("части с пробелами.bin.002"));
        QVERIFY(info.contains("части с пробелами.bin.005"));
        QVERIFY(!info.contains("части с пробелами.bin.003"));
        QVERIFY(info.contains("..."));
        QVERIFY(!QFileInfo::exists(base));
        QVERIFY(QFile::remove(base + ".003"));
        QVERIFY(VolumeOperation::inspectParts(base + ".001", &error).paths.isEmpty());
        QVERIFY(error.contains("пропуск"));
    }
};

QTEST_MAIN(VolumeOperationsTests)
#include "volume_operations_tests.moc"

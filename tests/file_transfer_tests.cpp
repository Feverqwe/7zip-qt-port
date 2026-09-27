#include "filetransferoperation.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class FileTransferTests : public QObject {
    Q_OBJECT

  private slots:
    void copiesNestedUnicodeAndMovesFile() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkpath(QStringLiteral("source/папка")));
        QVERIFY(QDir(temp.path()).mkdir("destination"));
        const QString source = temp.filePath(QStringLiteral("source/папка/имя с пробелом.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("contents"), 8);
        file.close();
        FileTransferOperation copy({temp.filePath("source")}, temp.filePath("destination"), false);
        QSignalSpy copied(&copy, &FileTransferOperation::result);
        copy.start();
        QVERIFY(copy.wait(30000));
        QTRY_COMPARE(copied.size(), 1);
        QVERIFY(copied.first().first().toBool());
        QFile duplicate(
            temp.filePath(QStringLiteral("destination/source/папка/имя с пробелом.txt")));
        QVERIFY(duplicate.open(QIODevice::ReadOnly));
        QCOMPARE(duplicate.readAll(), QByteArray("contents"));
        FileTransferOperation move({source}, temp.filePath("destination"), true);
        QSignalSpy moved(&move, &FileTransferOperation::result);
        move.start();
        QVERIFY(move.wait(30000));
        QTRY_COMPARE(moved.size(), 1);
        QVERIFY(moved.first().first().toBool());
        QVERIFY(!QFile::exists(source));
        QVERIFY(QFile::exists(temp.filePath(QStringLiteral("destination/имя с пробелом.txt"))));
    }

    void rejectsOccupiedTargetAndDestinationInsideSource() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkpath("source/child"));
        const QString source = temp.filePath("source");
        FileTransferOperation recursive({source}, temp.filePath("source/child"), false);
        QSignalSpy rejected(&recursive, &FileTransferOperation::result);
        recursive.start();
        QVERIFY(recursive.wait(30000));
        QTRY_COMPARE(rejected.size(), 1);
        QVERIFY(!rejected.first().first().toBool());
        QVERIFY(!QFile::exists(temp.filePath("source/child/source")));
        QVERIFY(QDir(temp.path()).mkdir("destination"));
        QVERIFY(QDir(temp.path()).mkdir("destination/source"));
        FileTransferOperation collision({source}, temp.filePath("destination"), true);
        QSignalSpy occupied(&collision, &FileTransferOperation::result);
        collision.start();
        QVERIFY(collision.wait(30000));
        QTRY_COMPARE(occupied.size(), 1);
        QVERIFY(!occupied.first().first().toBool());
        QVERIFY(QDir(source).exists());
    }

    void cancellationLeavesNoPublishedOrStagedFile() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir("destination"));
        const QString source = temp.filePath("large file.bin");
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.resize(64 * 1024 * 1024));
        file.close();
        FileTransferOperation copy({source}, temp.filePath("destination"), false);
        QSignalSpy finished(&copy, &FileTransferOperation::result);
        connect(
            &copy, &FileTransferOperation::advanced, &copy, [&copy](qint64) { copy.cancel(); },
            Qt::DirectConnection);
        copy.start();
        QVERIFY(copy.wait(30000));
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(!finished.first().first().toBool());
        QCOMPARE(
            QDir(temp.filePath("destination")).entryList(QDir::AllEntries | QDir::NoDotAndDotDot),
            QStringList());
        QVERIFY(QFile::exists(source));
    }

#ifndef Q_OS_WIN
    void rejectsNestedSymbolicLinkForCopyAndMove() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkpath("source/child"));
        QVERIFY(QDir(temp.path()).mkdir("destination"));
        const QString source = temp.filePath("source");
        QVERIFY(QFile::link(temp.path(), temp.filePath("source/child/link")));
        for (bool move : {false, true}) {
            FileTransferOperation operation({source}, temp.filePath("destination"), move);
            QSignalSpy finished(&operation, &FileTransferOperation::result);
            operation.start();
            QVERIFY(operation.wait(30000));
            QTRY_COMPARE(finished.size(), 1);
            QVERIFY(!finished.first().first().toBool());
            QVERIFY(QDir(source).exists());
            QVERIFY(!QDir(temp.filePath("destination/source")).exists());
        }
    }
#endif
};

QTEST_MAIN(FileTransferTests)
#include "file_transfer_tests.moc"

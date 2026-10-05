#include "archivebackend.h"
#include "qtarchivetransferbackup.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class ArchiveTransferBackupTests : public QObject {
    Q_OBJECT
    QTemporaryDir registry_;
    QByteArray previousRegistry_;
    bool hadRegistry_ = false;

  private slots:
    void initTestCase() {
        QVERIFY(registry_.isValid());
        hadRegistry_ = qEnvironmentVariableIsSet("SEVENZIP_OWNED_TEMP_REGISTRY");
        previousRegistry_ = qgetenv("SEVENZIP_OWNED_TEMP_REGISTRY");
        qputenv("SEVENZIP_OWNED_TEMP_REGISTRY", registry_.path().toUtf8());
    }
    void cleanupTestCase() {
        if (hadRegistry_)
            qputenv("SEVENZIP_OWNED_TEMP_REGISTRY", previousRegistry_);
        else
            qunsetenv("SEVENZIP_OWNED_TEMP_REGISTRY");
    }
    void restoresOriginalDestinationBytes_data() {
        QTest::addColumn<QByteArray>("original");
        QTest::newRow("empty") << QByteArray();
        QTest::newRow("multiple-copy-buffers") << QByteArray(1024 * 1024 + 37, '\xA5');
    }
    void restoresOriginalDestinationBytes() {
        QFETCH(QByteArray, original);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString destination = fixture.filePath(QStringLiteral("архив с пробелом.zip"));
        QFile file(destination);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(original), qint64(original.size()));
        file.close();
        ArchiveBackend backend;
        auto backup = backend.saveArchiveTransferDestination(destination);
        QVERIFY(backup);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write("changed destination"), qint64(19));
        file.close();
        QVERIFY(backup->restore());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), original);
    }
    void absentDestinationIsRefused() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        ArchiveBackend backend;
        QVERIFY(!backend.saveArchiveTransferDestination(fixture.filePath("absent.zip")));
        QVERIFY(!QFileInfo::exists(fixture.filePath("absent.zip")));
    }
    void failedRestorePreservesReplacementDirectory() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString destination = fixture.filePath("destination.zip");
        QFile file(destination);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("original"), qint64(8));
        file.close();
        ArchiveBackend backend;
        auto backup = backend.saveArchiveTransferDestination(destination);
        QVERIFY(backup);
        QVERIFY(QFile::remove(destination));
        QVERIFY(QDir().mkdir(destination));
        QFile sentinel(QDir(destination).filePath("keep.txt"));
        QVERIFY(sentinel.open(QIODevice::WriteOnly));
        QCOMPARE(sentinel.write("keep"), qint64(4));
        sentinel.close();
        QVERIFY(!backup->restore());
        QVERIFY(QFileInfo(destination).isDir());
        QVERIFY(sentinel.open(QIODevice::ReadOnly));
        QCOMPARE(sentinel.readAll(), QByteArray("keep"));
    }
};

QTEST_GUILESS_MAIN(ArchiveTransferBackupTests)
#include "archive_transfer_backup_tests.moc"

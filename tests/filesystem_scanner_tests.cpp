#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "Windows/PropVariant.h"
#include "filesystemfolderloader.h"
#include "filesystemscanner.h"
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>
#include <memory>

class FilesystemScannerTests : public QObject {
    Q_OBJECT
    static void createFiles(const QString &directory, int count) {
        QVERIFY(QDir(directory).mkpath(QStringLiteral("папка с пробелом/sub")));
        for (int i = 0; i < count; ++i) {
            QFile file(QDir(directory).filePath(
                QStringLiteral("папка с пробелом/sub/📦-текущий-%1.txt").arg(i)));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("fixture"), qint64(7));
        }
    }
  private slots:
    void sourceOwnershipAndOrderedBatches() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        createFiles(temp.path(), 300);
        FilesystemScanner scanner;
        CMyComPtr<IFolderFolder> retained;
        QList<QString> displayedPaths;
        bool ordered = true;
        bool batchesOnGui = true;
        connect(&scanner, &FilesystemScanner::folderReady, this,
                [&](IFolderFolder *folder) { retained = folder; });
        connect(&scanner, &FilesystemScanner::batchReady, this,
                [&](const QFileInfoList &files, quint32 first) {
                    ordered &= first == quint32(displayedPaths.size()) && files.size() <= 128;
                    batchesOnGui &= QThread::currentThread() == thread();
                    for (const QFileInfo &file : files)
                        displayedPaths.append(file.absoluteFilePath());
                });
        QSignalSpy completed(&scanner, &FilesystemScanner::completed);
        QSignalSpy failed(&scanner, &FilesystemScanner::failed);
        QSignalSpy progress(&scanner, &FilesystemScanner::advanced);
        bool timerRan = false;
        QTimer::singleShot(0, &scanner, [&] { timerRan = true; });
        scanner.start(temp.path());
        QVERIFY(scanner.isRunning());
        QTRY_COMPARE(completed.size(), 1);
        QVERIFY(timerRan && ordered && batchesOnGui);
        QVERIFY(failed.isEmpty());
        QVERIFY(!progress.isEmpty());
        QVERIFY(!scanner.isRunning());
        QVERIFY(retained);
        UInt32 count = 0;
        QCOMPARE(retained->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(302));
        QCOMPARE(displayedPaths.size(), qsizetype(count));
        const QString prefix = ToQString(GetFolderPath(retained));
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant name, relative;
            QCOMPARE(retained->GetProperty(i, kpidName, &name), S_OK);
            QCOMPARE(retained->GetProperty(i, kpidPrefix, &relative), S_OK);
            const QString path =
                prefix +
                (relative.vt == VT_BSTR ? ToQString(UString(relative.bstrVal)) : QString()) +
                ToQString(UString(name.bstrVal));
            QCOMPARE(QFileInfo(displayedPaths[int(i)]).canonicalFilePath(),
                     QFileInfo(QDir::fromNativeSeparators(path)).canonicalFilePath());
        }
        scanner.cancel();
        QCOMPARE(retained->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(302));
    }
    void cancellationDuringPresentationRejectsStaleGeneration() {
        QTemporaryDir first;
        QTemporaryDir second;
        QVERIFY(first.isValid() && second.isValid());
        createFiles(first.path(), 300);
        QFile file(second.filePath("only.txt"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        FilesystemScanner scanner;
        QStringList paths;
        bool restarted = false;
        connect(&scanner, &FilesystemScanner::batchReady, this,
                [&](const QFileInfoList &files, quint32) {
                    if (!restarted) {
                        restarted = true;
                        scanner.start(second.path());
                        return;
                    }
                    for (const QFileInfo &info : files)
                        paths.append(info.absoluteFilePath());
                });
        QSignalSpy completed(&scanner, &FilesystemScanner::completed);
        QSignalSpy failed(&scanner, &FilesystemScanner::failed);
        scanner.start(first.path());
        QTRY_COMPARE(completed.size(), 1);
        QVERIFY(restarted);
        QCOMPARE(paths, QStringList{second.filePath("only.txt")});
        QVERIFY(failed.isEmpty());
    }
    void cancelBeforeLoadAndDestroyActiveScanner() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        createFiles(temp.path(), 30);
        FilesystemFolderLoader loader(temp.path());
        loader.cancel();
        loader.start();
        QVERIFY(loader.wait(3000));
        QCOMPARE(loader.result(), E_ABORT);
        QVERIFY(loader.takeFolder() == nullptr);
        auto scanner = std::make_unique<FilesystemScanner>();
        QSignalSpy completed(scanner.get(), &FilesystemScanner::completed);
        QSignalSpy failed(scanner.get(), &FilesystemScanner::failed);
        scanner->start(temp.path());
        scanner.reset();
        QVERIFY(completed.isEmpty() && failed.isEmpty());
    }
    void restartFromNativeProgressDiscardsOldOwner() {
        QTemporaryDir first;
        QTemporaryDir second;
        QVERIFY(first.isValid() && second.isValid());
        createFiles(first.path(), 300);
        QFile file(second.filePath("only.txt"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        FilesystemScanner scanner;
        bool restarted = false;
        int owners = 0;
        QStringList paths;
        connect(&scanner, &FilesystemScanner::advanced, this, [&](quint64) {
            if (!restarted) {
                restarted = true;
                scanner.start(second.path());
            }
        });
        connect(&scanner, &FilesystemScanner::folderReady, this,
                [&](IFolderFolder *) { ++owners; });
        connect(&scanner, &FilesystemScanner::batchReady, this,
                [&](const QFileInfoList &files, quint32) {
                    for (const QFileInfo &info : files)
                        paths.append(info.absoluteFilePath());
                });
        QSignalSpy completed(&scanner, &FilesystemScanner::completed);
        QSignalSpy failed(&scanner, &FilesystemScanner::failed);
        scanner.start(first.path());
        QTRY_COMPARE(completed.size(), 1);
        QVERIFY(restarted);
        QCOMPARE(owners, 1);
        QCOMPARE(paths, QStringList{second.filePath("only.txt")});
        QVERIFY(failed.isEmpty());
    }
    void invalidDirectoryHasNoPartialListing() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        FilesystemScanner scanner;
        QSignalSpy failed(&scanner, &FilesystemScanner::failed);
        QSignalSpy completed(&scanner, &FilesystemScanner::completed);
        QSignalSpy batches(&scanner, &FilesystemScanner::batchReady);
        scanner.start(temp.filePath("missing"));
        QTRY_COMPARE(failed.size(), 1);
        QVERIFY(completed.isEmpty() && batches.isEmpty());
        QVERIFY(!scanner.isRunning());
    }
    void receiverCanDestroyScanner_data() {
        QTest::addColumn<bool>("inBatch");
        QTest::newRow("folder-ready") << false;
        QTest::newRow("batch-ready") << true;
    }
    void receiverCanDestroyScanner() {
        QFETCH(bool, inBatch);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        createFiles(temp.path(), 30);
        auto scanner = std::make_unique<FilesystemScanner>();
        connect(scanner.get(), &FilesystemScanner::folderReady, this, [&](IFolderFolder *) {
            if (!inBatch)
                scanner.reset();
        });
        connect(scanner.get(), &FilesystemScanner::batchReady, this,
                [&](const QFileInfoList &, quint32) {
                    if (inBatch)
                        scanner.reset();
                });
        scanner->start(temp.path());
        QTRY_VERIFY(!scanner);
    }
};
QTEST_GUILESS_MAIN(FilesystemScannerTests)
#include "filesystem_scanner_tests.moc"

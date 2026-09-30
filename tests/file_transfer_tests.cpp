#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "filetransferoperation.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class FileTransferTests : public QObject {
    Q_OBJECT

  private slots:
    void conflictAnswersPreserveAndReplace() {
        for (const int answer :
             {NOverwriteAnswer::kNo, NOverwriteAnswer::kYes, NOverwriteAnswer::kAutoRename}) {
            QTemporaryDir temp;
            QVERIFY(temp.isValid());
            QVERIFY(QDir(temp.path()).mkdir("destination"));
            const QString source = temp.filePath(QStringLiteral("длинное имя с пробелом.txt"));
            const QString target =
                temp.filePath(QStringLiteral("destination/длинное имя с пробелом.txt"));
            QFile incoming(source);
            QVERIFY(incoming.open(QIODevice::WriteOnly));
            QCOMPARE(incoming.write("new"), 3);
            incoming.close();
            QFile existing(target);
            QVERIFY(existing.open(QIODevice::WriteOnly));
            QCOMPARE(existing.write("old"), 3);
            existing.close();
            FileTransferOperation copy({source}, temp.filePath("destination"), false);
            QSignalSpy requested(&copy, &FileTransferOperation::overwriteRequested);
            QSignalSpy result(&copy, &FileTransferOperation::result);
            QSignalSpy advanced(&copy, &FileTransferOperation::advanced);
            connect(
                &copy, &FileTransferOperation::overwriteRequested, &copy,
                [&copy, answer](const QString &, const QString &) {
                    copy.submitOverwriteAnswer(answer);
                },
                Qt::DirectConnection);
            copy.start();
            QVERIFY(copy.wait(30000));
            QTRY_COMPARE(requested.size(), 1);
            QTRY_COMPARE(result.size(), 1);
            QVERIFY(result.first().first().toBool());
            qint64 bytes = 0;
            for (const auto &event : advanced)
                bytes += event.first().toLongLong();
            QCOMPARE(bytes, answer == NOverwriteAnswer::kNo ? qint64(0) : qint64(3));
            QVERIFY(QFile::exists(source));
            QFile contents(target);
            QVERIFY(contents.open(QIODevice::ReadOnly));
            QCOMPARE(contents.readAll(),
                     answer == NOverwriteAnswer::kYes ? QByteArray("new") : QByteArray("old"));
            if (answer == NOverwriteAnswer::kAutoRename) {
                QFile renamed(
                    temp.filePath(QStringLiteral("destination/длинное имя с пробелом_1.txt")));
                QVERIFY(renamed.open(QIODevice::ReadOnly));
                QCOMPARE(renamed.readAll(), QByteArray("new"));
            }
        }
    }

    void repeatedDecisionAndRollback() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir("destination"));
        QStringList sources;
        for (const QString &name : {QStringLiteral("first"), QStringLiteral("second")}) {
            const QString source = temp.filePath(name);
            const QString target = temp.filePath("destination/" + name);
            QFile incoming(source);
            QVERIFY(incoming.open(QIODevice::WriteOnly));
            incoming.write("new");
            incoming.close();
            QFile existing(target);
            QVERIFY(existing.open(QIODevice::WriteOnly));
            existing.write("old");
            existing.close();
            sources << source;
        }
        FileTransferOperation move(sources, temp.filePath("destination"), true);
        QSignalSpy requested(&move, &FileTransferOperation::overwriteRequested);
        QSignalSpy result(&move, &FileTransferOperation::result);
        connect(
            &move, &FileTransferOperation::overwriteRequested, &move,
            [&move](const QString &, const QString &) {
                move.submitOverwriteAnswer(NOverwriteAnswer::kYesToAll);
            },
            Qt::DirectConnection);
        move.start();
        QVERIFY(move.wait(30000));
        QTRY_COMPARE(result.size(), 1);
        QVERIFY(result.first().first().toBool());
        QCOMPARE(requested.size(), 1);
        for (const QString &source : sources) {
            QVERIFY(!QFile::exists(source));
            QFile target(temp.filePath("destination/" + QFileInfo(source).fileName()));
            QVERIFY(target.open(QIODevice::ReadOnly));
            QCOMPARE(target.readAll(), QByteArray("new"));
        }

        // A later cancellation restores both the replaced destination and the moved source.
        QFile restoredSource(sources.first());
        QVERIFY(restoredSource.open(QIODevice::WriteOnly));
        restoredSource.write("again");
        restoredSource.close();
        QFile secondSource(sources.last());
        QVERIFY(secondSource.open(QIODevice::WriteOnly));
        secondSource.write("second again");
        secondSource.close();
        FileTransferOperation canceled({sources.first(), sources.last()},
                                       temp.filePath("destination"), true);
        QSignalSpy canceledResult(&canceled, &FileTransferOperation::result);
        connect(
            &canceled, &FileTransferOperation::overwriteRequested, &canceled,
            [&canceled](const QString &existing, const QString &) {
                if (existing.endsWith("first"))
                    canceled.submitOverwriteAnswer(NOverwriteAnswer::kYes);
                else
                    canceled.cancel();
            },
            Qt::DirectConnection);
        canceled.start();
        QVERIFY(canceled.wait(30000));
        QTRY_COMPARE(canceledResult.size(), 1);
        QVERIFY(!canceledResult.first().first().toBool());
        QFile originalTarget(temp.filePath("destination/first"));
        QVERIFY(originalTarget.open(QIODevice::ReadOnly));
        QCOMPARE(originalTarget.readAll(), QByteArray("new"));
        QFile originalSource(sources.first());
        QVERIFY(originalSource.open(QIODevice::ReadOnly));
        QCOMPARE(originalSource.readAll(), QByteArray("again"));
    }

    void directoryConflictCanSkipOrReplace() {
        for (const int answer : {NOverwriteAnswer::kNo, NOverwriteAnswer::kYes}) {
            QTemporaryDir temp;
            QVERIFY(temp.isValid());
            QVERIFY(QDir(temp.path()).mkpath("incoming/folder"));
            QVERIFY(QDir(temp.path()).mkpath("destination/folder"));
            QFile incoming(temp.filePath("incoming/folder/new"));
            QVERIFY(incoming.open(QIODevice::WriteOnly));
            incoming.write("new");
            incoming.close();
            QFile existing(temp.filePath("destination/folder/old"));
            QVERIFY(existing.open(QIODevice::WriteOnly));
            existing.write("old");
            existing.close();
            FileTransferOperation copy({temp.filePath("incoming/folder")},
                                       temp.filePath("destination"), false);
            QSignalSpy result(&copy, &FileTransferOperation::result);
            connect(
                &copy, &FileTransferOperation::overwriteRequested, &copy,
                [&copy, answer](const QString &, const QString &) {
                    copy.submitOverwriteAnswer(answer);
                },
                Qt::DirectConnection);
            copy.start();
            QVERIFY(copy.wait(30000));
            QTRY_COMPARE(result.size(), 1);
            QVERIFY(result.first().first().toBool());
            QCOMPARE(QFile::exists(temp.filePath("destination/folder/old")),
                     answer == NOverwriteAnswer::kNo);
            QCOMPARE(QFile::exists(temp.filePath("destination/folder/new")),
                     answer == NOverwriteAnswer::kYes);
        }
    }

    void noToAllAndCancelDuringPromptLeaveSourcesUntouched() {
        for (bool cancel : {false, true}) {
            QTemporaryDir temp;
            QVERIFY(temp.isValid());
            QVERIFY(QDir(temp.path()).mkdir("destination"));
            QStringList sources;
            for (const QString &name : {QStringLiteral("first"), QStringLiteral("second")}) {
                QFile source(temp.filePath(name));
                QVERIFY(source.open(QIODevice::WriteOnly));
                source.write("new");
                source.close();
                QFile target(temp.filePath("destination/" + name));
                QVERIFY(target.open(QIODevice::WriteOnly));
                target.write("old");
                target.close();
                sources << source.fileName();
            }
            FileTransferOperation copy(sources, temp.filePath("destination"), false);
            QSignalSpy requested(&copy, &FileTransferOperation::overwriteRequested);
            QSignalSpy result(&copy, &FileTransferOperation::result);
            connect(
                &copy, &FileTransferOperation::overwriteRequested, &copy,
                [&copy, cancel](const QString &, const QString &) {
                    if (cancel)
                        copy.cancel();
                    else
                        copy.submitOverwriteAnswer(NOverwriteAnswer::kNoToAll);
                },
                Qt::DirectConnection);
            copy.start();
            QVERIFY(copy.wait(30000));
            QTRY_COMPARE(result.size(), 1);
            QCOMPARE(result.first().first().toBool(), !cancel);
            QCOMPARE(requested.size(), 1);
            for (const QString &source : sources) {
                QVERIFY(QFile::exists(source));
                QFile target(temp.filePath("destination/" + QFileInfo(source).fileName()));
                QVERIFY(target.open(QIODevice::ReadOnly));
                QCOMPARE(target.readAll(), QByteArray("old"));
            }
            QCOMPARE(QDir(temp.filePath("destination"))
                         .entryList(QDir::AllEntries | QDir::NoDotAndDotDot)
                         .size(),
                     2);
        }
    }

    void upstreamFolderCopyCountsHiddenFilesAndEmptyDirectories() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkpath(QStringLiteral("source/пустая папка")));
        const QByteArray bytes("copied bytes");
        for (const QString &name : {QStringLiteral(".hidden"), QStringLiteral("имя с пробелом")}) {
            QFile file(temp.filePath("source/" + name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(bytes), bytes.size());
        }
        NFsFolder::CCopyState state;
        quint64 completed = 0;
        QStringList paths;
        state.Progress = [&completed](quint64 value) {
            if (value < completed)
                return false;
            completed = value;
            return true;
        };
        state.CurrentFile = [&paths](const QString &path) { paths << path; };
        QVERIFY(NFsFolder::CopyFolder(state, temp.filePath("source"), temp.filePath("target")));
        QCOMPARE(completed, quint64(bytes.size() * 2));
        QCOMPARE(paths.size(), 2);
        QVERIFY(QDir(temp.filePath(QStringLiteral("target/пустая папка"))).exists());
        for (const QString &source : paths) {
            QFile copy(temp.filePath("target/" + QFileInfo(source).fileName()));
            QVERIFY(copy.open(QIODevice::ReadOnly));
            QCOMPARE(copy.readAll(), bytes);
        }
        QVERIFY(!NFsFolder::CopyFolder(state, temp.filePath("source"), temp.filePath("target")));
        QFile existing(temp.filePath("target/.hidden"));
        QVERIFY(existing.open(QIODevice::ReadOnly));
        QCOMPARE(existing.readAll(), bytes);
    }

    void canceledTreeCopyRollsBackEarlierSources() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkpath("tree/empty"));
        QVERIFY(QDir(temp.path()).mkdir("destination"));
        for (const QString &name : {QString("first"), QString("tree/second")}) {
            QFile file(temp.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("bytes"), 5);
        }
        FileTransferOperation copy({temp.filePath("first"), temp.filePath("tree")},
                                   temp.filePath("destination"), false);
        QSignalSpy finished(&copy, &FileTransferOperation::result);
        connect(
            &copy, &FileTransferOperation::currentFile, &copy,
            [&copy](const QString &path) {
                if (path.endsWith("second"))
                    copy.cancel();
            },
            Qt::DirectConnection);
        copy.start();
        QVERIFY(copy.wait(30000));
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(!finished.first().first().toBool());
        QCOMPARE(
            QDir(temp.filePath("destination")).entryList(QDir::AllEntries | QDir::NoDotAndDotDot),
            QStringList());
        QVERIFY(QFile::exists(temp.filePath("first")));
        QVERIFY(QFile::exists(temp.filePath("tree/second")));
    }

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

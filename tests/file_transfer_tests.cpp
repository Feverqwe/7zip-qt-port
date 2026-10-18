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
    void crossVolumeMovePublishesBeforeDeletingSource() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkpath(QStringLiteral("вложенная папка")));
        QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("назначение")));
        const QString file = temp.filePath(QStringLiteral("δ файл с пробелом.txt"));
        const QString folder = temp.filePath(QStringLiteral("вложенная папка"));
        QFile input(file);
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("file bytes"), qint64(10));
        input.close();
        QFile nested(QDir(folder).filePath(QStringLiteral("внутри.txt")));
        QVERIFY(nested.open(QIODevice::WriteOnly));
        QCOMPARE(nested.write("nested bytes"), qint64(12));
        nested.close();
        FileTransferOperation::FileSystemAdapter adapter;
        adapter.trySourceRename = [](const QString &, const QString &) { return false; };
        const QString destination = temp.filePath(QStringLiteral("назначение"));
        const QString occupied = QDir(destination).filePath(QFileInfo(file).fileName());
        QFile previous(occupied);
        QVERIFY(previous.open(QIODevice::WriteOnly));
        QCOMPARE(previous.write("old bytes"), qint64(9));
        previous.close();
        FileTransferOperation move({file, folder}, destination, true, nullptr, adapter);
        QSignalSpy overwrite(&move, &FileTransferOperation::overwriteRequested);
        QSignalSpy result(&move, &FileTransferOperation::result);
        bool oldPreservedAtPrompt = false;
        bool sourcePresentAtPrompt = false;
        connect(
            &move, &FileTransferOperation::overwriteRequested, &move,
            [&move, file, occupied, &oldPreservedAtPrompt,
             &sourcePresentAtPrompt](const QString &, const QString &) {
                QFile old(occupied);
                oldPreservedAtPrompt =
                    old.open(QIODevice::ReadOnly) && old.readAll() == QByteArray("old bytes");
                sourcePresentAtPrompt = QFileInfo::exists(file);
                move.submitOverwriteAnswer(NOverwriteAnswer::kYes);
            },
            Qt::DirectConnection);
        move.start();
        QVERIFY(move.wait(30000));
        QTRY_COMPARE(result.size(), 1);
        QVERIFY(result.first().first().toBool());
        QCOMPARE(overwrite.size(), 1);
        QVERIFY(oldPreservedAtPrompt);
        QVERIFY(sourcePresentAtPrompt);
        QVERIFY(!QFileInfo::exists(file));
        QVERIFY(!QFileInfo::exists(folder));
        QFile movedFile(QDir(destination).filePath(QFileInfo(file).fileName()));
        QVERIFY(movedFile.open(QIODevice::ReadOnly));
        QCOMPARE(movedFile.readAll(), QByteArray("file bytes"));
        QFile movedNested(QDir(destination).filePath(QStringLiteral("вложенная папка/внутри.txt")));
        QVERIFY(movedNested.open(QIODevice::ReadOnly));
        QCOMPARE(movedNested.readAll(), QByteArray("nested bytes"));
    }

    void crossVolumeDeleteFailureReportsPartialMove() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir("destination"));
        const QString first = temp.filePath("first.txt");
        const QString second = temp.filePath("second.txt");
        for (const QString &path : {first, second}) {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(path.toUtf8()), qint64(path.toUtf8().size()));
        }
        FileTransferOperation::FileSystemAdapter adapter;
        adapter.trySourceRename = [](const QString &, const QString &) { return false; };
        adapter.removeSource = [first](const QString &path) {
            return path == first ? QFile::remove(path) : false;
        };
        const QString destination = temp.filePath("destination");
        FileTransferOperation move({first, second}, destination, true, nullptr, adapter);
        QSignalSpy result(&move, &FileTransferOperation::result);
        move.start();
        QVERIFY(move.wait(30000));
        QTRY_COMPARE(result.size(), 1);
        QVERIFY(!result.first().first().toBool());
        QVERIFY(result.first().at(1).toString().contains(QStringLiteral("Could not delete")));
        QCOMPARE(move.resultKind(), FileTransferOperation::ResultKind::Failure);
        QVERIFY(!QFileInfo::exists(first));
        QVERIFY(QFileInfo::exists(second));
        for (const QString &path : {first, second}) {
            QFile published(QDir(destination).filePath(QFileInfo(path).fileName()));
            QVERIFY(published.open(QIODevice::ReadOnly));
            QCOMPARE(published.readAll(), path.toUtf8());
        }
    }

    void crossVolumeCancelBeforeDeleteRollsBackPublishedTargets() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir("destination"));
        const QString first = temp.filePath("first.txt");
        const QString second = temp.filePath("second.txt");
        for (const QString &path : {first, second}) {
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("keep"), qint64(4));
        }
        FileTransferOperation *operation = nullptr;
        FileTransferOperation::FileSystemAdapter adapter;
        adapter.trySourceRename = [&operation, second](const QString &source, const QString &) {
            if (source == second)
                operation->cancel();
            return false;
        };
        const QString destination = temp.filePath("destination");
        FileTransferOperation move({first, second}, destination, true, nullptr, adapter);
        operation = &move;
        QSignalSpy result(&move, &FileTransferOperation::result);
        move.start();
        QVERIFY(move.wait(30000));
        QTRY_COMPARE(result.size(), 1);
        QVERIFY(!result.first().first().toBool());
        QVERIFY(QFileInfo::exists(first));
        QVERIFY(QFileInfo::exists(second));
        QCOMPARE(QDir(destination).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).size(), 0);
    }

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

    void directoryConflictMergesAndAsksForNestedFile() {
        for (const int answer : {NOverwriteAnswer::kNo, NOverwriteAnswer::kYes}) {
            QTemporaryDir temp;
            QVERIFY(temp.isValid());
            QVERIFY(QDir(temp.path()).mkpath("incoming/folder"));
            QVERIFY(QDir(temp.path()).mkpath("destination/folder"));
            QFile incoming(temp.filePath("incoming/folder/new"));
            QVERIFY(incoming.open(QIODevice::WriteOnly));
            incoming.write("new");
            incoming.close();
            QFile incomingConflict(temp.filePath("incoming/folder/same"));
            QVERIFY(incomingConflict.open(QIODevice::WriteOnly));
            incomingConflict.write("incoming");
            incomingConflict.close();
            QFile existing(temp.filePath("destination/folder/old"));
            QVERIFY(existing.open(QIODevice::WriteOnly));
            existing.write("old");
            existing.close();
            QFile existingConflict(temp.filePath("destination/folder/same"));
            QVERIFY(existingConflict.open(QIODevice::WriteOnly));
            existingConflict.write("existing");
            existingConflict.close();
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
            QVERIFY(QFile::exists(temp.filePath("destination/folder/old")));
            QVERIFY(QFile::exists(temp.filePath("destination/folder/new")));
            QFile merged(temp.filePath("destination/folder/same"));
            QVERIFY(merged.open(QIODevice::ReadOnly));
            QCOMPARE(merged.readAll(), answer == NOverwriteAnswer::kYes ? QByteArray("incoming")
                                                                        : QByteArray("existing"));
        }
    }

    void directoryMoveMergeKeepsSkippedSourceFile() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath("incoming/folder/sub")));
        QVERIFY(QDir().mkpath(temp.filePath("destination/folder/sub")));
        QFile sourceConflict(temp.filePath("incoming/folder/sub/same"));
        QVERIFY(sourceConflict.open(QIODevice::WriteOnly));
        QCOMPARE(sourceConflict.write("incoming"), qint64(8));
        sourceConflict.close();
        QFile sourceNew(temp.filePath("incoming/folder/sub/new"));
        QVERIFY(sourceNew.open(QIODevice::WriteOnly));
        QCOMPARE(sourceNew.write("new"), qint64(3));
        sourceNew.close();
        QFile targetConflict(temp.filePath("destination/folder/sub/same"));
        QVERIFY(targetConflict.open(QIODevice::WriteOnly));
        QCOMPARE(targetConflict.write("existing"), qint64(8));
        targetConflict.close();
        FileTransferOperation move({temp.filePath("incoming/folder")}, temp.filePath("destination"),
                                   true);
        QSignalSpy result(&move, &FileTransferOperation::result);
        connect(
            &move, &FileTransferOperation::overwriteRequested, &move,
            [&move](const QString &, const QString &) {
                move.submitOverwriteAnswer(NOverwriteAnswer::kNo);
            },
            Qt::DirectConnection);
        move.start();
        QVERIFY(move.wait(30000));
        QTRY_COMPARE(result.size(), 1);
        QVERIFY(result.first().first().toBool());
        QFile destinationNew(temp.filePath("destination/folder/sub/new"));
        QVERIFY(destinationNew.open(QIODevice::ReadOnly));
        QCOMPARE(destinationNew.readAll(), QByteArray("new"));
        QFile destinationOld(temp.filePath("destination/folder/sub/same"));
        QVERIFY(destinationOld.open(QIODevice::ReadOnly));
        QCOMPARE(destinationOld.readAll(), QByteArray("existing"));
        QVERIFY(!QFileInfo::exists(sourceNew.fileName()));
        QVERIFY(QFileInfo::exists(sourceConflict.fileName()));
    }

    void directoryMergeAutoRenamesNestedConflict() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath("incoming/folder")));
        QVERIFY(QDir().mkpath(temp.filePath("destination/folder")));
        QFile source(temp.filePath("incoming/folder/same.txt"));
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write("incoming"), qint64(8));
        source.close();
        QFile target(temp.filePath("destination/folder/same.txt"));
        QVERIFY(target.open(QIODevice::WriteOnly));
        QCOMPARE(target.write("existing"), qint64(8));
        target.close();
        FileTransferOperation copy({temp.filePath("incoming/folder")}, temp.filePath("destination"),
                                   false);
        QSignalSpy result(&copy, &FileTransferOperation::result);
        connect(
            &copy, &FileTransferOperation::overwriteRequested, &copy,
            [&copy](const QString &, const QString &) {
                copy.submitOverwriteAnswer(NOverwriteAnswer::kAutoRename);
            },
            Qt::DirectConnection);
        copy.start();
        QVERIFY(copy.wait(30000));
        QTRY_COMPARE(result.size(), 1);
        QVERIFY(result.first().first().toBool());
        QVERIFY(target.open(QIODevice::ReadOnly));
        QCOMPARE(target.readAll(), QByteArray("existing"));
        const QStringList names =
            QDir(temp.filePath("destination/folder")).entryList(QDir::Files | QDir::NoDotAndDotDot);
        QCOMPARE(names.size(), 2);
        for (const QString &name : names) {
            if (name == QStringLiteral("same.txt"))
                continue;
            QFile renamed(temp.filePath("destination/folder/" + name));
            QVERIFY(renamed.open(QIODevice::ReadOnly));
            QCOMPARE(renamed.readAll(), QByteArray("incoming"));
        }
    }

    void directoryMergeCancelRestoresOriginalTree() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath("incoming/folder")));
        QVERIFY(QDir().mkpath(temp.filePath("destination/folder")));
        QFile source(temp.filePath("incoming/folder/same"));
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write("new"), qint64(3));
        source.close();
        QFile target(temp.filePath("destination/folder/same"));
        QVERIFY(target.open(QIODevice::WriteOnly));
        QCOMPARE(target.write("old"), qint64(3));
        target.close();
        FileTransferOperation move({temp.filePath("incoming/folder")}, temp.filePath("destination"),
                                   true);
        QSignalSpy result(&move, &FileTransferOperation::result);
        connect(
            &move, &FileTransferOperation::overwriteRequested, &move,
            [&move](const QString &, const QString &) { move.cancel(); }, Qt::DirectConnection);
        move.start();
        QVERIFY(move.wait(30000));
        QTRY_COMPARE(result.size(), 1);
        QVERIFY(!result.first().first().toBool());
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), QByteArray("new"));
        QVERIFY(target.open(QIODevice::ReadOnly));
        QCOMPARE(target.readAll(), QByteArray("old"));
        QCOMPARE(
            QDir(temp.filePath("destination")).entryList(QDir::NoDotAndDotDot | QDir::AllEntries),
            QStringList({QStringLiteral("folder")}));
    }

    void directoryMergeDeleteFailureRetainsPublishedBytes() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir().mkpath(temp.filePath("incoming/folder")));
        QVERIFY(QDir().mkpath(temp.filePath("destination/folder")));
        QFile source(temp.filePath("incoming/folder/new"));
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write("new bytes"), qint64(9));
        source.close();
        QFile original(temp.filePath("destination/folder/old"));
        QVERIFY(original.open(QIODevice::WriteOnly));
        QCOMPARE(original.write("old bytes"), qint64(9));
        original.close();
        FileTransferOperation::FileSystemAdapter adapter;
        adapter.removeSource = [](const QString &) { return false; };
        FileTransferOperation move({temp.filePath("incoming/folder")}, temp.filePath("destination"),
                                   true, nullptr, adapter);
        QSignalSpy result(&move, &FileTransferOperation::result);
        move.start();
        QVERIFY(move.wait(30000));
        QTRY_COMPARE(result.size(), 1);
        QVERIFY(!result.first().first().toBool());
        QVERIFY(QFileInfo::exists(source.fileName()));
        QVERIFY(QFileInfo::exists(temp.filePath("destination/folder/new")));
        QVERIFY(QFileInfo::exists(original.fileName()));
    }

    void noToAllAndCancelDuringPromptLeaveSourcesUntouched_data() {
        QTest::addColumn<bool>("cancel");
        QTest::addColumn<bool>("queued");
        QTest::newRow("skip-before-wait") << false << false;
        QTest::newRow("cancel-before-wait") << true << false;
        QTest::newRow("skip-from-gui") << false << true;
        QTest::newRow("cancel-from-gui") << true << true;
    }

    void noToAllAndCancelDuringPromptLeaveSourcesUntouched() {
        QFETCH(bool, cancel);
        QFETCH(bool, queued);
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
            queued ? Qt::QueuedConnection : Qt::DirectConnection);
        copy.start();
        QTRY_COMPARE(result.size(), 1);
        QVERIFY(copy.wait(30000));
        QCOMPARE(result.first().first().toBool(), !cancel);
        QCOMPARE(copy.resultKind(), cancel ? FileTransferOperation::ResultKind::Canceled
                                           : FileTransferOperation::ResultKind::Success);
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

    void mergesOccupiedDirectoryAndRejectsDestinationInsideSource() {
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
        QVERIFY(occupied.first().first().toBool());
        QVERIFY(!QDir(source).exists());
        QVERIFY(QDir(temp.filePath("destination/source/child")).exists());
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
    void preservesNestedSymbolicLinkForCopyAndMove() {
        for (bool move : {false, true}) {
            QTemporaryDir temp;
            QVERIFY(temp.isValid());
            QVERIFY(QDir(temp.path()).mkpath("source/child"));
            QVERIFY(QDir(temp.path()).mkdir("destination"));
            const QString source = temp.filePath("source");
            QVERIFY(QFile::link(temp.path(), temp.filePath("source/child/link")));
            FileTransferOperation operation({source}, temp.filePath("destination"), move);
            QSignalSpy finished(&operation, &FileTransferOperation::result);
            operation.start();
            QVERIFY(operation.wait(30000));
            QTRY_COMPARE(finished.size(), 1);
            QVERIFY2(finished.first().first().toBool(), qPrintable(finished.first()[1].toString()));
            QCOMPARE(QDir(source).exists(), !move);
            const QFileInfo copied(temp.filePath("destination/source/child/link"));
            QVERIFY(copied.isSymLink());
            QCOMPARE(copied.symLinkTarget(), temp.path());
        }
    }
#endif
};

QTEST_MAIN(FileTransferTests)
#include "file_transfer_tests.moc"

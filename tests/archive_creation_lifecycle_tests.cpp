#include "CPP/7zip/UI/Common/EnumDirItems.h"
#include "Common/StringConvert.h"
#include "archivebackend.h"
#include "native_link_fixture.h"
#include "qtarchivedeleteinputs.h"
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

namespace {
bool writeFile(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
HRESULT snapshot(QtArchiveDeleteInputs &inputs, const QStringList &paths) {
    if (inputs.DirItems.SymLinks) {
        NWildcard::CCensor censor;
        for (const QString &path : paths)
            censor.AddPreItem_NoWildcard(path.toStdWString().c_str());
        censor.AddPathsToCensor(NWildcard::k_RelatPath);
        return EnumerateItems(censor, NWildcard::k_RelatPath, UString(), inputs.DirItems);
    }
    FStringVector names;
    for (const QString &path : paths)
        names.Add(us2fs(path.toStdWString().c_str()));
    return inputs.DirItems.EnumerateItems2(FString(), UString(), names, nullptr);
}
} // namespace

class ArchiveCreationLifecycleTests final : public QObject {
    Q_OBJECT

  private slots:
    void nativeStoredLinksDeleteOnlyProcessedUnchangedInputs() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString target = fixture.filePath("цель с пробелом.txt");
        QVERIFY(writeFile(target, "outside bytes"));
        const QString processed = fixture.filePath("processed link");
        const QString skipped = fixture.filePath("skipped link");
        const QString changed = fixture.filePath("changed link");
        for (const QString &path : {processed, skipped, changed})
            QVERIFY(CreateNativeSymbolicLink(path, target));
        QtArchiveDeleteInputs inputs;
        inputs.SymLinks.SetTrueTrue();
        inputs.DirItems.SymLinks = true;
        QCOMPARE(snapshot(inputs, {processed, skipped, changed}), S_OK);
        inputs.MarkCreatedInputsProcessed();
        QCOMPARE(inputs.DirItems.Items.Size(), 3U);
        for (unsigned i = 0; i < inputs.DirItems.Items.Size(); ++i) {
            QVERIFY(inputs.DirItems.Items[i].AreReparseData());
            const QString path = QDir::fromNativeSeparators(
                QString::fromWCharArray(fs2us(inputs.DirItems.GetPhyPath(i)).Ptr()));
            if (path == skipped)
                inputs.ProcessedItems[i] = 0;
        }
        QVERIFY(QFile::remove(changed));
        QVERIFY(writeFile(changed, "replacement file"));
        QtUpdateDeletionCallback callback;
        QCOMPARE(inputs.Delete(callback), S_OK);
        QVERIFY(!QFileInfo(processed).isSymLink());
        QVERIFY(QFileInfo(skipped).isSymLink());
        QCOMPARE(readFile(changed), QByteArray("replacement file"));
        QCOMPARE(readFile(target), QByteArray("outside bytes"));
    }
    void createStoredLinksDeletesAfterPublication_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<int>("outcome");
        QTest::addColumn<bool>("selectedLeaves");
        for (const QString &format : {QString("7z"), QString("zip")}) {
#ifdef Q_OS_WIN
            QTest::newRow(qPrintable(format + "-unproved-link-capability-refusal"))
                << format << 3 << false;
#else
            QTest::newRow(qPrintable(format + "-success")) << format << 0 << false;
            QTest::newRow(qPrintable(format + "-selected")) << format << 0 << true;
            QTest::newRow(qPrintable(format + "-cancel")) << format << 1 << false;
            QTest::newRow(qPrintable(format + "-failure")) << format << 2 << false;
#endif
        }
    }
    void createStoredLinksDeletesAfterPublication() {
        QFETCH(QString, format);
        QFETCH(int, outcome);
        QFETCH(bool, selectedLeaves);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString target = fixture.filePath("outside/цель.txt");
        QVERIFY(QDir().mkpath(QFileInfo(target).absolutePath()));
        QVERIFY(writeFile(target, "outside bytes"));
        const QString root = fixture.filePath("source folder");
        QVERIFY(QDir().mkpath(root));
        QVERIFY(CreateNativeSymbolicLink(root + "/file link", target));
        QVERIFY(
            CreateNativeSymbolicLink(root + "/directory link", QFileInfo(target).absolutePath()));
        QVERIFY(
            CreateNativeSymbolicLink(root + "/dangling link", fixture.filePath("missing target")));
        const QString archive = fixture.filePath("result." + format);
        CompressionOptions options;
        options.SymLinks.SetTrueTrue();
        if (selectedLeaves)
            options.SymLinks.Def = false;
        options.DeleteAfterCompressing = true;
        ArchiveBackend backend(nullptr,
                               outcome == 2 ? fixture.filePath("missing-engine") : QString());
        if (outcome == 1)
            connect(&backend, &ArchiveBackend::engineStarted, this, [&] { backend.cancel(); });
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        const QStringList sources = selectedLeaves
                                        ? QStringList{root + "/file link", root + "/directory link",
                                                      root + "/dangling link"}
                                        : QStringList{root};
        backend.create(archive, sources, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QCOMPARE(finished.last().first().toBool(), outcome == 0);
        if (outcome == 3)
            QCOMPARE(started.size(), 0);
        QCOMPARE(readFile(target), QByteArray("outside bytes"));
        if (outcome != 0) {
            QVERIFY(QFileInfo(root + "/file link").isSymLink());
            QVERIFY(QFileInfo(root + "/directory link").isSymLink());
            QVERIFY(QFileInfo(root + "/dangling link").isSymLink());
            QVERIFY(!QFileInfo::exists(archive));
            return;
        }
        if (selectedLeaves)
            QVERIFY(QDir(root).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty());
        else
            QVERIFY(!QFileInfo::exists(root));
        const QString extracted = fixture.filePath("extracted");
        QProcess engine;
        engine.start("7zz", {"x", "-y", "-snld", "-o" + extracted, "--", archive});
        QVERIFY(engine.waitForFinished(15000));
        QCOMPARE(engine.exitCode(), 0);
        for (const QString &name :
             {QString("file link"), QString("directory link"), QString("dangling link")})
            QVERIFY2(QFileInfo(extracted + (selectedLeaves ? "/" : "/source folder/") + name)
                         .isSymLink(),
                     qPrintable(name));
    }
    void nativeProcessedAndChangedFiles() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString processed = fixture.filePath("processed.txt");
        const QString skipped = fixture.filePath("skipped.txt");
        const QString changed = fixture.filePath("changed.txt");
        for (const QString &path : {processed, skipped, changed})
            QVERIFY(writeFile(path, "original"));
        QtArchiveDeleteInputs inputs;
        QCOMPARE(snapshot(inputs, {processed, skipped, changed}), S_OK);
        inputs.MarkCreatedInputsProcessed();
        for (unsigned i = 0; i < inputs.DirItems.Items.Size(); ++i)
            if (QDir::fromNativeSeparators(
                    QString::fromWCharArray(fs2us(inputs.DirItems.GetPhyPath(i)).Ptr())) == skipped)
                inputs.ProcessedItems[i] = 0;
        QVERIFY(writeFile(changed, "changed longer bytes"));
        QtUpdateDeletionCallback callback;
        QCOMPARE(inputs.Delete(callback), S_OK);
        QVERIFY(!QFileInfo::exists(processed));
        QCOMPARE(readFile(skipped), QByteArray("original"));
        QCOMPARE(readFile(changed), QByteArray("changed longer bytes"));
    }
    void nativeTimeChangesPreserveSameSizeFiles() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("source.txt");
        QVERIFY(writeFile(source, "original"));
        QtArchiveDeleteInputs inputs;
        QCOMPARE(snapshot(inputs, {source}), S_OK);
        inputs.MarkCreatedInputsProcessed();
        QFile file(source);
        QVERIFY(file.open(QIODevice::ReadWrite));
        QVERIFY(file.setFileTime(QFileInfo(source).lastModified().addDays(-1),
                                 QFileDevice::FileModificationTime));
        file.close();
        QtUpdateDeletionCallback callback;
        QCOMPARE(inputs.Delete(callback), S_OK);
        QCOMPARE(readFile(source), QByteArray("original"));
#ifndef Q_OS_WIN
        QtArchiveDeleteInputs metadata;
        QCOMPARE(snapshot(metadata, {source}), S_OK);
        metadata.MarkCreatedInputsProcessed();
        // On POSIX the source CTime is inode metadata time. Changing permission
        // preserves size/mtime, and must prevent deletion of that changed input.
        NWindows::NFile::NFind::CFileInfo changedInfo;
        const auto metadataChanged = [&] {
            if (!QFile::setPermissions(source,
                                       QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup) ||
                !changedInfo.Find(inputs.DirItems.GetPhyPath(0)))
                return false;
            return Compare_FiTime(&changedInfo.CTime, &metadata.DirItems.Items[0].CTime) != 0;
        };
        // Some filesystems coalesce consecutive metadata changes in one tick.
        // Establish a real source CTime change before testing its deletion guard.
        QTRY_VERIFY_WITH_TIMEOUT(metadataChanged(), 2000);
        QCOMPARE(metadata.Delete(callback), S_OK);
        QCOMPARE(readFile(source), QByteArray("original"));
#endif
    }
    void nativeCancellationAndEmptyDirectoryOrder() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source folder");
        QVERIFY(QDir().mkpath(root + "/nested/empty"));
        const QString a = root + "/первый.txt", b = root + "/второй.txt";
        QVERIFY(writeFile(a, "a") && writeFile(b, "b"));
        QtArchiveDeleteInputs cancelled;
        QCOMPARE(snapshot(cancelled, {a, b}), S_OK);
        cancelled.MarkCreatedInputsProcessed();
        QtUpdateDeletionCallback stop;
        int calls = 0;
        stop.Progress = [&calls](const FString &, bool) { return ++calls == 2 ? E_ABORT : S_OK; };
        QCOMPARE(cancelled.Delete(stop), E_ABORT);
        QVERIFY(!QFileInfo::exists(a));
        QCOMPARE(readFile(b), QByteArray("b"));
        QtArchiveDeleteInputs remaining;
        QCOMPARE(snapshot(remaining, {root}), S_OK);
        remaining.MarkCreatedInputsProcessed();
        QtUpdateDeletionCallback callback;
        QStringList deletedDirectories;
        callback.Progress = [&](const FString &path, bool directory) {
            if (directory)
                deletedDirectories.append(
                    QDir::fromNativeSeparators(QString::fromWCharArray(fs2us(path).Ptr())));
            return S_OK;
        };
        QCOMPARE(remaining.Delete(callback), S_OK);
        QVERIFY(!QFileInfo::exists(root));
        QVERIFY(deletedDirectories.indexOf(root + "/nested/empty") <
                deletedDirectories.indexOf(root + "/nested"));
        QCOMPARE(deletedDirectories.last(), root);
    }
    void createDeletesOnlyAfterPublication_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("volumes");
        QTest::newRow("zip") << QString("zip") << false;
        QTest::newRow("7z") << QString("7z") << false;
        QTest::newRow("zip-volumes") << QString("zip") << true;
        QTest::newRow("7z-volumes") << QString("7z") << true;
    }
    void createDeletesOnlyAfterPublication() {
        QFETCH(QString, format);
        QFETCH(bool, volumes);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString root = fixture.filePath("source folder");
        QVERIFY(QDir().mkpath(root + "/empty"));
        const QString input = root + "/файл с пробелом.txt";
        const QByteArray bytes(8192, 'x');
        QVERIFY(writeFile(input, bytes));
        const QString archive = fixture.filePath("result." + format);
        CompressionOptions options;
        options.Level = 0;
        options.DeleteAfterCompressing = true;
        if (volumes)
            options.VolumeSizes.Add(2048);
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        bool sourceExistedDuringEngine = false;
        connect(&backend, &ArchiveBackend::engineStarted, this,
                [&] { sourceExistedDuringEngine = QFileInfo::exists(input); });
        backend.create(archive, {root}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QVERIFY2(finished.last().first().toBool(), qPrintable(finished.last().at(1).toString()));
        QVERIFY(sourceExistedDuringEngine);
        QVERIFY(!QFileInfo::exists(root));
        const QString result = volumes ? archive + ".001" : archive;
        QVERIFY(QFileInfo::exists(result));
        QProcess engine;
        const QString extracted = fixture.filePath("extracted");
        engine.start("7zz", {"x", "-y", "-o" + extracted, "--", result});
        QVERIFY(engine.waitForFinished(15000));
        QCOMPARE(engine.exitCode(), 0);
        QCOMPARE(readFile(extracted + "/source folder/файл с пробелом.txt"), bytes);
    }
    void cancelledSecondStagePreservesEverySource() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QVERIFY(QDir().mkpath(fixture.filePath("a")) && QDir().mkpath(fixture.filePath("b")));
        const QString a = fixture.filePath("a/один.txt"), b = fixture.filePath("b/two.txt");
        QVERIFY(writeFile(a, "a") && writeFile(b, "b"));
        ArchiveBackend backend;
        CompressionOptions options;
        options.DeleteAfterCompressing = true;
        int stages = 0;
        connect(&backend, &ArchiveBackend::engineStarted, this, [&] {
            if (++stages == 2)
                backend.cancel();
        });
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        const QString archive = fixture.filePath("result.7z");
        backend.create(archive, {a, b}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QCOMPARE(stages, 2);
        QVERIFY(!finished.last().first().toBool());
        QCOMPARE(readFile(a), QByteArray("a"));
        QCOMPARE(readFile(b), QByteArray("b"));
        QVERIFY(!QFileInfo::exists(archive));
    }
    void changedInputAndPublicationFailureRetainSources_data() {
        QTest::addColumn<bool>("occupiedVolume");
        QTest::newRow("changed-input") << false;
        QTest::newRow("occupied-volume") << true;
    }
    void changedInputAndPublicationFailureRetainSources() {
        QFETCH(bool, occupiedVolume);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("source.txt");
        const QString archive = fixture.filePath("result.7z");
        QVERIFY(writeFile(source, "old bytes"));
        ArchiveBackend backend;
        CompressionOptions options;
        options.DeleteAfterCompressing = true;
        if (occupiedVolume)
            options.VolumeSizes.Add(128);
        bool changed = false;
        connect(&backend, &ArchiveBackend::engineStarted, this, [&] {
            changed = writeFile(occupiedVolume ? archive + ".001" : source,
                                occupiedVolume ? "occupied" : "changed larger bytes");
        });
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(archive, {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QVERIFY(changed);
        QCOMPARE(finished.last().first().toBool(), !occupiedVolume);
        QCOMPARE(readFile(source),
                 occupiedVolume ? QByteArray("old bytes") : QByteArray("changed larger bytes"));
        if (occupiedVolume) {
            QCOMPARE(readFile(archive + ".001"), QByteArray("occupied"));
            QVERIFY(!QFileInfo::exists(archive));
        } else
            QVERIFY(QFileInfo::exists(archive));
    }
    void preparationCancelAndUnsupportedMode() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("source.txt");
        QVERIFY(writeFile(source, "original"));
        ArchiveBackend backend;
        CompressionOptions options;
        options.DeleteAfterCompressing = true;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        backend.create(fixture.filePath("cancelled.7z"), {source}, {}, false, options);
        backend.cancelAndWait();
        QCOMPARE(finished.size(), 1);
        QVERIFY(!finished.last().first().toBool());
        QCOMPARE(started.size(), 0);
        QCOMPARE(readFile(source), QByteArray("original"));
        finished.clear();
        options.UpdateMode = NCompressDialog::NUpdateMode::kFresh;
        backend.create(fixture.filePath("unsupported.7z"), {source}, {}, false, options);
        QCOMPARE(finished.size(), 1);
        QVERIFY(!finished.last().first().toBool());
        QCOMPARE(started.size(), 0);
        QCOMPARE(readFile(source), QByteArray("original"));
    }
    void engineStartFailureKeepsInput() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("source.txt");
        QVERIFY(writeFile(source, "original"));
        ArchiveBackend backend(nullptr, fixture.filePath("missing-engine"));
        CompressionOptions options;
        options.DeleteAfterCompressing = true;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(fixture.filePath("result.7z"), {source}, {}, false, options);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 15000);
        QVERIFY(!finished.last().first().toBool());
        QCOMPARE(readFile(source), QByteArray("original"));
        QVERIFY(!QFileInfo::exists(fixture.filePath("result.7z")));
        QVERIFY(
            QDir(fixture.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
};
QTEST_GUILESS_MAIN(ArchiveCreationLifecycleTests)
#include "archive_creation_lifecycle_tests.moc"

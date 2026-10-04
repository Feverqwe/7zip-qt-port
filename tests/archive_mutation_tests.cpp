#include "archive_backend_test_support.h"
#include <memory>

class ArchiveMutationTests : public QObject, protected ArchiveBackendTestSupport {
    Q_OBJECT

  private slots:
    void destroyedActiveBackend_data() {
        QTest::addColumn<bool>("creating");
        QTest::newRow("listing") << false;
        QTest::newRow("partial-create") << true;
    }
    void destroyedActiveBackend() {
        QFETCH(bool, creating);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("имя с пробелом.txt"));
        writeFile(source, "original");
        const QString archive = temp.filePath("unfinished.7z");
        qputenv("SEVENZIP_TEST_HELPER", creating ? "createpartial" : "cancel");
        auto backend =
            std::make_unique<ArchiveBackend>(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy output(backend.get(), &ArchiveBackend::outputReceived);
        QSignalSpy finished(backend.get(), &ArchiveBackend::finished);
        if (creating)
            backend->create(archive, {source});
        else
            backend->list(archive);
        QTRY_VERIFY_WITH_TIMEOUT(!output.isEmpty(), 3000);
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(backend->busy());
        backend.reset();
        QVERIFY(finished.isEmpty());
        QVERIFY(!QFileInfo::exists(archive));
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
        QFile original(source);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), QByteArray("original"));
    }
    void replaceEditedEntry_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QStringLiteral("7z");
        QTest::newRow("zip") << QStringLiteral("zip");
    }
    void replaceEditedEntry() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("папка с пробелом/файл.txt"));
        QVERIFY(QDir().mkpath(QFileInfo(source).absolutePath()));
        writeFile(source, "before");
        const QString archive = temp.filePath("edited." + format);
        ArchiveBackend backend;
        backend.create(archive, {source});
        await(backend, true);
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(archive);
        await(backend, true);
        QCOMPARE(entries.size(), 1);
        const QString entry = entries.first().path;
        QCOMPARE(entry.normalized(QString::NormalizationForm_C), QStringLiteral("файл.txt"));
        QTemporaryDir edit;
        QVERIFY(edit.isValid());
        writeFile(edit.filePath(entry), "after");
        QSignalSpy replaced(&backend, &ArchiveBackend::finished);
        backend.replaceEntry(archive, entry, edit.path());
        if (replaced.isEmpty())
            QVERIFY(replaced.wait(30000));
        const QList<QVariant> outcome = replaced.takeFirst();
        QVERIFY2(outcome.at(0).toBool(), qPrintable(outcome.at(1).toString()));
        backend.list(archive);
        await(backend, true);
        QTemporaryDir extracted;
        QVERIFY(extracted.isValid());
        backend.extract(archive, extracted.path(), {entry});
        await(backend, true);
        QFile result(extracted.filePath(entry));
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), QByteArray("after"));
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
    void rejectedReplacementPreservesArchive() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath("original.txt");
        const QString archive = temp.filePath("original.7z");
        writeFile(source, "original");
        ArchiveBackend backend;
        backend.create(archive, {source});
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        QFile saved(archive);
        QVERIFY(saved.open(QIODevice::ReadOnly));
        const QByteArray before = saved.readAll();
        saved.close();
        QTemporaryDir edit;
        QVERIFY(edit.isValid());
        writeFile(edit.filePath("original.txt"), "changed");
        QSignalSpy failure(&backend, &ArchiveBackend::finished);
        backend.replaceEntry(archive, "../original.txt", edit.path());
        QCOMPARE(failure.size(), 1);
        QVERIFY(!failure.takeFirst().at(0).toBool());
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), before);
    }
    void cancelledReplacementPreservesArchive() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath("original.txt");
        const QString archive = temp.filePath("original.zip");
        writeFile(source, "original");
        ArchiveBackend real;
        real.create(archive, {source});
        await(real, true);
        QFile saved(archive);
        QVERIFY(saved.open(QIODevice::ReadOnly));
        const QByteArray before = saved.readAll();
        saved.close();
        QTemporaryDir edit;
        QVERIFY(edit.isValid());
        writeFile(edit.filePath("original.txt"), "changed");

        qputenv("SEVENZIP_TEST_HELPER", "listing");
        qputenv("SEVENZIP_TEST_LISTING", "Path = original.txt\nSize = 8\n\n");
        ArchiveBackend helper(nullptr, QCoreApplication::applicationFilePath());
        helper.list(archive);
        await(helper, true);
        qunsetenv("SEVENZIP_TEST_LISTING");
        qputenv("SEVENZIP_TEST_HELPER", "createpartial");
        QSignalSpy output(&helper, &ArchiveBackend::outputReceived);
        QSignalSpy result(&helper, &ArchiveBackend::finished);
        helper.replaceEntry(archive, "original.txt", edit.path());
        QTRY_VERIFY_WITH_TIMEOUT(!output.isEmpty(), 3000);
        helper.cancel();
        QVERIFY(result.wait(30000));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(!result.takeFirst().at(0).toBool());
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), before);
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
    void addToExisting_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QString("7z");
        QTest::newRow("zip") << QString("zip");
    }
    void archiveMutation_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QStringLiteral("7z");
        QTest::newRow("zip") << QStringLiteral("zip");
    }
    void renameArchiveFolder_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QStringLiteral("7z");
        QTest::newRow("zip") << QStringLiteral("zip");
    }
    void renameArchiveFolder() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString folder = temp.filePath(QStringLiteral("старая папка"));
        QVERIFY(QDir().mkpath(folder + "/nested"));
        writeFile(folder + QStringLiteral("/nested/файл.txt"), "content");
        const QString archive = temp.filePath("folders." + format);
        ArchiveBackend backend;
        backend.create(archive, {folder});
        await(backend, true);
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(archive);
        await(backend, true);
        const QString oldName = QStringLiteral("старая папка");
        const QString newName = QStringLiteral("новая папка");
        backend.renameEntry(archive, oldName, newName);
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        QStringList paths;
        for (const ArchiveEntry &entry : entries)
            paths
                << QDir::fromNativeSeparators(entry.path).normalized(QString::NormalizationForm_C);
        QVERIFY(paths.contains(newName + QStringLiteral("/nested/файл.txt")));
        QVERIFY(!paths.contains(oldName + QStringLiteral("/nested/файл.txt")));
        QTemporaryDir extracted;
        QVERIFY(extracted.isValid());
        backend.extract(archive, extracted.path());
        await(backend, true);
        QFile restored(extracted.filePath(newName + QStringLiteral("/nested/файл.txt")));
        QVERIFY(restored.open(QIODevice::ReadOnly));
        QCOMPARE(restored.readAll(), QByteArray("content"));
    }
    void renameImplicitZipFolderAndRejectCollision() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString oldFolder = QStringLiteral("старая папка");
        QVERIFY(QDir().mkpath(temp.filePath(oldFolder + "/sub")));
        writeFile(temp.filePath(oldFolder + QStringLiteral("/sub/файл.txt")), "implicit");
        writeFile(temp.filePath(QStringLiteral("занято.txt")), "keep");
        const QString archive = temp.filePath("implicit.zip");
        QProcess process;
        process.setWorkingDirectory(temp.path());
        process.start("7zz", {"a", "-tzip", archive, oldFolder + QStringLiteral("/sub/файл.txt"),
                              QStringLiteral("занято.txt")});
        QVERIFY(process.waitForFinished(30000));
        QCOMPARE(process.exitCode(), 0);
        ArchiveBackend backend;
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(archive);
        await(backend, true);
        QCOMPARE(entries.size(), 2);
        QSignalSpy conflict(&backend, &ArchiveBackend::finished);
        backend.renameEntry(archive, oldFolder, QStringLiteral("занято.txt"));
        QCOMPARE(conflict.size(), 1);
        QVERIFY(!conflict.takeFirst().at(0).toBool());
        backend.renameEntry(archive, oldFolder, QStringLiteral("новая папка"));
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        QStringList paths;
        for (const ArchiveEntry &entry : entries)
            paths
                << QDir::fromNativeSeparators(entry.path).normalized(QString::NormalizationForm_C);
        QVERIFY(paths.contains(QStringLiteral("новая папка/sub/файл.txt")));
        QVERIFY(paths.contains(QStringLiteral("занято.txt")));
    }
    void archiveMutation() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString first = temp.filePath(QStringLiteral("старое имя.txt"));
        const QString second = temp.filePath(QStringLiteral("другой файл.txt"));
        const QString archive = temp.filePath("mutate." + format);
        writeFile(first, "one");
        writeFile(second, "two");
        ArchiveBackend backend;
        backend.create(archive, {first, second});
        await(backend, true);
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(archive);
        await(backend, true);
        QCOMPARE(entries.size(), 2);
        const QString before = QStringLiteral("старое имя.txt");
        const QString after = QStringLiteral("новое имя.txt");
        backend.renameEntry(archive, before, after);
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        QStringList paths;
        for (const ArchiveEntry &entry : entries)
            paths << QDir::fromNativeSeparators(entry.path);
        QVERIFY(paths.contains(after));
        QVERIFY(!paths.contains(before));
        QSignalSpy collision(&backend, &ArchiveBackend::finished);
        backend.renameEntry(archive, after, QStringLiteral("другой файл.txt"));
        QCOMPARE(collision.size(), 1);
        QVERIFY(!collision.takeFirst().at(0).toBool());
        backend.remove(archive, {after});
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.first().path.normalized(QString::NormalizationForm_C),
                 QStringLiteral("другой файл.txt"));
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
    void archiveMutationFailureAndCancellation() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath("original.txt");
        const QString archive = temp.filePath("existing.zip");
        writeFile(source, "original");
        ArchiveBackend real;
        real.create(archive, {source});
        await(real, true);
        QFile saved(archive);
        QVERIFY(saved.open(QIODevice::ReadOnly));
        const QByteArray before = saved.readAll();
        saved.close();

        const QString vanishing = temp.filePath("vanishing-7zz");
        QVERIFY(QFile::copy(QCoreApplication::applicationFilePath(), vanishing));
        qputenv("SEVENZIP_TEST_HELPER", "listing");
        qputenv("SEVENZIP_TEST_LISTING", "Path = original.txt\nSize = 8\n\n");
        ArchiveBackend missing(nullptr, vanishing);
        missing.list(archive);
        await(missing, true);
        QVERIFY(QFile::remove(vanishing));
        QSignalSpy launch(&missing, &ArchiveBackend::finished);
        missing.remove(archive, {"original.txt"});
        QTRY_VERIFY_WITH_TIMEOUT(!launch.isEmpty(), 3000);
        QVERIFY(!launch.takeFirst().at(0).toBool());
        QVERIFY(!missing.busy());

        ArchiveBackend helper(nullptr, QCoreApplication::applicationFilePath());
        helper.list(archive);
        await(helper, true);
        qunsetenv("SEVENZIP_TEST_LISTING");
        qputenv("SEVENZIP_TEST_HELPER", "createpartial");
        QSignalSpy output(&helper, &ArchiveBackend::outputReceived);
        QSignalSpy result(&helper, &ArchiveBackend::finished);
        helper.renameEntry(archive, "original.txt", "renamed.txt");
        QTRY_VERIFY_WITH_TIMEOUT(!output.isEmpty(), 3000);
        helper.cancel();
        QVERIFY(result.wait(30000));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(!result.takeFirst().at(0).toBool());
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), before);
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
    void archiveFolderDeletion_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QStringLiteral("7z");
        QTest::newRow("zip") << QStringLiteral("zip");
    }
    void archiveFolderDeletion() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString folder = temp.filePath(QStringLiteral("папка с пробелом"));
        QVERIFY(QDir().mkpath(folder));
        writeFile(folder + QStringLiteral("/внутри.txt"), "data");
        const QString archive = temp.filePath("folder." + format);
        ArchiveBackend backend;
        backend.create(archive, {folder});
        await(backend, true);
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(archive);
        await(backend, true);
        QStringList paths;
        for (const ArchiveEntry &entry : entries)
            paths << QDir::fromNativeSeparators(entry.path);
        QVERIFY(paths.size() >= 2);
        QString oldFile;
        for (const QString &path : paths) {
            if (path.endsWith(QStringLiteral("/внутри.txt")))
                oldFile = path;
        }
        QVERIFY(!oldFile.isEmpty());
        backend.renameEntry(archive, oldFile,
                            oldFile.left(oldFile.lastIndexOf('/') + 1) +
                                QStringLiteral("новое имя.txt"));
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        paths.clear();
        for (const ArchiveEntry &entry : entries)
            paths << QDir::fromNativeSeparators(entry.path);
        QVERIFY(paths.contains(oldFile.left(oldFile.lastIndexOf('/') + 1) +
                               QStringLiteral("новое имя.txt")));
        backend.remove(archive, paths);
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        QVERIFY(entries.isEmpty());
    }
    void addToExisting() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString original = temp.filePath(QStringLiteral("исходный.txt"));
        writeFile(original, "keep");
        const QString archive = temp.filePath("existing." + format);
        ArchiveBackend backend;
        backend.create(archive, {original});
        await(backend, true);
        backend.list(archive);
        await(backend, true);

        const QString folder = temp.filePath(QStringLiteral("папка Unicode"));
        QVERIFY(QDir().mkpath(folder + "/nested"));
        const QString large = folder + QStringLiteral("/nested/большой файл.bin");
        const QByteArray content(5 * 1024 * 1024, 'q');
        writeFile(large, content);
        backend.add(archive, {folder});
        await(backend, true);
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(archive);
        await(backend, true);
        QStringList names;
        for (const ArchiveEntry &entry : entries)
            names
                << QDir::fromNativeSeparators(entry.path).normalized(QString::NormalizationForm_C);
        QVERIFY(names.contains(QStringLiteral("исходный.txt")));
        QVERIFY(names.contains(QStringLiteral("папка Unicode/nested/большой файл.bin")));
        const QString output = temp.filePath("out");
        QVERIFY(QDir().mkpath(output));
        backend.extract(archive, output);
        await(backend, true);
        QFile restored(output + QStringLiteral("/папка Unicode/nested/большой файл.bin"));
        QVERIFY(restored.open(QIODevice::ReadOnly));
        QCOMPARE(restored.readAll(), content);

        const QByteArray before = [&] {
            QFile file(archive);
            if (!file.open(QIODevice::ReadOnly))
                return QByteArray();
            return file.readAll();
        }();
        QSignalSpy conflict(&backend, &ArchiveBackend::finished);
        backend.add(archive, {original});
        QCOMPARE(conflict.size(), 1);
        QVERIFY(!conflict.takeFirst().at(0).toBool());
        QFile unchanged(archive);
        QVERIFY(unchanged.open(QIODevice::ReadOnly));
        QCOMPARE(unchanged.readAll(), before);
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
    void addFailureAndCancellation() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString original = temp.filePath("original.txt");
        const QString next = temp.filePath("next.txt");
        writeFile(original, "original");
        writeFile(next, "next");
        const QString archive = temp.filePath("existing.zip");
        ArchiveBackend backend;
        backend.create(archive, {original});
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        QFile saved(archive);
        QVERIFY(saved.open(QIODevice::ReadOnly));
        const QByteArray before = saved.readAll();
        saved.close();

        ArchiveBackend missing(nullptr, "/definitely/missing/7zz");
        QSignalSpy missingList(&missing, &ArchiveBackend::finished);
        missing.list(archive);
        QTRY_VERIFY_WITH_TIMEOUT(!missingList.isEmpty(), 3000);
        QSignalSpy noListing(&missing, &ArchiveBackend::finished);
        missing.add(archive, {next});
        QCOMPARE(noListing.size(), 1);
        QVERIFY(!noListing.takeFirst().at(0).toBool());

        const QString vanishingProgram = temp.filePath("vanishing-7zz");
        QVERIFY(QFile::copy(QCoreApplication::applicationFilePath(), vanishingProgram));
        qputenv("SEVENZIP_TEST_HELPER", "listing");
        qputenv("SEVENZIP_TEST_LISTING", "Path = original.txt\nSize = 8\n\n");
        ArchiveBackend launchFailure(nullptr, vanishingProgram);
        launchFailure.list(archive);
        await(launchFailure, true);
        QVERIFY(QFile::remove(vanishingProgram));
        QSignalSpy launchResult(&launchFailure, &ArchiveBackend::finished);
        launchFailure.add(archive, {next});
        QTRY_VERIFY_WITH_TIMEOUT(!launchResult.isEmpty(), 3000);
        QVERIFY(!launchResult.takeFirst().at(0).toBool());
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());

        ArchiveBackend helper(nullptr, QCoreApplication::applicationFilePath());
        helper.list(archive);
        await(helper, true);
        qunsetenv("SEVENZIP_TEST_LISTING");
        qputenv("SEVENZIP_TEST_HELPER", "createfail");
        helper.add(archive, {next});
        await(helper, false);
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), before);
        saved.close();

        qputenv("SEVENZIP_TEST_HELPER", "createpartial");
        QSignalSpy output(&helper, &ArchiveBackend::outputReceived);
        QSignalSpy result(&helper, &ArchiveBackend::finished);
        helper.add(archive, {next});
        QTRY_VERIFY_WITH_TIMEOUT(!output.isEmpty(), 3000);
        helper.cancel();
        QVERIFY(result.wait(30000));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(!result.takeFirst().at(0).toBool());
        QVERIFY(saved.open(QIODevice::ReadOnly));
        QCOMPARE(saved.readAll(), before);
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
    void addToEmptyArchive() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString archive = temp.filePath("empty.zip");
        writeFile(archive, QByteArray::fromHex("504b0506000000000000000000000000000000000000"));
        const QString source = temp.filePath(QStringLiteral("после пустоты.txt"));
        writeFile(source, "added");
        ArchiveBackend backend;
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(archive);
        await(backend, true);
        QVERIFY(entries.isEmpty());
        backend.add(archive, {source});
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.first().path.normalized(QString::NormalizationForm_C),
                 QStringLiteral("после пустоты.txt"));
    }
    void cancelledPartialCreation() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath("source.txt");
        writeFile(source, "data");
        const QString target = temp.filePath("cancel.zip");
        qputenv("SEVENZIP_TEST_HELPER", "createpartial");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy output(&backend, &ArchiveBackend::outputReceived);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(target, {source});
        QTRY_VERIFY_WITH_TIMEOUT(!output.isEmpty(), 3000);
        backend.cancel();
        QVERIFY(finished.wait(30000));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(!finished.takeFirst().at(0).toBool());
        QVERIFY(!QFileInfo::exists(target));
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
        qputenv("SEVENZIP_TEST_HELPER", "createfail");
        QSignalSpy failed(&backend, &ArchiveBackend::finished);
        backend.create(target, {source});
        QVERIFY(failed.wait(30000));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(!failed.takeFirst().at(0).toBool());
        QVERIFY(!QFileInfo::exists(target));
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
    void fragmentedOutputAndError() {
        qputenv("SEVENZIP_TEST_HELPER", "fragment");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy progress(&backend, &ArchiveBackend::progress);
        QSignalSpy output(&backend, &ArchiveBackend::outputReceived);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list("fake.7z");
        QVERIFY(finished.wait(30000));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(!finished.takeFirst().at(0).toBool());
        QVERIFY(!backend.busy());
        QCOMPARE(progress.size(), 1);
        QCOMPARE(progress.at(0).at(0).toInt(), 42);
        QCOMPARE(progress.at(0).at(1).toString(), QStringLiteral("папка/имя с пробелом.txt"));
        QCOMPARE(output.size(), 2);
        QCOMPARE(output.at(1).at(0).toString(), QStringLiteral("ERROR: incomplete tail"));
    }
    void largeListingStaysResponsive() {
        qputenv("SEVENZIP_TEST_HELPER", "large");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy entries(&backend, &ArchiveBackend::entriesReady);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        bool timerRan = false;
        backend.list("large.7z");
        QTimer::singleShot(10, &backend, [&] { timerRan = true; });
        QVERIFY(finished.wait(30000));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(timerRan);
        QVERIFY(finished.takeFirst().at(0).toBool());
        QCOMPARE(entries.size(), 1);
        QCOMPARE(qvariant_cast<QList<ArchiveEntry>>(entries.at(0).at(0)).size(), 3000);
    }
    void cancelledPartialOutput() {
        qputenv("SEVENZIP_TEST_HELPER", "cancel");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy output(&backend, &ArchiveBackend::outputReceived);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list("cancel.7z");
        QTRY_VERIFY_WITH_TIMEOUT(!output.isEmpty(), 3000);
        backend.cancel();
        QVERIFY(finished.wait(30000));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(!finished.takeFirst().at(0).toBool());
        QVERIFY(!backend.busy());
        QVERIFY(output.at(0).at(0).toString().contains("started"));
    }
};

int main(int argc, char **argv) {
    ArchiveMutationTests tests;
    return backendTestMain(argc, argv, &tests);
}
#include "archive_mutation_tests.moc"

#include "CPP/7zip/UI/Common/ZipRegistry.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "Common/StringConvert.h"
#include "archive_backend_test_support.h"
#include "dot_path_cpio_fixture.h"
#include "qtmemorylockservices.h"
#include <QScopeGuard>

class ArchiveBackendTests : public QObject, protected ArchiveBackendTestSupport {
    Q_OBJECT

  private slots:
    void rejectedRequestDoesNotReusePreviousResult_data() {
        QTest::addColumn<QString>("operation");
        QTest::addColumn<bool>("cancelled");
        for (const char *operation :
             {"create", "extract", "test", "add", "remove", "rename", "replace"})
            for (bool cancelled : {false, true})
                QTest::newRow(qPrintable(
                    QString("%1-after-%2").arg(operation, cancelled ? "cancel" : "success")))
                    << QString::fromLatin1(operation) << cancelled;
    }
    void rejectedRequestDoesNotReusePreviousResult() {
        QFETCH(QString, operation);
        QFETCH(bool, cancelled);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString archive = fixture.filePath(QStringLiteral("архив с пробелом.zip"));
        writeFile(archive, "unchanged archive fixture");
        qputenv("SEVENZIP_TEST_HELPER", "listing");
        qputenv("SEVENZIP_TEST_LISTING", "Type = zip\n----------\nPath = a.txt\nSize = 1\n\n");
        const auto restore = qScopeGuard([] {
            qunsetenv("SEVENZIP_TEST_HELPER");
            qunsetenv("SEVENZIP_TEST_LISTING");
        });
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        backend.list(archive);
        await(backend, true);
        QCOMPARE(backend.lastResult(), S_OK);
        if (cancelled) {
            const auto cancellation = connect(&backend, &ArchiveBackend::engineStarted, &backend,
                                              &ArchiveBackend::cancel);
            backend.testSourceArchive(archive, {});
            await(backend, false);
            disconnect(cancellation);
            QCOMPARE(backend.lastResult(), E_ABORT);
        }
        QSignalSpy refused(&backend, &ArchiveBackend::finished);
        if (operation == "create")
            backend.create(archive, {archive}); // Refuse an occupied output.
        else if (operation == "extract")
            backend.extract(archive, fixture.filePath("output"), {"missing.txt"});
        else if (operation == "test")
            backend.test(fixture.filePath("unlisted.zip"));
        else if (operation == "add")
            backend.add(archive, {});
        else if (operation == "remove")
            backend.remove(archive, {"missing.txt"});
        else if (operation == "rename")
            backend.renameEntry(archive, "missing.txt", "renamed.txt");
        else
            backend.replaceEntry(archive, "missing.txt", fixture.path());
        QCOMPARE(refused.size(), 1);
        QVERIFY(!refused[0][0].toBool());
        QVERIFY(FAILED(backend.lastResult()));
        QVERIFY(backend.lastResult() != E_ABORT);
        QVERIFY(!backend.busy());
        QFile original(archive);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), QByteArray("unchanged archive fixture"));
    }
    void nativePathListingFailure_data() {
        QTest::addColumn<QString>("error");
        QTest::addColumn<HRESULT>("result");
        QTest::newRow("not-archive") << "ERROR: %1 : Cannot open the file as archive\n" << S_FALSE;
        QTest::newRow("wrong-password")
            << "ERROR: %1 : Cannot open encrypted archive. Wrong password?\n"
            << S_FALSE;
        QTest::newRow("specific-type")
            << "ERROR: %1 : %1\nOpen ERROR: Cannot open the file as [zip] archive\n"
            << S_FALSE;
        QTest::newRow("io-failure") << "ERROR: %1 : opening : fixture I/O error\n" << E_FAIL;
    }
    void nativePathListingFailure() {
        QFETCH(QString, error);
        QFETCH(HRESULT, result);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString archive = fixture.filePath(QStringLiteral("не архив 空 with spaces.bin"));
        writeFile(archive, "ordinary bytes");
        qputenv("SEVENZIP_TEST_HELPER", "listing-failure");
        qputenv("SEVENZIP_TEST_LISTING", error.arg(QDir::toNativeSeparators(archive)).toUtf8());
        const auto restore = qScopeGuard([] {
            qunsetenv("SEVENZIP_TEST_HELPER");
            qunsetenv("SEVENZIP_TEST_LISTING");
        });
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        backend.list(archive);
        await(backend, false);
        QCOMPARE(backend.lastResult(), result);
        QFile source(archive);
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), QByteArray("ordinary bytes"));
    }
    void windowsPasswordPromptThroughProcessPipe() {
        QTemporaryDir fixture;
        const QString archive = fixture.filePath("fixture.7z");
        writeFile(archive, "local prompt fixture");
        qputenv("SEVENZIP_TEST_HELPER", "windows-password-prompt");
        const auto restore = qScopeGuard([] { qunsetenv("SEVENZIP_TEST_HELPER"); });
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy prompt(&backend, &ArchiveBackend::passwordRequired);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        connect(&backend, &ArchiveBackend::passwordRequired, &backend,
                [&] { backend.providePassword("fixture-only-password"); });
        backend.list(archive);
        QTRY_VERIFY_WITH_TIMEOUT(!prompt.isEmpty(), 3000);
        QTRY_VERIFY_WITH_TIMEOUT(!finished.isEmpty(), 3000);
        QCOMPARE(prompt.size(), 1);
        QVERIFY(finished.first().first().toBool());
    }
    void alternateStreamOutputValidation_data() {
        QTest::addColumn<QString>("base");
        QTest::addColumn<QString>("name");
        QTest::addColumn<bool>("declared");
        QTest::addColumn<bool>("accepted");
        QTest::newRow("real-stream") << "folder/file.bin" << "stream δ" << true << true;
        QTest::newRow("native-separators") << "folder\\file.bin" << "stream δ" << true << true;
        QTest::newRow("ordinary-colon") << "folder/file.bin" << "stream" << false << false;
        QTest::newRow("unsafe-base") << "../file.bin" << "stream" << true << false;
        QTest::newRow("unsafe-stream") << "folder/file.bin" << "../escape" << true << false;
        QTest::newRow("nested-stream") << "folder/file.bin" << "dir/file" << true << false;
        QTest::newRow("nested-ads") << "folder/file.bin" << "stream:extra" << true << false;
    }
    void alternateStreamOutputValidation() {
        QFETCH(QString, base);
        QFETCH(QString, name);
        QFETCH(bool, declared);
        QFETCH(bool, accepted);
        QTemporaryDir fixture;
        const QString entry = base + ':' + name;
        qputenv("SEVENZIP_TEST_HELPER", "listing");
        qputenv("SEVENZIP_TEST_LISTING", ("Path = " + entry + "\nSize = 1\nAlternate Stream = " +
                                          (declared ? "+" : "-") + "\n\n")
                                             .toUtf8());
        const auto restore = qScopeGuard([] {
            qunsetenv("SEVENZIP_TEST_HELPER");
            qunsetenv("SEVENZIP_TEST_LISTING");
        });
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        const QString archive = fixture.filePath("fixture.wim");
        writeFile(archive, "local listing fixture");
        backend.list(archive);
        await(backend, true);
        ExtractionOptions options;
        options.pathMode = ExtractionOptions::PathMode::CurrentPaths;
        options.currentFolder = base + ':';
        options.currentFolder.replace('\\', '/');
        QString selected = entry;
        selected.replace('\\', '/');
        QString error;
        QCOMPARE(
            backend.validateExtraction(fixture.filePath("output"), {selected}, options, &error),
            accepted);
        if (accepted) {
            QVERIFY(QDir().mkpath(fixture.filePath("output/" + name)));
            QVERIFY(!backend.validateExtraction(fixture.filePath("output"), {selected}, options,
                                                &error));
        }
    }
    void extractionValidationLanguagePreservesRawPath() {
        QTemporaryDir fixture;
        const QString languageSettings = fixture.filePath("language.ini");
        const QString entry = QStringLiteral("../путь с пробелом 密碼.txt");
        qputenv("SEVENZIP_TEST_HELPER", "listing");
        qputenv("SEVENZIP_TEST_LISTING", ("Path = " + entry + "\nSize = 1\n\n").toUtf8());
        const auto restore = qScopeGuard([&] {
            qunsetenv("SEVENZIP_TEST_LISTING");
            qunsetenv("SEVENZIP_TEST_HELPER");
            SaveRegLang(UString("-"), languageSettings);
            ReloadLang(languageSettings);
        });
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        const QString archive = fixture.filePath("fixture.zip");
        writeFile(archive, "local listing fixture");
        backend.list(archive);
        await(backend, true);
        for (const char *language : {"-", "ru", "-"}) {
            SaveRegLang(UString(language), languageSettings);
            ReloadLang(languageSettings);
            qputenv("SEVENZIP_TEST_HELPER", "listing");
            QSignalSpy listed(&backend, &ArchiveBackend::finished);
            backend.list(archive);
            QVERIFY(listed.wait(30000));
            const auto opened = listed.takeFirst();
            QVERIFY(opened.at(0).toBool());
            QCOMPARE(opened.at(1).toString(), QString(language) == "ru"
                                                  ? QStringLiteral("Архив открыт.")
                                                  : QString("Archive opened."));
            QSignalSpy result(&backend, &ArchiveBackend::finished);
            backend.extract(archive, QString());
            QCOMPARE(result.size(), 1);
            const auto empty = result.takeFirst();
            QVERIFY(!empty.at(0).toBool());
            QCOMPARE(empty.at(1).toString(), QString(language) == "ru"
                                                 ? QStringLiteral("Укажите папку назначения.")
                                                 : QString("Specify a destination folder."));
            backend.extract(archive, fixture.path());
            QCOMPARE(result.size(), 1);
            const auto unsafe = result.takeFirst();
            QVERIFY(!unsafe.at(0).toBool());
            QCOMPARE(unsafe.at(1).toString(),
                     (QString(language) == "ru"
                          ? QStringLiteral("Архив содержит небезопасный путь или ссылку: ")
                          : QString("Archive contains an unsafe path or link: ")) +
                         entry);
            QVERIFY(!backend.busy());
            QVERIFY(!QFileInfo::exists(fixture.filePath("путь с пробелом 密碼.txt")));
            qputenv("SEVENZIP_TEST_HELPER", "fragment");
            backend.list(archive);
            QVERIFY(result.wait(30000));
            const auto failed = result.takeFirst();
            QVERIFY(!failed.at(0).toBool());
            QVERIFY(failed.at(1).toString().startsWith(
                QString(language) == "ru" ? QStringLiteral("7zz завершился с кодом 2. ")
                                          : QString("7zz exited with code 2. ")));
            QVERIFY(failed.at(1).toString().endsWith("ERROR: incomplete tail"));
        }
    }
    void creationUsesWorkingDirectoryAndRejectsUnavailablePath() {
        QTemporaryDir fixture;
        const QString settings = fixture.filePath("working.ini");
        const QByteArray original = qgetenv("SEVENZIP_SETTINGS_PATH");
        qputenv("SEVENZIP_SETTINGS_PATH", settings.toUtf8());
        const auto restore = qScopeGuard([&] {
            if (original.isNull())
                qunsetenv("SEVENZIP_SETTINGS_PATH");
            else
                qputenv("SEVENZIP_SETTINGS_PATH", original);
        });
        const QString source = fixture.filePath("источник с пробелом.txt");
        writeFile(source, "contents");
        const QString work = fixture.filePath("рабочая папка");
        NWorkDir::CInfo info;
        info.SetDefault();
        info.Mode = NWorkDir::NMode::kSpecified;
        info.ForRemovableOnly = false;
        info.Path = us2fs(work.toStdWString().c_str());
        info.Save(settings);
        QVERIFY(!QFileInfo::exists(work));
        ArchiveBackend backend;
        const QString archive = fixture.filePath("created.zip");
        backend.create(archive, {source});
        await(backend, true);
        QVERIFY(QFileInfo::exists(archive));
        QVERIFY(QFileInfo(work).isDir());
        QVERIFY(QDir(work).entryList(QDir::Files | QDir::Hidden).isEmpty());
        const QString blocked = fixture.filePath("occupied-file");
        writeFile(blocked, "occupied");
        info.Path = us2fs((blocked + "/child").toStdWString().c_str());
        info.Save(settings);
        const QString failed = fixture.filePath("failed.zip");
        backend.create(failed, {source});
        await(backend, false);
        QVERIFY(!backend.busy());
        QVERIFY(!QFileInfo::exists(failed));
        QVERIFY(
            QDir(fixture.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
        QFile unchanged(source);
        QVERIFY(unchanged.open(QIODevice::ReadOnly));
        QCOMPARE(unchanged.readAll(), QByteArray("contents"));
    }
    void largePagesSettingReachesEngineOnlyWithPlatformCapability() {
        QTemporaryDir fixture;
        const QString settings = fixture.filePath("memory.ini");
        const QByteArray originalSettings = qgetenv("SEVENZIP_SETTINGS_PATH");
        qputenv("SEVENZIP_SETTINGS_PATH", settings.toUtf8());
        qputenv("SEVENZIP_TEST_HELPER", "memoryoption");
        const auto restore = qScopeGuard([&] {
            if (originalSettings.isNull())
                qunsetenv("SEVENZIP_SETTINGS_PATH");
            else
                qputenv("SEVENZIP_SETTINGS_PATH", originalSettings);
            qunsetenv("SEVENZIP_TEST_HELPER");
        });
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QString requested;
        connect(&backend, &ArchiveBackend::infoReady, this, [&](const ArchiveInfo &info) {
            requested = info.properties.value("LargePagesRequested");
        });
        for (bool enable : {false, true, false}) {
            SaveLockMemoryEnable(enable, settings);
            QCOMPARE(ReadLockMemoryEnable(settings), enable);
            backend.list(fixture.filePath("fixture.zip"));
            await(backend, true);
            const QtMemoryLockServices memoryLock;
            QCOMPARE(requested, QString(IsLargePageSupported() && enable &&
                                                memoryLock.Get_LargePages_RiskLevel() == 0
                                            ? "1"
                                            : "0"));
        }
    }
    void commandsUseTheirListedArchiveAcrossTwoPanels() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString firstSource = temp.filePath(QStringLiteral("δ first file.txt"));
        const QString secondSource = temp.filePath(QStringLiteral("second file.txt"));
        const QString addedSource = temp.filePath(QStringLiteral("added file.txt"));
        const QString firstArchive = temp.filePath(QStringLiteral("first.zip"));
        const QString secondArchive = temp.filePath(QStringLiteral("second.zip"));
        const QString destination = temp.filePath(QStringLiteral("extracted"));
        writeFile(firstSource, "first contents");
        writeFile(secondSource, "second contents");
        writeFile(addedSource, "added contents");
        QVERIFY(QDir().mkpath(destination));
        ArchiveBackend backend;
        backend.create(firstArchive, {firstSource});
        await(backend, true);
        backend.create(secondArchive, {secondSource});
        await(backend, true);
        backend.list(firstArchive);
        await(backend, true);
        backend.list(secondArchive);
        await(backend, true);

        backend.test(firstArchive);
        await(backend, true);
        backend.extract(firstArchive, destination, {QStringLiteral("δ first file.txt")});
        await(backend, true);
        QFile extracted(QDir(destination).filePath(QStringLiteral("δ first file.txt")));
        QVERIFY(extracted.open(QIODevice::ReadOnly));
        QCOMPARE(extracted.readAll(), QByteArray("first contents"));

        backend.add(firstArchive, {addedSource});
        await(backend, true);
        QSignalSpy stale(&backend, &ArchiveBackend::finished);
        backend.test(firstArchive);
        QCOMPARE(stale.size(), 1);
        QVERIFY(!stale.takeFirst().at(0).toBool());
        backend.test(secondArchive);
        await(backend, true);
    }
    void roundTrip_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QString("7z");
        QTest::newRow("zip") << QString("zip");
    }
    void roundTrip() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.path() + "/имя с пробелом";
        QVERIFY(QDir().mkpath(source + "/вложено"));
        writeFile(source + "/обычный файл.txt", "hello");
        writeFile(source + "/вложено/δ space.txt", "nested");
        ArchiveBackend backend;
        const QString archive = temp.path() + "/archive." + format;
        backend.create(archive, {source});
        await(backend, true);
        QVERIFY(QFileInfo::exists(archive));

        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(archive);
        await(backend, true);
        QVERIFY(entries.size() >= 4);
        QString nested;
        for (const auto &entry : entries) {
            if (entry.path.endsWith("δ space.txt")) {
                nested = entry.path;
                QCOMPARE(entry.size, qint64(6));
                QVERIFY(entry.packedSize >= 0);
                QVERIFY(!entry.modified.isEmpty());
            }
        }
        QVERIFY(!nested.isEmpty());

        const QString selected = temp.path() + "/selected";
        QVERIFY(QDir().mkpath(selected));
        backend.extract(archive, selected, {nested});
        await(backend, true);
        QVERIFY(QFileInfo::exists(selected + "/" + nested));
        QVERIFY(!QFileInfo::exists(selected + "/имя с пробелом/обычный файл.txt"));

        const QString all = temp.path() + "/all";
        QVERIFY(QDir().mkpath(all));
        backend.extract(archive, all);
        await(backend, true);
        QFile file(all + "/имя с пробелом/обычный файл.txt");
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("hello"));
    }
    void encryptedRoundTrip_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z-hidden-names") << QStringLiteral("7z");
        QTest::newRow("zip-aes256") << QStringLiteral("zip");
    }
    void encryptedRoundTrip() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString fixturePassword = format == "7z" ? QStringLiteral("test-only-δ-пароль")
                                                       : QStringLiteral("test-only-password");
        const QString source = temp.filePath(QStringLiteral("δ file with spaces.txt"));
        const QString archive = temp.filePath(QStringLiteral("encrypted.") + format);
        const QString destination = temp.filePath(QStringLiteral("extracted files"));
        writeFile(source, "protected content");
        QVERIFY(QDir().mkpath(destination));
        ArchiveBackend backend;
        QSignalSpy output(&backend, &ArchiveBackend::outputReceived);
        if (format == "zip") {
            QSignalSpy rejected(&backend, &ArchiveBackend::finished);
            backend.create(archive, {source}, QStringLiteral("test-only-δ-пароль"));
            QCOMPARE(rejected.size(), 1);
            QVERIFY(!rejected.takeFirst().at(0).toBool());
            QVERIFY(!QFileInfo::exists(archive));
        }
        backend.create(archive, {source}, fixturePassword, format == "7z");
        await(backend, true);
        QVERIFY(QFileInfo::exists(archive));
        QSignalSpy info(&backend, &ArchiveBackend::infoReady);
        if (format == "7z") {
            QSignalSpy wrong(&backend, &ArchiveBackend::finished);
            backend.list(archive, QStringLiteral("incorrect-test-password"));
            QVERIFY(wrong.wait(30000));
            QVERIFY(!wrong.takeFirst().at(0).toBool());
            QVERIFY(info.isEmpty());
        }
        QSignalSpy prompt(&backend, &ArchiveBackend::passwordRequired);
        connect(&backend, &ArchiveBackend::passwordRequired, &backend,
                [&backend, &fixturePassword] { backend.providePassword(fixturePassword); });
        backend.list(archive);
        await(backend, true);
        if (format == "7z")
            QCOMPARE(prompt.size(), 1);
        QCOMPARE(info.size(), 1);
        QVERIFY(qvariant_cast<ArchiveInfo>(info.takeFirst().at(0)).encrypted);
        QSignalSpy wrong(&backend, &ArchiveBackend::finished);
        backend.test(archive, QStringLiteral("incorrect-test-password"));
        QVERIFY(wrong.wait(30000));
        const auto failure = wrong.takeFirst();
        QVERIFY(!failure.at(0).toBool());
        QVERIFY(failure.at(1).toString().contains(QStringLiteral("Wrong password")));
        QVERIFY(!failure.at(1).toString().contains(fixturePassword));
        backend.test(archive, fixturePassword);
        await(backend, true);
        QSignalSpy extractFailure(&backend, &ArchiveBackend::finished);
        backend.extract(archive, destination, {}, QStringLiteral("incorrect-test-password"));
        QVERIFY(extractFailure.wait(30000));
        QVERIFY(!extractFailure.takeFirst().at(0).toBool());
        QCOMPARE(QDir(destination).entryList(QDir::Files | QDir::NoDotAndDotDot).size(), 0);
        backend.extract(archive, destination, {}, fixturePassword);
        await(backend, true);
        QFile extracted(destination + "/δ file with spaces.txt");
        QVERIFY(extracted.open(QIODevice::ReadOnly));
        QCOMPARE(extracted.readAll(), QByteArray("protected content"));
        for (const auto &line : output)
            QVERIFY(!line.at(0).toString().contains(fixturePassword));
    }
    void encryptedCreationLaunchFailure() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("δ source.txt"));
        const QString archive = temp.filePath("encrypted.7z");
        const QString password = QStringLiteral("disposable-δ-password");
        writeFile(source, "content");
        ArchiveBackend missing(nullptr, "/definitely/missing/7zz");
        QSignalSpy result(&missing, &ArchiveBackend::finished);
        missing.create(archive, {source}, password, true);
        QTRY_VERIFY_WITH_TIMEOUT(!result.isEmpty(), 3000);
        const auto failure = result.takeFirst();
        QVERIFY(!failure.at(0).toBool());
        QVERIFY(!failure.at(1).toString().contains(password));
        QVERIFY(!missing.busy());
        QVERIFY(!QFileInfo::exists(archive));
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
    void testAndArchiveInfo_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QStringLiteral("7z");
        QTest::newRow("zip") << QStringLiteral("zip");
    }
    void testAndArchiveInfo() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.path() + QStringLiteral("/δ space.txt");
        const QString archive = temp.path() + "/archive." + format;
        writeFile(source, "verified");
        ArchiveBackend backend;
        backend.create(archive, {source});
        await(backend, true);
        QSignalSpy info(&backend, &ArchiveBackend::infoReady);
        backend.list(archive);
        await(backend, true);
        QCOMPARE(info.size(), 1);
        const ArchiveInfo details = qvariant_cast<ArchiveInfo>(info.takeFirst().at(0));
        QCOMPARE(details.format.toLower(), format);
        QCOMPARE(details.physicalSize, QFileInfo(archive).size());
        QCOMPARE(details.files, 1);
        QCOMPARE(details.unpackedSize, qint64(8));
        QSignalSpy result(&backend, &ArchiveBackend::finished);
        backend.test(archive);
        QVERIFY(result.wait(30000));
        QVERIFY(result.takeFirst().at(0).toBool());
        QCOMPARE(QDir(temp.path()).entryList(QDir::Files | QDir::NoDotAndDotDot).size(), 2);
        QFile corrupt(archive);
        QVERIFY(corrupt.open(QIODevice::WriteOnly | QIODevice::Truncate));
        corrupt.write("broken");
        corrupt.close();
        backend.test(archive);
        QCOMPARE(result.size(), 1);
        QVERIFY(!result.takeFirst().at(0).toBool());
    }
    void selectedExtractionIgnoresUnselectedCollision() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString archive = temp.path() + "/fixture.zip";
        const QString destination = temp.path() + "/out";
        QVERIFY(QDir().mkpath(destination));
        writeFile(destination + "/occupied.txt", "keep");
        const QString source = temp.path() + "/source";
        QVERIFY(QDir().mkpath(source));
        writeFile(source + "/occupied.txt", "old");
        writeFile(source + "/chosen.txt", "yes");
        ArchiveBackend backend;
        backend.create(archive, {source + "/occupied.txt", source + "/chosen.txt"});
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        QSignalSpy result(&backend, &ArchiveBackend::finished);
        backend.extract(archive, destination, {"chosen.txt"});
        QVERIFY(result.wait(30000));
        QVERIFY(result.takeFirst().at(0).toBool());
        QFile existing(destination + "/occupied.txt");
        QVERIFY(existing.open(QIODevice::ReadOnly));
        QCOMPARE(existing.readAll(), QByteArray("keep"));
        QFile chosen(destination + "/chosen.txt");
        QVERIFY(chosen.open(QIODevice::ReadOnly));
        QCOMPARE(chosen.readAll(), QByteArray("yes"));
    }
    void testCancellation() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        qputenv("SEVENZIP_TEST_HELPER", "listing");
        qputenv("SEVENZIP_TEST_LISTING", "Path = δ space.txt\nSize = 1\n\n");
        writeFile(temp.path() + "/archive.7z", "test fixture");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        backend.list(temp.path() + "/archive.7z");
        await(backend, true);
        qunsetenv("SEVENZIP_TEST_LISTING");
        qputenv("SEVENZIP_TEST_HELPER", "cancel");
        QSignalSpy output(&backend, &ArchiveBackend::outputReceived);
        QSignalSpy result(&backend, &ArchiveBackend::finished);
        backend.test(temp.path() + "/archive.7z");
        QTRY_VERIFY_WITH_TIMEOUT(!output.isEmpty(), 3000);
        backend.cancel();
        QVERIFY(result.wait(30000));
        qunsetenv("SEVENZIP_TEST_HELPER");
        QVERIFY(!result.takeFirst().at(0).toBool());
        QVERIFY(!backend.busy());
    }
    void invalidArchive() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString archive = temp.path() + "/broken.7z";
        writeFile(archive, "this is not an archive");
        ArchiveBackend backend;
        QSignalSpy spy(&backend, &ArchiveBackend::finished);
        backend.list(archive);
        QVERIFY(spy.wait(30000));
        QVERIFY(!spy.takeFirst().at(0).toBool());
    }
    void missingExecutable() {
        ArchiveBackend backend(nullptr, "/no/such/7zz");
        QSignalSpy spy(&backend, &ArchiveBackend::finished);
        backend.list("/no/such/archive.7z");
        QTRY_VERIFY_WITH_TIMEOUT(!spy.isEmpty(), 30000);
        const auto result = spy.takeFirst();
        QVERIFY(!result.at(0).toBool());
        QVERIFY(result.at(1).toString().contains("7zz"));
    }
    void dotPathsInCpio_data() {
        QTest::addColumn<bool>("currentFolder");
        QTest::newRow("full-paths") << false;
        QTest::newRow("current-paths") << true;
    }
    void dotPathsInCpio() {
        QFETCH(bool, currentFolder);
        QTemporaryDir fixture;
        const QString archive = fixture.filePath("Payload.cpio");
        writeFile(archive, DotPathCpioFixture());
        ArchiveBackend backend;
        backend.list(archive);
        await(backend, true);
        const QString leaf = QStringLiteral("имя 空.txt");
        ExtractionOptions options;
        QStringList selected;
        if (currentFolder) {
            options.pathMode = ExtractionOptions::PathMode::CurrentPaths;
            options.currentFolder = "./ProxyBridge.app";
            selected << options.currentFolder + '/' + leaf;
        }
        const QString destination = fixture.filePath("output");
        backend.extract(archive, destination, selected, {}, options);
        await(backend, true);
        QFile extracted(QDir(destination).filePath("ProxyBridge.app/" + leaf));
        QVERIFY(extracted.open(QIODevice::ReadOnly));
        QCOMPARE(extracted.readAll(), QByteArray("payload bytes"));
    }
    void currentSourceFolderValidation_data() {
        QTest::addColumn<QString>("entry");
        QTest::addColumn<QString>("currentFolder");
        QTest::addColumn<QString>("sourceFolder");
        QTest::addColumn<bool>("accepted");
        QTest::newRow("bound-parent") << "../имя 空.txt" << ".." << ".." << true;
        QTest::newRow("bound-nested-parent")
            << "outer/../inside/имя 空.txt" << "outer/../inside" << "outer/../inside" << true;
        QTest::newRow("unbound-parent") << "../file.txt" << ".." << "" << false;
        QTest::newRow("different-binding") << "../file.txt" << ".." << "other" << false;
        QTest::newRow("remaining-traversal")
            << "../inside/../escape.txt" << "../inside" << "../inside" << false;
        QTest::newRow("prefix-boundary")
            << "../inside-other/file.txt" << "../inside" << "../inside" << false;
        QTest::newRow("reserved-output") << "../CON.txt" << ".." << ".." << false;
    }
    void currentSourceFolderValidation() {
        QFETCH(QString, entry);
        QFETCH(QString, currentFolder);
        QFETCH(QString, sourceFolder);
        QFETCH(bool, accepted);
        QTemporaryDir fixture;
        qputenv("SEVENZIP_TEST_HELPER", "listing");
        qputenv("SEVENZIP_TEST_LISTING", ("Path = " + entry + "\nSize = 1\n\n").toUtf8());
        const auto restore = qScopeGuard([] {
            qunsetenv("SEVENZIP_TEST_HELPER");
            qunsetenv("SEVENZIP_TEST_LISTING");
        });
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        backend.list(fixture.filePath("fixture.cpio"));
        await(backend, true);
        ExtractionOptions options;
        options.pathMode = ExtractionOptions::PathMode::CurrentPaths;
        options.currentFolder = currentFolder;
        QString error;
        const QString output = fixture.filePath("output");
        QCOMPARE(backend.validateExtraction(output, {entry}, options, &error, false, sourceFolder),
                 accepted);
        if (accepted) {
            options.pathMode = ExtractionOptions::PathMode::FullPaths;
            QVERIFY(
                !backend.validateExtraction(output, {entry}, options, &error, false, sourceFolder));
            options.pathMode = ExtractionOptions::PathMode::CurrentPaths;
            QVERIFY(QDir().mkpath(QDir(output).filePath(entry.section('/', -1))));
            QVERIFY(
                !backend.validateExtraction(output, {entry}, options, &error, false, sourceFolder));
        }
    }
    void unsafePath() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.path() + "/outside.txt";
        const QString archive = temp.path() + "/unsafe.zip";
        const QString destination = temp.path() + "/out";
        writeFile(source, "unsafe");
        QVERIFY(QDir().mkpath(destination));
        QProcess maker;
        maker.start("7zz", {"a", "-tzip", "-spf", archive, source});
        QVERIFY(maker.waitForFinished(30000));
        QCOMPARE(maker.exitCode(), 0);
        ArchiveBackend backend;
        backend.list(archive);
        await(backend, true);
        QSignalSpy spy(&backend, &ArchiveBackend::finished);
        backend.extract(archive, destination);
        QCOMPARE(spy.size(), 1);
        QVERIFY(!spy.takeFirst().at(0).toBool());
        QVERIFY(QDir(destination).entryList(QDir::Files | QDir::NoDotAndDotDot).isEmpty());
    }
    void unsafeListing_data() {
        QTest::addColumn<QByteArray>("listing");
        QTest::newRow("parent") << QByteArray("Path = ../escape.txt\nSize = 1\n\n");
        QTest::newRow("dot-parent") << QByteArray("Path = ./../escape.txt\nSize = 1\n\n");
        QTest::newRow("backslash parent")
            << QByteArray("Path = folder\\..\\escape.txt\nSize = 1\n\n");
        QTest::newRow("drive") << QByteArray("Path = C:\\escape.txt\nSize = 1\n\n");
        QTest::newRow("link") << QByteArray(
            "Path = link\nAttributes = A\nSymbolic Link = ../outside\n\n");
        QTest::newRow("link before attributes")
            << QByteArray("Path = link\nSymbolic Link = ../outside\nAttributes = A\n\n");
        QTest::newRow("hard link") << QByteArray("Path = link\nHard Link = target\n\n");
        QTest::newRow("duplicate")
            << QByteArray("Path = same.txt\nSize = 1\n\nPath = SAME.TXT\nSize = 1\n\n");
        QTest::newRow("dot-duplicate")
            << QByteArray("Path = ./same.txt\nSize = 1\n\nPath = same.txt\nSize = 1\n\n");
        QTest::newRow("dot-file-parent")
            << QByteArray("Path = ./parent\nSize = 1\n\nPath = parent/child\nSize = 1\n\n");
        QTest::newRow("unicode normalization")
            << (QStringLiteral("Path = café.txt\n\nPath = cafe\u0301.txt\n\n").toLocal8Bit());
        QTest::newRow("file parent")
            << QByteArray("Path = parent\nSize = 1\n\nPath = parent/child\nSize = 1\n\n");
        QTest::newRow("reserved") << QByteArray("Path = CON.txt\nSize = 1\n\n");
    }
    void unsafeListing() {
        QFETCH(QByteArray, listing);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        qputenv("SEVENZIP_TEST_HELPER", "listing");
        qputenv("SEVENZIP_TEST_LISTING", listing);
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        const QString archive = temp.path() + "/fixture.zip";
        backend.list(archive);
        await(backend, true);
        QSignalSpy spy(&backend, &ArchiveBackend::finished);
        backend.extract(archive, temp.path());
        QCOMPARE(spy.size(), 1);
        QVERIFY(!spy.takeFirst().at(0).toBool());
        QVERIFY(!backend.busy());
        qunsetenv("SEVENZIP_TEST_LISTING");
        qunsetenv("SEVENZIP_TEST_HELPER");
    }
    void unsafeDestination() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.path() + "/имя с пробелом.txt";
        const QString archive = temp.path() + "/safe.zip";
        writeFile(source, "original");
        ArchiveBackend backend;
        backend.create(archive, {source});
        await(backend, true);
        backend.list(archive);
        await(backend, true);

        const QString created = temp.path() + "/новая папка/ещё одна";
        backend.extract(archive, created);
        await(backend, true);
        QFile createdFile(created + "/имя с пробелом.txt");
        QVERIFY(createdFile.open(QIODevice::ReadOnly));
        QCOMPARE(createdFile.readAll(), QByteArray("original"));

        const QString occupied = temp.path() + "/occupied";
        QVERIFY(QDir().mkpath(occupied));
        writeFile(occupied + "/имя с пробелом.txt", "keep");
        QSignalSpy occupiedSpy(&backend, &ArchiveBackend::finished);
        backend.extract(archive, occupied);
        QCOMPARE(occupiedSpy.size(), 1);
        QVERIFY(!occupiedSpy.takeFirst().at(0).toBool());
        QFile existing(occupied + "/имя с пробелом.txt");
        QVERIFY(existing.open(QIODevice::ReadOnly));
        QCOMPARE(existing.readAll(), QByteArray("keep"));

        const QString linked = temp.path() + "/linked";
        if (QFile::link(occupied, linked)) {
            QSignalSpy linkedSpy(&backend, &ArchiveBackend::finished);
            backend.extract(archive, linked + "/новая папка");
            QCOMPARE(linkedSpy.size(), 1);
            QVERIFY(!linkedSpy.takeFirst().at(0).toBool());
            QVERIFY(!QFileInfo::exists(occupied + "/новая папка"));
        }
    }
    void cancellation_data() {
        QTest::addColumn<QString>("language");
        QTest::newRow("English") << QString("-");
        QTest::newRow("Russian") << QString("ru");
    }
    void cancellation() {
        QFETCH(QString, language);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString settings = temp.filePath("language.ini");
        SaveRegLang(UString(language.toStdWString().c_str()), settings);
        ReloadLang(settings);
        const auto restore = qScopeGuard([&] {
            SaveRegLang(UString("-"), settings);
            ReloadLang(settings);
        });
        const QString source = temp.path() + "/large.bin";
        writeFile(source, QByteArray(8 * 1024 * 1024, 'x'));
        ArchiveBackend backend;
        QSignalSpy spy(&backend, &ArchiveBackend::finished);
        backend.create(temp.path() + "/cancel.7z", {source});
        QVERIFY(backend.busy());
        backend.cancel();
        QVERIFY(spy.wait(30000));
        const auto result = spy.takeFirst();
        QVERIFY(!result.at(0).toBool());
        QCOMPARE(result.at(1).toString(), language == "ru" ? QStringLiteral("Операция отменена.")
                                                           : QString("Operation canceled."));
        QCOMPARE(backend.lastResult(), E_ABORT);
        QVERIFY(!backend.busy());
        QVERIFY(!QFileInfo::exists(temp.path() + "/cancel.7z"));
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
    void createFailureAndRetry() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("источник с пробелом.txt"));
        writeFile(source, "hello");
        const QString target = temp.filePath("new.zip");
        ArchiveBackend missing(nullptr, "/definitely/missing/7zz");
        QSignalSpy missingResult(&missing, &ArchiveBackend::finished);
        missing.create(target, {source});
        QTRY_VERIFY_WITH_TIMEOUT(!missingResult.isEmpty(), 3000);
        QVERIFY(!missingResult.takeFirst().at(0).toBool());
        QVERIFY(!QFileInfo::exists(target));
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());

        ArchiveBackend backend;
        backend.create(target, {source});
        await(backend, true);
        QVERIFY(QFileInfo::exists(target));
        QSignalSpy occupied(&backend, &ArchiveBackend::finished);
        backend.create(target, {source});
        QCOMPARE(occupied.size(), 1);
        QVERIFY(!occupied.takeFirst().at(0).toBool());
        QVERIFY(QFileInfo::exists(target));
    }
    void mixedSources() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("файл с пробелом.txt"));
        const QString folder = temp.filePath(QStringLiteral("папка Unicode"));
        QVERIFY(QDir().mkpath(folder));
        writeFile(source, "file");
        writeFile(folder + "/nested.txt", "nested");
        ArchiveBackend backend;
        const QString target = temp.filePath("mixed.7z");
        backend.create(target, {source, folder});
        await(backend, true);
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(target);
        await(backend, true);
        QStringList names;
        for (const ArchiveEntry &entry : entries)
            names
                << QDir::fromNativeSeparators(entry.path).normalized(QString::NormalizationForm_C);
        QVERIFY(names.contains(QStringLiteral("файл с пробелом.txt")));
        QVERIFY(names.contains(QStringLiteral("папка Unicode/nested.txt")));
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
};

int main(int argc, char **argv) {
    ArchiveBackendTests tests;
    return backendTestMain(argc, argv, &tests);
}
#include "archive_backend_tests.moc"

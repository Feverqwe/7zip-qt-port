#include "archivebackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QtTest>

#include <cstdio>

static void writeOutput(const QByteArray &text) {
    std::fwrite(text.constData(), 1, size_t(text.size()), stdout);
    std::fflush(stdout);
}

class ArchiveBackendTests : public QObject {
    Q_OBJECT

  private:
    static void writeFile(const QString &path, const QByteArray &data) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(data), data.size());
    }
    static void await(ArchiveBackend &backend, bool expected) {
        QSignalSpy spy(&backend, &ArchiveBackend::finished);
        if (spy.isEmpty())
            QVERIFY2(spy.wait(30000), "operation timed out");
        QCOMPARE(spy.takeFirst().at(0).toBool(), expected);
    }

  private slots:
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
        QVERIFY(failure.at(1).toString().contains(QStringLiteral("Неверный пароль")));
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
        QVERIFY(result.wait(30000));
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
        QVERIFY(spy.wait(30000));
        const auto result = spy.takeFirst();
        QVERIFY(!result.at(0).toBool());
        QVERIFY(result.at(1).toString().contains("7zz"));
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

        const QString missing = temp.path() + "/missing";
        QSignalSpy missingSpy(&backend, &ArchiveBackend::finished);
        backend.extract(archive, missing);
        QCOMPARE(missingSpy.size(), 1);
        QVERIFY(!missingSpy.takeFirst().at(0).toBool());
        QVERIFY(!QFileInfo::exists(missing));

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
            backend.extract(archive, linked);
            QCOMPARE(linkedSpy.size(), 1);
            QVERIFY(!linkedSpy.takeFirst().at(0).toBool());
        }
    }
    void cancellation() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
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
        QVERIFY(result.at(1).toString().contains("отменена"));
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
            names << entry.path.normalized(QString::NormalizationForm_C);
        QVERIFY(names.contains(QStringLiteral("файл с пробелом.txt")));
        QVERIFY(names.contains(QStringLiteral("папка Unicode/nested.txt")));
        QVERIFY(QDir(temp.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
    void addToExisting_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QString("7z");
        QTest::newRow("zip") << QString("zip");
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
            names << entry.path.normalized(QString::NormalizationForm_C);
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
    QCoreApplication app(argc, argv);
    const QByteArray mode = qgetenv("SEVENZIP_TEST_HELPER");
    if (!mode.isEmpty()) {
        if (mode == "fragment") {
            writeOutput("  4");
            QThread::msleep(20);
            writeOutput("2%\b\b    \b\b+ папка/имя с пробелом.txt\r");
            writeOutput("ERROR: incomplete tail");
            return 2;
        }
        if (mode == "large") {
            for (int i = 0; i < 3000; ++i) {
                writeOutput("Path = file" + QByteArray::number(i) + ".txt\nSize = 1\n\n");
                if (i == 999)
                    QThread::msleep(100);
            }
            return 0;
        }
        if (mode == "listing") {
            writeOutput(qgetenv("SEVENZIP_TEST_LISTING"));
            return 0;
        }
        if (mode == "createpartial" || mode == "createfail") {
            const QStringList arguments = QCoreApplication::arguments();
            const int separator = arguments.indexOf("--");
            if (separator < 0 || separator + 1 >= arguments.size())
                return 3;
            QFile partial(arguments.at(separator + 1));
            if (!partial.open(QIODevice::WriteOnly))
                return 4;
            partial.write("incomplete archive");
            partial.close();
            writeOutput("started\n");
            if (mode == "createfail")
                return 2;
            QThread::msleep(30000);
            return 0;
        }
        writeOutput("started\npartial");
        QThread::msleep(30000);
        return 0;
    }
    ArchiveBackendTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "archive_backend_tests.moc"

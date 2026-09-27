#include "archive_backend_test_support.h"

class ArchiveBackendTests : public QObject, protected ArchiveBackendTestSupport {
    Q_OBJECT

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
};

int main(int argc, char **argv) {
    ArchiveBackendTests tests;
    return backendTestMain(argc, argv, &tests);
}
#include "archive_backend_tests.moc"

#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "archivebackend.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QtTest>

#include <cstdio>

class ExtractionOptionsTests : public QObject {
    Q_OBJECT

    static void writeFile(const QString &path, const QByteArray &contents) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(contents), contents.size());
    }

    static QByteArray readFile(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return file.readAll();
    }

    static void await(ArchiveBackend &backend, bool success) {
        QSignalSpy result(&backend, &ArchiveBackend::finished);
        QVERIFY(result.wait(30000));
        QCOMPARE(result.takeFirst().at(0).toBool(), success);
    }

  private slots:
    void timestampFilenameRemainsInsideDestination() {
#ifdef Q_OS_WIN
        QSKIP("Colons are not ordinary Windows filename characters");
#else
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("2024-05-07T02:00:02Z.sql.gz");
        const QString source = fixture.filePath(name);
        const QString archive = fixture.filePath("timestamp.zip");
        const QString destination = fixture.filePath("output");
        writeFile(source, "fixture SQL gzip bytes");
        ArchiveBackend backend;
        backend.create(archive, {source});
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        QString error;
        const QString output = QDir(destination).filePath(name);
        QVERIFY2(ArchiveBackend::validateExtractionOutput(destination, output, false, &error),
                 qPrintable(error));
        backend.extract(archive, destination);
        await(backend, true);
        QCOMPARE(readFile(output), QByteArray("fixture SQL gzip bytes"));
        QVERIFY(!ArchiveBackend::validateExtractionOutput(
            destination, fixture.filePath("escape:outside.txt"), false, &error));
        QVERIFY(QDir().mkpath(destination));
        const QString linked = QDir(destination).filePath("link:inside");
        if (QFile::link(fixture.path(), linked)) {
            QVERIFY(!ArchiveBackend::validateExtractionOutput(destination, linked + "/escape.txt",
                                                              false, &error));
            QVERIFY(!QFileInfo::exists(fixture.filePath("escape.txt")));
        }
#endif
    }
    void askOverwriteAfterCancelledProcessCanSucceed() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString name = QStringLiteral("файл с пробелом.txt");
        const QString source = fixture.filePath(name);
        const QString archive = fixture.filePath("fixture.zip");
        const QString output = fixture.filePath("output");
        writeFile(source, "new archive bytes");
        QVERIFY(QDir().mkpath(output));
        writeFile(QDir(output).filePath(name), "old bytes");
        ArchiveBackend backend;
        backend.create(archive, {source});
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        const auto cancellation =
            connect(&backend, &ArchiveBackend::engineStarted, &backend, &ArchiveBackend::cancel);
        backend.testSourceArchive(archive, {});
        await(backend, false);
        disconnect(cancellation);
        QCOMPARE(backend.lastResult(), E_ABORT);
        QSignalSpy requested(&backend, &ArchiveBackend::overwriteRequired);
        connect(&backend, &ArchiveBackend::overwriteRequired, &backend,
                [&] { backend.provideOverwriteAnswer(NOverwriteAnswer::kYes); });
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        backend.extract(archive, output, {}, {}, options);
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(requested.size(), 1);
        QCOMPARE(readFile(QDir(output).filePath(name)), QByteArray("new archive bytes"));
        QCOMPARE(backend.lastResult(), S_OK);
        QVERIFY(!backend.busy());
    }
    void currentFolderValidationUsesOutputRelativePath() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("папка с пробелом"));
        QVERIFY(QDir().mkpath(source));
        writeFile(QDir(source).filePath(QStringLiteral("δ.txt")), "archive bytes");
        const QString other = temp.filePath(QStringLiteral("другая папка"));
        QVERIFY(QDir().mkpath(other));
        writeFile(QDir(other).filePath(QStringLiteral("δ.txt")), "other bytes");
        const QString archive = temp.filePath("fixture.zip");
        ArchiveBackend backend;
        backend.create(archive, {source, other});
        await(backend, true);
        backend.list(archive);
        await(backend, true);

        const QString output = temp.filePath("output");
        QVERIFY(QDir().mkpath(output + QStringLiteral("/δ.txt")));
        ExtractionOptions options;
        options.pathMode = ExtractionOptions::PathMode::CurrentPaths;
        options.currentFolder = QStringLiteral("папка с пробелом");
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        QString error;
        QVERIFY(!backend.validateExtraction(output, {QStringLiteral("папка с пробелом/δ.txt")},
                                            options, &error));
        QVERIFY(error.contains(QStringLiteral("δ.txt")));

        error.clear();
        QVERIFY(!backend.validateExtraction(output, {QStringLiteral("другая папка/δ.txt")}, options,
                                            &error));
        QVERIFY(error.contains("outside the current archive folder"));
    }

    void askDecisions_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QStringLiteral("7z");
        QTest::newRow("zip") << QStringLiteral("zip");
    }

    void askDecisions() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("источник с пробелом"));
        const QString output = temp.filePath("output");
        QVERIFY(QDir().mkpath(source));
        QVERIFY(QDir().mkpath(output + QStringLiteral("/источник с пробелом")));
        for (const QString &name :
             {QStringLiteral("α.txt"), QStringLiteral("β.txt"), QStringLiteral("γ.txt")}) {
            writeFile(source + "/" + name, "new");
            writeFile(output + QStringLiteral("/источник с пробелом/") + name, "old");
        }
        writeFile(output + QStringLiteral("/источник с пробелом/γ_1.txt"), "occupied rename");
        const QString archive = temp.filePath("fixture." + format);
        ArchiveBackend backend;
        backend.create(archive, {source});
        await(backend, true);
        backend.list(archive);
        await(backend, true);

        QList<int> answers{NOverwriteAnswer::kNo, NOverwriteAnswer::kYes,
                           NOverwriteAnswer::kAutoRename};
        QSignalSpy requested(&backend, &ArchiveBackend::overwriteRequired);
        connect(&backend, &ArchiveBackend::overwriteRequired, &backend,
                [&backend, &answers](const QString &, const ArchiveEntry &) {
                    backend.provideOverwriteAnswer(answers.takeFirst());
                });
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        backend.extract(archive, output, {}, {}, options);
        await(backend, true);
        QCOMPARE(requested.size(), 3);
        QCOMPARE(readFile(output + QStringLiteral("/источник с пробелом/α.txt")),
                 QByteArray("old"));
        QCOMPARE(readFile(output + QStringLiteral("/источник с пробелом/β.txt")),
                 QByteArray("new"));
        QCOMPARE(readFile(output + QStringLiteral("/источник с пробелом/γ.txt")),
                 QByteArray("old"));
        const QStringList names = QDir(output + QStringLiteral("/источник с пробелом"))
                                      .entryList(QDir::Files | QDir::NoDotAndDotDot);
        QCOMPARE(names.size(), 5);
        QCOMPARE(readFile(output + QStringLiteral("/источник с пробелом/γ_1.txt")),
                 QByteArray("occupied rename"));
        for (const QString &name : names)
            if (name != QStringLiteral("α.txt") && name != QStringLiteral("β.txt") &&
                name != QStringLiteral("γ.txt") && name != QStringLiteral("γ_1.txt"))
                QCOMPARE(readFile(output + QStringLiteral("/источник с пробелом/") + name),
                         QByteArray("new"));

        const QString flat = temp.filePath("flat output");
        QVERIFY(QDir().mkpath(flat));
        writeFile(flat + QStringLiteral("/β.txt"), "flat old");
        ExtractionOptions flatOptions;
        flatOptions.pathMode = ExtractionOptions::PathMode::NoPaths;
        flatOptions.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        answers = {NOverwriteAnswer::kYes};
        backend.extract(archive, flat, {QStringLiteral("источник с пробелом/β.txt")}, {},
                        flatOptions);
        await(backend, true);
        QCOMPARE(readFile(flat + QStringLiteral("/β.txt")), QByteArray("new"));
        QVERIFY(!QFileInfo::exists(flat + QStringLiteral("/α.txt")));
    }

    void askAllAndCancel() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath("source");
        QVERIFY(QDir().mkpath(source));
        for (const QString &name : {QStringLiteral("one"), QStringLiteral("two")})
            writeFile(source + "/" + name, "new");
        const QString archive = temp.filePath("fixture.7z");
        ArchiveBackend backend;
        backend.create(archive, {source});
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        for (const int answer :
             {NOverwriteAnswer::kNoToAll, NOverwriteAnswer::kYesToAll, NOverwriteAnswer::kCancel}) {
            const QString output = temp.filePath(QStringLiteral("output-%1").arg(answer));
            QVERIFY(QDir().mkpath(output + "/source"));
            writeFile(output + "/source/one", "old");
            writeFile(output + "/source/two", "old");
            QSignalSpy requested(&backend, &ArchiveBackend::overwriteRequired);
            QSignalSpy finished(&backend, &ArchiveBackend::finished);
            const auto connection =
                connect(&backend, &ArchiveBackend::overwriteRequired, &backend,
                        [&backend, answer](const QString &, const ArchiveEntry &) {
                            if (answer == NOverwriteAnswer::kCancel)
                                backend.cancel();
                            else
                                backend.provideOverwriteAnswer(answer);
                        });
            backend.extract(archive, output, {}, {}, options);
            QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 30000);
            QCOMPARE(finished.first().first().toBool(), answer != NOverwriteAnswer::kCancel);
            QCOMPARE(requested.size(), 1);
            QCOMPARE(readFile(output + "/source/one"),
                     answer == NOverwriteAnswer::kYesToAll ? QByteArray("new") : QByteArray("old"));
            QCOMPARE(readFile(output + "/source/two"),
                     answer == NOverwriteAnswer::kYesToAll ? QByteArray("new") : QByteArray("old"));
            disconnect(connection);
        }
    }

    void encryptedAskQueue_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QStringLiteral("7z");
        QTest::newRow("zip") << QStringLiteral("zip");
    }

    void encryptedAskQueue() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath("secret.txt");
        const QString output = temp.filePath("output");
        QVERIFY(QDir().mkpath(output));
        writeFile(source, "new secret");
        writeFile(output + "/secret.txt", "old secret");
        const QString archive = temp.filePath("encrypted." + format);
        const QString password = QStringLiteral("test-only-password");
        ArchiveBackend backend;
        backend.create(archive, {source}, password);
        await(backend, true);
        backend.list(archive, password);
        await(backend, true);
        connect(&backend, &ArchiveBackend::overwriteRequired, &backend,
                [&backend](const QString &, const ArchiveEntry &) {
                    backend.provideOverwriteAnswer(NOverwriteAnswer::kYes);
                });
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        QSignalSpy rejected(&backend, &ArchiveBackend::finished);
        backend.extract(archive, output, {}, QStringLiteral("incorrect-test-password"), options);
        QTRY_COMPARE_WITH_TIMEOUT(rejected.size(), 1, 30000);
        QVERIFY(!rejected.takeFirst().first().toBool());
        QCOMPARE(readFile(output + "/secret.txt"), QByteArray("old secret"));
        backend.extract(archive, output, {}, password, options);
        await(backend, true);
        QCOMPARE(readFile(output + "/secret.txt"), QByteArray("new secret"));
    }

    void askWithoutConsumerFailsBeforeEngineWrite() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath("file.txt");
        const QString archive = temp.filePath("fixture.7z");
        const QString output = temp.filePath("output");
        QVERIFY(QDir().mkpath(output));
        writeFile(source, "new");
        writeFile(output + "/file.txt", "old");
        ArchiveBackend backend;
        backend.create(archive, {source});
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        ExtractionOptions options;
        options.overwriteMode = ExtractionOptions::OverwriteMode::Ask;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.extract(archive, output, {}, {}, options);
        QCOMPARE(finished.size(), 1);
        QVERIFY(!finished.first().first().toBool());
        QVERIFY(!backend.busy());
        QCOMPARE(readFile(output + "/file.txt"), QByteArray("old"));
    }

    void modes_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QStringLiteral("7z");
        QTest::newRow("zip") << QStringLiteral("zip");
    }

    void modes() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath(QStringLiteral("source folder"));
        const QString destination = temp.filePath(QStringLiteral("output folder"));
        QVERIFY(QDir().mkpath(source + QStringLiteral("/вложено")));
        QVERIFY(QDir().mkpath(destination));
        writeFile(source + QStringLiteral("/вложено/δ file.txt"), "archive version");
        writeFile(source + QStringLiteral("/new.txt"), "new file");
        const QString archive = temp.filePath(QStringLiteral("fixture.") + format);
        ArchiveBackend backend;
        backend.create(archive, {source});
        await(backend, true);
        backend.list(archive);
        await(backend, true);

        ExtractionOptions flat;
        flat.pathMode = ExtractionOptions::PathMode::NoPaths;
        backend.extract(archive, destination, {}, {}, flat);
        await(backend, true);
        QCOMPARE(readFile(destination + QStringLiteral("/δ file.txt")),
                 QByteArray("archive version"));
        QVERIFY(
            !QFileInfo::exists(destination + QStringLiteral("/source folder/вложено/δ file.txt")));

        writeFile(destination + QStringLiteral("/δ file.txt"), "existing version");
        QSignalSpy rejected(&backend, &ArchiveBackend::finished);
        backend.extract(archive, destination, {}, {}, flat);
        QCOMPARE(rejected.size(), 1);
        QVERIFY(!rejected.takeFirst().at(0).toBool());
        QCOMPARE(readFile(destination + QStringLiteral("/δ file.txt")),
                 QByteArray("existing version"));

        flat.overwriteMode = ExtractionOptions::OverwriteMode::SkipExisting;
        backend.extract(archive, destination, {}, {}, flat);
        await(backend, true);
        QCOMPARE(readFile(destination + QStringLiteral("/δ file.txt")),
                 QByteArray("existing version"));

        flat.overwriteMode = ExtractionOptions::OverwriteMode::OverwriteExisting;
        backend.extract(archive, destination, {}, {}, flat);
        await(backend, true);
        QCOMPARE(readFile(destination + QStringLiteral("/δ file.txt")),
                 QByteArray("archive version"));

        const QString full = temp.filePath(QStringLiteral("full paths"));
        QVERIFY(QDir().mkpath(full));
        backend.extract(archive, full);
        await(backend, true);
        QCOMPARE(readFile(full + QStringLiteral("/source folder/вложено/δ file.txt")),
                 QByteArray("archive version"));
    }

    void renameConflicts_data() {
        QTest::addColumn<QString>("format");
        QTest::newRow("7z") << QStringLiteral("7z");
        QTest::newRow("zip") << QStringLiteral("zip");
    }

    void renameConflicts() {
        QFETCH(QString, format);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath("источник");
        QVERIFY(QDir().mkpath(source));
        writeFile(source + "/δ file.txt", "archive version");
        const QString archive = temp.filePath("fixture." + format);
        ArchiveBackend backend;
        backend.create(archive, {source});
        await(backend, true);
        backend.list(archive);
        await(backend, true);

        const QString relative = QStringLiteral("источник/δ file.txt");
        for (const auto mode : {ExtractionOptions::OverwriteMode::RenameExtracted,
                                ExtractionOptions::OverwriteMode::RenameExisting}) {
            const QString destination = temp.filePath(
                mode == ExtractionOptions::OverwriteMode::RenameExtracted ? "renamed extracted"
                                                                          : "renamed existing");
            QVERIFY(QDir().mkpath(destination + "/источник"));
            writeFile(destination + "/" + relative, "existing version");
            ExtractionOptions options;
            options.overwriteMode = mode;
            backend.extract(archive, destination, {}, {}, options);
            await(backend, true);

            const QByteArray expectedOriginal =
                mode == ExtractionOptions::OverwriteMode::RenameExtracted
                    ? QByteArray("existing version")
                    : QByteArray("archive version");
            QCOMPARE(readFile(destination + "/" + relative), expectedOriginal);
            const QStringList names =
                QDir(destination + "/источник").entryList(QDir::Files | QDir::NoDotAndDotDot);
            QCOMPARE(names.size(), 2);
            for (const QString &name : names) {
                if (name == QStringLiteral("δ file.txt"))
                    continue;
                QCOMPARE(readFile(destination + "/источник/" + name),
                         mode == ExtractionOptions::OverwriteMode::RenameExtracted
                             ? QByteArray("archive version")
                             : QByteArray("existing version"));
            }
        }
    }

    void flatNameCollision() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString source = temp.filePath("sources");
        const QString destination = temp.filePath("output");
        QVERIFY(QDir().mkpath(source + "/first"));
        QVERIFY(QDir().mkpath(source + "/second"));
        QVERIFY(QDir().mkpath(destination));
        writeFile(source + "/first/same.txt", "first");
        writeFile(source + "/second/same.txt", "second");
        const QString archive = temp.filePath("collision.zip");
        ArchiveBackend backend;
        backend.create(archive, {source});
        await(backend, true);
        backend.list(archive);
        await(backend, true);
        ExtractionOptions flat;
        flat.pathMode = ExtractionOptions::PathMode::NoPaths;
        QSignalSpy rejected(&backend, &ArchiveBackend::finished);
        backend.extract(archive, destination, {}, {}, flat);
        QCOMPARE(rejected.size(), 1);
        QVERIFY(!rejected.takeFirst().at(0).toBool());
        QVERIFY(!QFileInfo::exists(destination + "/same.txt"));

        backend.extract(archive, destination, {"sources/first/same.txt"}, {}, flat);
        await(backend, true);
        QCOMPARE(readFile(destination + "/same.txt"), QByteArray("first"));
    }

    void skipRejectsLinkedDestination() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString archive = temp.filePath("fake.zip");
        const QString destination = temp.filePath("destination");
        QVERIFY(QDir().mkpath(destination));
        writeFile(temp.filePath("outside.txt"), "outside");
        if (!QFile::link(temp.filePath("outside.txt"), destination + "/file.txt"))
            QSKIP("This filesystem does not permit test symlinks");
        qputenv("SEVENZIP_EXTRACTION_HELPER", "1");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        backend.list(archive);
        await(backend, true);
        ExtractionOptions skip;
        skip.overwriteMode = ExtractionOptions::OverwriteMode::SkipExisting;
        QSignalSpy rejected(&backend, &ArchiveBackend::finished);
        backend.extract(archive, destination, {}, {}, skip);
        QCOMPARE(rejected.size(), 1);
        QVERIFY(!rejected.takeFirst().at(0).toBool());
        QCOMPARE(readFile(temp.filePath("outside.txt")), QByteArray("outside"));
        qunsetenv("SEVENZIP_EXTRACTION_HELPER");
    }

    void cancellation() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString archive = temp.filePath("fake.zip");
        const QString destination = temp.filePath("output");
        QVERIFY(QDir().mkpath(destination));
        writeFile(archive, "test fixture");
        qputenv("SEVENZIP_EXTRACTION_HELPER", "1");
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        backend.list(archive);
        await(backend, true);
        ExtractionOptions flat;
        flat.pathMode = ExtractionOptions::PathMode::NoPaths;
        flat.overwriteMode = ExtractionOptions::OverwriteMode::SkipExisting;
        QSignalSpy progress(&backend, &ArchiveBackend::progress);
        QSignalSpy result(&backend, &ArchiveBackend::finished);
        backend.extract(archive, destination, {}, {}, flat);
        QTRY_VERIFY_WITH_TIMEOUT(!progress.isEmpty(), 3000);
        backend.cancel();
        QTRY_VERIFY_WITH_TIMEOUT(!result.isEmpty(), 3000);
        QVERIFY(!result.takeFirst().at(0).toBool());
        QVERIFY(!backend.busy());
        qunsetenv("SEVENZIP_EXTRACTION_HELPER");
    }
};

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (qEnvironmentVariableIsSet("SEVENZIP_EXTRACTION_HELPER")) {
        if (app.arguments().contains("l")) {
            std::fputs("Path = file.txt\nSize = 1\n\n", stdout);
            std::fflush(stdout);
            return 0;
        }
        std::fputs("1% file.txt\n", stdout);
        std::fflush(stdout);
        QThread::sleep(30);
        return 0;
    }
    ExtractionOptionsTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "extraction_options_tests.moc"

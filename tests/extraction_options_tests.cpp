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

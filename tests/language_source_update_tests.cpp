#include "7zip/UI/GUI/CompressDialogRes.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivebackend.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

class LanguageSourceUpdateTests final : public QObject {
    Q_OBJECT
    QTemporaryDir settings;
    void language(bool russian) {
        const QString path = settings.filePath("language.ini");
        SaveRegLang(UString(russian ? "ru" : "-"), path);
        ReloadLang(path);
    }
    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }
    static bool write(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }

  private slots:
    void cleanup() { language(false); }
    void compressionAndMutationRejections_data() { sourceUpdate_data(); }
    void compressionAndMutationRejections() {
        QFETCH(bool, russian);
        language(russian);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString source = directory.filePath(QStringLiteral("input 密碼 file.txt"));
        const QString target = directory.filePath("output.zip");
        QVERIFY(write(source, "input bytes"));
        ArchiveBackend backend;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        CompressionOptions options;
        options.EncryptionMethod = "AES256";
        backend.create(target, {source}, QString(100, 'x'), false, options);
        QCOMPARE(finished.size(), 1);
        auto result = finished.takeFirst();
        QVERIFY(!result.first().toBool());
        QCOMPARE(result.at(1).toString(),
                 QString::fromWCharArray(LangString(IDS_PASSWORD_TOO_LONG).Ptr()));
        backend.create(target, {source}, QStringLiteral("密碼"), false, options);
        QCOMPARE(finished.size(), 1);
        result = finished.takeFirst();
        QVERIFY(!result.first().toBool());
        QCOMPARE(result.at(1).toString(),
                 QString::fromWCharArray(LangString(IDS_PASSWORD_USE_ASCII).Ptr()));
        options.method = "invalid";
        backend.create(target, {source}, {}, false, options);
        QCOMPARE(finished.size(), 1);
        result = finished.takeFirst();
        QVERIFY(!result.first().toBool());
        QCOMPARE(result.at(1).toString(),
                 russian ? QStringLiteral("Недопустимые параметры сжатия для выбранного формата.")
                         : QString("Invalid compression parameters for the selected format."));
        backend.renameEntry(target, "old.txt", "new.txt");
        QCOMPARE(finished.size(), 1);
        result = finished.takeFirst();
        QVERIFY(!result.first().toBool());
        QCOMPARE(result.at(1).toString(),
                 russian ? QStringLiteral("Сначала откройте архив и проверьте его содержимое.")
                         : QString("Open the archive and inspect its contents first."));
        QCOMPARE(started.size(), 0);
        backend.extractSourceEntry(target, source, directory.filePath("extracted"), "zip", {});
        QCOMPARE(finished.size(), 1);
        result = finished.takeFirst();
        QVERIFY(!result.first().toBool());
        QCOMPARE(result.at(1).toString(),
                 russian ? QStringLiteral("Недопустимый файл или поток извлечения.")
                         : QString("Invalid extraction file or stream."));
        QCOMPARE(backend.lastResult(), E_FAIL);
        QCOMPARE(started.size(), 0);
        QVERIFY(!QFile::exists(target));
        QCOMPARE(read(source), QByteArray("input bytes"));
        QCOMPARE(QDir(directory.path()).entryList(QDir::Files | QDir::Hidden),
                 QStringList{QFileInfo(source).fileName()});
    }
    void sourceUpdate_data() {
        QTest::addColumn<bool>("russian");
        QTest::newRow("en") << false;
        QTest::newRow("ru") << true;
    }
    void sourceUpdate() {
        QFETCH(bool, russian);
        language(russian);
        const QString engine = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine.isEmpty());
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString name = QStringLiteral("file 密碼 с пробелом.txt");
        const QString source = directory.filePath(name);
        const QString archive = directory.filePath("original.zip");
        const QString output = directory.filePath("updated.zip");
        QVERIFY(write(source, "original"));
        QProcess create;
        create.setWorkingDirectory(directory.path());
        create.start(engine, {"a", "-tzip", "--", archive, name});
        QVERIFY(create.waitForFinished(30000));
        QCOMPARE(create.exitCode(), 0);
        const QByteArray original = read(archive);
        QVERIFY(!original.isEmpty());
        ArchiveBackend backend(nullptr, engine);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QString root = "unchanged";
        QStringList paths = {"unchanged"};
        QString error;
        const QString missing = directory.filePath(QStringLiteral("missing 密碼 file.txt"));
        QVERIFY(!ArchiveBackend::prepareSourceSelection(archive, {missing}, &root, &paths, &error));
        QCOMPARE(error, (russian ? QStringLiteral("Недоступный источник обновления: %1")
                                 : QString("Update source unavailable: %1"))
                            .arg(missing));
        QCOMPARE(root, QString("unchanged"));
        QCOMPARE(paths, QStringList{"unchanged"});

        backend.updateSourceAdd(archive, {"../escape.txt"}, directory.path(), output, "zip", {});
        QCOMPARE(finished.size(), 1);
        auto result = finished.takeFirst();
        QVERIFY(!result.first().toBool());
        QCOMPARE(result.at(1).toString(),
                 russian ? QStringLiteral("Небезопасный путь добавления в архив.")
                         : QString("Unsafe archive add path."));
        QCOMPARE(backend.lastResult(), E_FAIL);
        QVERIFY(!QFile::exists(output));
        QCOMPARE(read(archive), original);

        ArchiveBackend::SourceUpdatePlan plan;
        plan.files = {name};
        plan.fileRoot = directory.path();
        backend.updateSourcePlan(archive, plan, output, "zip", "fixture-password");
        QCOMPARE(finished.size(), 1);
        result = finished.takeFirst();
        QVERIFY(!result.first().toBool());
        QCOMPARE(result.at(1).toString(),
                 russian ? QStringLiteral("Обновление защищённого архива не подтверждено.")
                         : QString("Protected archive update has not been confirmed."));
        QCOMPARE(backend.lastResult(), E_FAIL);
        QVERIFY(!QFile::exists(output));

        QVERIFY(write(source, "new bytes"));
        backend.updateSourcePlan(archive, plan, output, "zip", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 30000);
        result = finished.takeFirst();
        QVERIFY(result.first().toBool());
        QCOMPARE(backend.lastResult(), S_OK);
        QCOMPARE(read(archive), original);
        const QString extracted = directory.filePath("extracted");
        QProcess verify;
        verify.start(engine, {"x", "-y", "-o" + extracted, "--", output});
        QVERIFY(verify.waitForFinished(30000));
        QCOMPARE(verify.exitCode(), 0);
        QCOMPARE(read(QDir(extracted).filePath(name)), QByteArray("new bytes"));
        QVERIFY(QFile::remove(output));

        backend.updateSourcePlan(archive, {}, output, "zip", {});
        QCOMPARE(finished.size(), 1);
        result = finished.takeFirst();
        QVERIFY(result.first().toBool());
        QCOMPARE(result.at(1).toString(), russian ? QStringLiteral("Архив не требует обновления.")
                                                  : QString("The archive does not need updating."));
        QCOMPARE(read(output), original);
        QVERIFY(QFile::remove(output));

        connect(&backend, &ArchiveBackend::engineStarted, &backend, &ArchiveBackend::cancel);
        backend.updateSourcePlan(archive, plan, output, "zip", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 30000);
        result = finished.takeFirst();
        QVERIFY(!result.first().toBool());
        QCOMPARE(result.at(1).toString(),
                 russian ? QStringLiteral("Операция отменена.") : QString("Operation canceled."));
        QCOMPARE(backend.lastResult(), E_ABORT);
        QVERIFY(!backend.busy());
        QVERIFY(!QFile::exists(output));
        QCOMPARE(read(archive), original);
        QCOMPARE(read(source), QByteArray("new bytes"));
    }
};
QTEST_GUILESS_MAIN(LanguageSourceUpdateTests)
#include "language_source_update_tests.moc"

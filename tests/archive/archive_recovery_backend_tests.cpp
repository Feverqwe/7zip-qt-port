#include "archive/support/archive_backend_test_support.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QtEndian>

class ArchiveRecoveryBackendTests final : public QObject, protected ArchiveBackendTestSupport {
    Q_OBJECT
    QString engine_;
    void create(const QString &root, const QString &archive, bool encrypted) {
        writeFile(QDir(root).filePath(QStringLiteral("healthy 空 file.txt")), "healthy bytes");
        writeFile(QDir(root).filePath("broken.txt"), QByteArray(256, 'B'));
        QStringList args{"a", "-tzip", "-mx=0"};
        if (encrypted)
            args << "-mem=ZipCrypto" << "-pfixture-only-password";
        args << "--" << archive << QStringLiteral("healthy 空 file.txt") << "broken.txt";
        QProcess creator;
        creator.setWorkingDirectory(root);
        creator.start(engine_, args);
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
    }
  private slots:
    void initTestCase() {
        engine_ = QStandardPaths::findExecutable("7z");
        QVERIFY(!engine_.isEmpty());
    }
    void readableCatalogueSurvivesListingErrors() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString path = fixture.filePath(QStringLiteral("усечённый archive.zip"));
        create(fixture.path(), path, false);
        QFile source(path);
        QVERIFY(source.open(QIODevice::ReadOnly));
        QByteArray bytes = source.readAll();
        source.close();
        const auto central = bytes.indexOf(QByteArray::fromHex("504b0102"));
        QVERIFY(central > 0);
        bytes.truncate(central);
        const auto payload = bytes.indexOf(QByteArray(256, 'B'));
        QVERIFY(payload > 0);
        bytes[payload] = '!';
        writeFile(path, bytes);
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy info(&backend, &ArchiveBackend::infoReady);
        QSignalSpy entries(&backend, &ArchiveBackend::entriesReady);
        backend.list(path);
        await(backend, true);
        QCOMPARE(entries.size(), 1);
        QCOMPARE(qvariant_cast<QList<ArchiveEntry>>(entries[0][0]).size(), 2);
        QVERIFY(!qvariant_cast<ArchiveInfo>(info[0][0]).readLimitations.isEmpty());
        const QString output = fixture.filePath("output");
        backend.extract(path, output, {QStringLiteral("healthy 空 file.txt")});
        await(backend, false); // Readable output does not clear the archive header error.
        QFile copied(output + QStringLiteral("/healthy 空 file.txt"));
        QVERIFY(copied.open(QIODevice::ReadOnly));
        QCOMPARE(copied.readAll(), QByteArray("healthy bytes"));
        backend.extract(path, output, {"broken.txt"});
        await(backend, false);
        QFile broken(output + "/broken.txt");
        QVERIFY(broken.open(QIODevice::ReadOnly));
        QCOMPARE(broken.readAll(), QByteArray("!") + QByteArray(255, 'B'));
        QStringList diagnostics;
        connect(&backend, &ArchiveBackend::outputReceived, &backend,
                [&](const QString &line) { diagnostics.append(line); });
        backend.extractSourceEntry(path, "broken.txt", fixture.filePath("unverified stream"), "zip",
                                   {});
        await(backend, false);
        QVERIFY2(backend.lastSourceExtractOperation() ==
                     NArchive::NExtract::NOperationResult::kCRCError,
                 qPrintable(diagnostics.join('\n')));
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), bytes);
    }
    void encryptedSelectionDoesNotTestUnselectedCorruption() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString path = fixture.filePath("encrypted.zip");
        create(fixture.path(), path, true);
        QFile source(path);
        QVERIFY(source.open(QIODevice::ReadOnly));
        QByteArray bytes = source.readAll();
        source.close();
        const auto name = bytes.indexOf("broken.txt");
        const auto header = bytes.lastIndexOf(QByteArray::fromHex("504b0304"), name);
        QVERIFY(header >= 0);
        const auto *raw = reinterpret_cast<const uchar *>(bytes.constData() + header);
        const auto payload = header + 30 + qFromLittleEndian<quint16>(raw + 26) +
                             qFromLittleEndian<quint16>(raw + 28);
        QVERIFY(payload + 80 < bytes.size());
        bytes[payload + 80] ^= 1;
        writeFile(path, bytes);
        ArchiveBackend backend(nullptr, engine_);
        backend.list(path);
        await(backend, true);
        backend.test(path, "fixture-only-password");
        await(backend, false);
        const QString output = fixture.filePath("healthy output");
        backend.extract(path, output, {QStringLiteral("healthy 空 file.txt")},
                        "fixture-only-password");
        await(backend, true);
        QFile copied(output + QStringLiteral("/healthy 空 file.txt"));
        QVERIFY(copied.open(QIODevice::ReadOnly));
        QCOMPARE(copied.readAll(), QByteArray("healthy bytes"));
        backend.extract(path, fixture.filePath("bad password"),
                        {QStringLiteral("healthy 空 file.txt")}, "wrong-password");
        await(backend, false);
        backend.extract(path, fixture.filePath("unverified output"), {"broken.txt"},
                        "fixture-only-password");
        await(backend, false);
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), bytes);
    }
    void truncatedStreamsRetainReadableData_data() {
        QTest::addColumn<QString>("format");
        for (const QString &format : {"gzip", "bzip2", "xz", "zstd"})
            QTest::newRow(qPrintable(format)) << format;
    }
    void truncatedTarRetainsKnownFilePrefix() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        writeFile(fixture.filePath("a.txt"), "healthy bytes");
        writeFile(fixture.filePath("z.txt"), QByteArray(256, 'Z'));
        const QString path = fixture.filePath("incomplete.tar");
        QProcess creator;
        creator.setWorkingDirectory(fixture.path());
        creator.start(engine_, {"a", "-ttar", "--", path, "a.txt", "z.txt"});
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
        QFile source(path);
        QVERIFY(source.open(QIODevice::ReadOnly));
        QByteArray bytes = source.readAll();
        source.close();
        const auto payload = bytes.indexOf(QByteArray(256, 'Z'));
        QVERIFY(payload > 0);
        bytes.truncate(payload + 128);
        writeFile(path, bytes);
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy entries(&backend, &ArchiveBackend::entriesReady);
        backend.list(path);
        await(backend, true);
        QCOMPARE(qvariant_cast<QList<ArchiveEntry>>(entries[0][0]).size(), 2);
        const QString stream = fixture.filePath("readable prefix");
        backend.extractSourceEntry(path, "z.txt", stream, "tar", {});
        await(backend, false);
        QCOMPARE(backend.lastResult(), S_FALSE);
        QFile output(stream);
        QVERIFY(output.open(QIODevice::ReadOnly));
        QCOMPARE(output.readAll(), QByteArray(128, 'Z'));
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), bytes);
    }
    void truncatedStreamsRetainReadableData() {
        QFETCH(QString, format);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QByteArray payload(256 * 1024, '\0');
        quint32 random = 0x352147;
        for (char &byte : payload) {
            random = random * 1664525u + 1013904223u;
            byte = char(random >> 24);
        }
        writeFile(fixture.filePath("payload.bin"), payload);
        const QString path = fixture.filePath("stream." + format);
        QProcess creator;
        creator.setWorkingDirectory(fixture.path());
        if (format == "zstd")
            creator.start(QString::fromUtf8(SEVENZIP_RECOVERY_PYTHON),
                          {QString::fromUtf8(SEVENZIP_ZSTD_FIXTURE), "checksum", path});
        else
            creator.start(engine_, {"a", "-t" + format, "-mx=1", "--", path, "payload.bin"});
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
        if (format == "zstd")
            payload = QByteArray::fromHex(QJsonDocument::fromJson(creator.readAllStandardOutput())
                                              .object()["payload"]
                                              .toString()
                                              .toLatin1());
        QFile source(path);
        QVERIFY(source.open(QIODevice::ReadOnly));
        QByteArray bytes = source.readAll();
        source.close();
        QVERIFY(bytes.size() > 64);
        bytes.chop(4);
        writeFile(path, bytes);
        const QString stream = fixture.filePath("readable prefix");
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy entries(&backend, &ArchiveBackend::entriesReady);
        backend.list(path);
        await(backend, true);
        QCOMPARE(entries.size(), 1);
        const auto listed = qvariant_cast<QList<ArchiveEntry>>(entries[0][0]);
        QCOMPARE(listed.size(), 1);
        backend.extractSourceEntry(path, listed[0].path, stream, format, {});
        await(backend, false);
        QCOMPARE(backend.lastResult(), S_FALSE);
        QFile output(stream);
        QVERIFY(output.open(QIODevice::ReadOnly));
        const auto recovered = output.readAll();
        QVERIFY(recovered.size() >= 65536);
        QVERIFY(payload.startsWith(recovered));
        QVERIFY(backend.lastSourceExtractOperation() != NArchive::NExtract::NOperationResult::kOK);
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), bytes);
    }
};

QTEST_GUILESS_MAIN(ArchiveRecoveryBackendTests)
#include "archive_recovery_backend_tests.moc"

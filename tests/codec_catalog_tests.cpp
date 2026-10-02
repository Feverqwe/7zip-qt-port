#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "qtarchivefoldermanager.h"
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

class CodecCatalogTests : public QObject {
    Q_OBJECT

    class BusyBackend final : public ArchiveBackend {
      public:
        bool busy() const override { return true; }
    };

  private slots:
    void realOpenModesUseContentWithUnknownExtension_data() {
        QTest::addColumn<QString>("type");
        QTest::newRow("forced") << "zip";
        QTest::newRow("one-handler") << "*";
        QTest::newRow("parser") << "#";
    }
    void realOpenModesUseContentWithUnknownExtension() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; real open modes remain unverified");
#else
        QFETCH(QString, type);
        const QString engine = QStandardPaths::findExecutable("7zz");
        if (engine.isEmpty())
            QSKIP("Official engine unavailable; real open modes remain unverified");
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        ArchiveBackend backend(nullptr, engine);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog());
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QFile input(temporary.filePath("имя с пробелом.bin"));
        QVERIFY(input.open(QIODevice::WriteOnly));
        const QByteArray payload("binary\0payload", 14);
        QCOMPARE(input.write(payload), qint64(payload.size()));
        input.close();
        const QString seed = temporary.filePath("seed.zip");
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.create(seed, {input.fileName()});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile original(seed);
        QVERIFY(original.open(QIODevice::ReadOnly));
        const QByteArray bytes = original.readAll();
        QVERIFY(!bytes.isEmpty());
        QFile unknown(temporary.filePath("archives with spaces.unknown"));
        QVERIFY(unknown.open(QIODevice::WriteOnly));
        const QByteArray contents = type == "#" ? bytes + bytes : bytes;
        QCOMPARE(unknown.write(contents), qint64(contents.size()));
        unknown.close();
        QString detected;
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::infoReady, this,
                [&](const ArchiveInfo &info) { detected = info.format; });
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        finished.clear();
        backend.openFolder(unknown.fileName(), type);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(detected, type == "#" ? QStringLiteral("#") : QStringLiteral("zip"));
        if (type == "#") {
            QCOMPARE(entries.size(), 2);
            QCOMPARE(entries[0].path, QStringLiteral("1.zip"));
            QCOMPARE(entries[1].path, QStringLiteral("2.zip"));
            QVERIFY(!backend.codecCatalog()->canUpdate(detected));
        } else {
            QCOMPARE(entries.size(), 1);
            QCOMPARE(entries[0].size, qint64(payload.size()));
        }
        QVERIFY(original.seek(0));
        QCOMPARE(original.readAll(), bytes);
#endif
    }

    void realEngineDiscoversMatchingModuleAsynchronously() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; real discovery remains unverified");
#else
        const QString engine = QStandardPaths::findExecutable("7zz");
        if (engine.isEmpty())
            QSKIP("Official engine unavailable; real discovery remains unverified");
        ArchiveBackend backend(nullptr, engine);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog());
        QVERIFY(!backend.discoverCodecCatalog());
        QCOMPARE(backend.loadCodecCatalog("unused", 1), E_ABORT);
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        const auto catalog = backend.codecCatalog();
        QVERIFY(catalog && catalog->canUpdate("zip"));
        QVERIFY(!catalog->canUpdate("Rar5"));
        QVERIFY(backend.supportsArchiveUpdate("ZIP"));
        QVERIFY(!backend.supportsArchiveUpdate("Rar5"));
        QVERIFY(catalog->canUpdate("gzip"));
        QVERIFY(backend.supportsArchiveUpdate("gzip"));
        QVERIFY(!backend.supportsArchiveUpdate("#"));
        const QStringList filters = backend.openDialogFilter().split(";;");
        QCOMPARE(filters.size(), 2);
        QVERIFY(filters[0].contains("*.docx"));
        QVERIFY(filters[0].contains("*.001"));
        QCOMPARE(filters[1], QStringLiteral("All files (*)"));
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CMyComBSTR extensions;
        QCOMPARE(manager->GetExtensions(&extensions), S_OK);
        const QStringList aliases = QString::fromWCharArray(extensions).split(' ');
        QVERIFY(aliases.contains("docx"));
        QVERIFY(aliases.contains("tgz"));
        QCOMPARE(manager->GetExtensions(nullptr), E_INVALIDARG);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QSignalSpy started(&backend, &ArchiveBackend::engineStarted);
        backend.openFolder("unused", "zip..*");
        QCOMPARE(finished.size(), 1);
        QVERIFY(!finished[0][0].toBool());
        QCOMPARE(backend.lastResult(), E_INVALIDARG);
        QVERIFY(started.isEmpty());
        QVERIFY(!backend.busy());
#endif
    }

    void cancelledProbeCanRestartAndLateDeliveryCanDeleteBackend() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QFileInfo fixture(QString::fromUtf8(CODEC_PROBE_FIXTURE));
        const QString suffix = fixture.suffix().isEmpty() ? QString() : "." + fixture.suffix();
        const QString engine = temporary.filePath("probe й hang" + suffix);
        QVERIFY(QFile::copy(fixture.absoluteFilePath(), engine));
        auto *backend = new ArchiveBackend(nullptr, engine);
        QSignalSpy ready(backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend->discoverCodecCatalog("unused"));
        backend->cancelCodecDiscovery();
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(!ready[0][0].toBool());
        QVERIFY(!backend->codecCatalog());
        connect(backend, &ArchiveBackend::codecCatalogReady, this, [backend] { delete backend; });
        QVERIFY(backend->discoverCodecCatalog("unused"));
        backend->cancelCodecDiscovery();
        QTRY_COMPARE(ready.size(), 2);
    }

    void missingModulePublishesNoCapabilities() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        ArchiveBackend backend;
        QCOMPARE(backend.loadCodecCatalog(temporary.filePath("missing й library"), 1), E_FAIL);
        const auto catalog = backend.codecCatalog();
        QVERIFY(catalog);
        QVERIFY(!catalog->error().isEmpty());
        QVERIFY(catalog->extensions().isEmpty());
        QVERIFY(!catalog->canUpdate("zip"));
        QVERIFY(!backend.supportsArchiveUpdate("zip"));
        QCOMPARE(catalog->formatIndex("rar"), -1);
        CObjectVector<COpenType> types;
        QVERIFY(!catalog->parseOpenTypes("*", types));
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CMyComBSTR extensions;
        QCOMPARE(manager->GetExtensions(&extensions), E_FAIL);
        QVERIFY(!extensions);
        QCOMPARE(backend.openDialogFilter(), QStringLiteral("All files (*)"));
    }

    void busyAndUnverifiedVersionDoNotStartDiscovery() {
        BusyBackend busy;
        QCOMPARE(busy.loadCodecCatalog("unused", 1), E_ABORT);
        QVERIFY(!busy.codecCatalog());
        ArchiveBackend backend;
        QCOMPARE(backend.loadCodecCatalog("unused", 0), E_INVALIDARG);
        QVERIFY(!backend.codecCatalog());
    }

    void versionMismatchAndReloadPreserveOnlyRetainedNativeLeases() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; runtime catalog remains unverified");
#else
        const QString module = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveCodecCatalog probe(module, 0);
        QCOMPARE(probe.result(), S_OK);
        ArchiveBackend backend;
        QCOMPARE(backend.loadCodecCatalog(module, probe.version()), S_OK);
        auto previous = backend.codecCatalog();
        QVERIFY(previous->canUpdate("zip"));
        QVERIFY(!previous->canUpdate("Rar5"));
        QVERIFY(previous->extensions().contains("docx"));
        CObjectVector<COpenType> parsed;
        QVERIFY(previous->parseOpenTypes("zip.*", parsed));
        QCOMPARE(parsed.Size(), 2u);
        CMyComPtr<IInArchive> reader;
        const int rar = previous->formatIndex("Rar5");
        QVERIFY(rar >= 0);
        QCOMPARE(previous->nativeCodecs().CreateInArchive(unsigned(rar), reader), S_OK);
        QVERIFY(reader);
        QCOMPARE(backend.loadCodecCatalog(module, probe.version() + 1), E_FAIL);
        const auto failed = backend.codecCatalog();
        QVERIFY(failed != previous);
        QVERIFY(!failed->error().isEmpty());
        QVERIFY(failed->extensions().isEmpty());
        QVERIFY(!failed->canUpdate("zip"));
        QVERIFY(previous->canUpdate("zip"));
        // The old catalog keeps its module loaded until its native objects die.
        UInt32 count = 0;
        QCOMPARE(reader->GetNumberOfItems(&count), S_OK);
        reader.Release();
        previous.reset();
        QCOMPARE(backend.loadCodecCatalog(module, probe.version()), S_OK);
        QVERIFY(backend.codecCatalog()->canUpdate("zip"));
#endif
    }
};

QTEST_GUILESS_MAIN(CodecCatalogTests)
#include "codec_catalog_tests.moc"

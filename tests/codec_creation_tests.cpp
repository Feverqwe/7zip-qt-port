#include "archive_backend_test_support.h"
#include "archivecodeccatalog.h"
#include "source_creation_fixture.h"
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTimer>

class CodecCreationTests final : public QObject, protected ArchiveBackendTestSupport {
    Q_OBJECT
    QString engine_, module_;
    quint32 version_ = 0;

    void load(ArchiveBackend &backend) {
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
    }

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; real handler creation remains unverified");
#else
        engine_ = QStandardPaths::findExecutable("7z");
        if (engine_.isEmpty())
            QSKIP("Official engine unavailable; real handler creation remains unverified");
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        version_ = backend.codecCatalog()->version();
        QVERIFY(version_);
#endif
    }
    void realTypedFormatsRoundTripWithUnknownSuffix_data() {
        QTest::addColumn<QString>("format");
        for (const QString &format : {"7z", "zip", "gzip", "bzip2", "xz", "tar", "wim"})
            QTest::newRow(qPrintable(format)) << format;
    }
    void realTypedFormatsRoundTripWithUnknownSuffix() {
        QFETCH(QString, format);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath(QStringLiteral("payload 空 with spaces.bin"));
        const QByteArray bytes("binary\0payload", 14);
        writeFile(source, bytes);
        const QString target = fixture.filePath(QStringLiteral("архив 空 with spaces.unknown"));
        ArchiveBackend backend(nullptr, engine_);
        load(backend);
        CompressionOptions options;
        options.format = format;
        options.Level = format == "tar" || format == "wim" ? 0 : 5;
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QCOMPARE(SourceCreationFixture::Create(backend, target, {source}, {}, false, options),
                 S_OK);
        QVERIFY(QFileInfo::exists(target));
        QList<ArchiveEntry> entries;
        QString detected;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&](const QList<ArchiveEntry> &value) { entries = value; });
        connect(&backend, &ArchiveBackend::infoReady, this,
                [&](const ArchiveInfo &info) { detected = info.format; });
        finished.clear();
        backend.openFolder(target, format);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(detected.compare(format, Qt::CaseInsensitive), 0);
        QCOMPARE(entries.size(), 1);
        // The bzip2 handler does not report an unpacked size before decoding.
        QCOMPARE(entries[0].size, format == "bzip2" ? qint64(-1) : qint64(bytes.size()));
        QVERIFY(!entries[0].path.isEmpty());
        finished.clear();
        const QString output = fixture.filePath("extracted 空");
        backend.extract(target, output);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile extracted(QDir(output).filePath(entries[0].path));
        QVERIFY(extracted.open(QIODevice::ReadOnly));
        QCOMPARE(extracted.readAll(), bytes);
        QFile original(source);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QCOMPARE(original.readAll(), bytes);
        QVERIFY(
            QDir(fixture.path()).entryList({".sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }
};

QTEST_MAIN(CodecCreationTests)
#include "codec_creation_tests.moc"

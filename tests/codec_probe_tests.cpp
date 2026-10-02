#include "archivecodecprobe.h"
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class CodecProbeTests : public QObject {
    Q_OBJECT

    static QString Fixture(const QTemporaryDir &temporary, const QString &mode) {
        const QFileInfo source(QString::fromUtf8(CODEC_PROBE_FIXTURE));
        const QString suffix = source.suffix().isEmpty() ? QString() : "." + source.suffix();
        const QString target = temporary.filePath("probe й " + mode + suffix);
        return QFile::copy(source.absoluteFilePath(), target) ? target : QString();
    }

  private slots:
    void engineFailureAndMalformedVersion_data() {
        QTest::addColumn<QString>("mode");
        QTest::newRow("missing") << "missing";
        QTest::newRow("exit") << "failed-exit";
        QTest::newRow("banner") << "bad-banner";
    }
    void engineFailureAndMalformedVersion() {
        QFETCH(QString, mode);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString engine =
            mode == "missing" ? temporary.filePath(mode) : Fixture(temporary, mode);
        QVERIFY(!engine.isEmpty());
        ArchiveCodecProbe probe;
        int calls = 0;
        ArchiveCodecProbeResult result;
        QVERIFY(probe.Start(engine, "unused", [&](auto value) {
            result = std::move(value);
            ++calls;
        }));
        QVERIFY(!probe.Start(engine, "unused", {}));
        QTRY_COMPARE(calls, 1);
        QCOMPARE(result.result, E_FAIL);
        QVERIFY(!result.error.isEmpty());
        QVERIFY(!result.catalog);
        QVERIFY(!probe.active());
    }

    void cancellationAndDeadlineKeepEventLoopResponsive_data() {
        QTest::addColumn<bool>("cancel");
        QTest::newRow("cancel") << true;
        QTest::newRow("deadline") << false;
    }
    void cancellationAndDeadlineKeepEventLoopResponsive() {
        QFETCH(bool, cancel);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString engine = Fixture(temporary, "hang");
        QVERIFY(!engine.isEmpty());
        ArchiveCodecProbe probe;
        bool heartbeat = false;
        int calls = 0;
        ArchiveCodecProbeResult result;
        QTimer::singleShot(0, [&] { heartbeat = true; });
        QVERIFY(probe.Start(
            engine, "unused",
            [&](auto value) {
                result = std::move(value);
                ++calls;
            },
            cancel ? 2000 : 100));
        if (cancel)
            probe.Cancel(); // Also exercises cancellation while QProcess is Starting.
        QTRY_COMPARE(calls, 1);
        QVERIFY(heartbeat);
        QCOMPARE(result.result, E_ABORT);
        QVERIFY(!result.catalog);
        QVERIFY(!probe.active());
    }

    void matchingOfficialModuleAndVersionMismatch_data() {
        QTest::addColumn<QString>("mode");
        QTest::newRow("matching") << "matching";
        QTest::newRow("mismatch") << "wrong-version";
    }
    void matchingOfficialModuleAndVersionMismatch() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; version/module transport remains unverified");
#else
        QFETCH(QString, mode);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString engine = Fixture(temporary, mode);
        QVERIFY(!engine.isEmpty());
        ArchiveCodecProbe probe;
        int calls = 0;
        ArchiveCodecProbeResult result;
        QVERIFY(probe.Start(engine, QString::fromUtf8(SEVENZIP_CODEC_MODULE), [&](auto value) {
            result = std::move(value);
            ++calls;
        }));
        QTRY_COMPARE(calls, 1);
        QVERIFY(result.catalog);
        if (mode == "matching") {
            QCOMPARE(result.result, S_OK);
            QVERIFY(result.catalog->canUpdate("zip"));
            QCOMPARE(result.catalog->version(), UInt32((26 << 16) | 3));
        } else {
            QCOMPARE(result.result, E_FAIL);
            QVERIFY(result.catalog->extensions().isEmpty());
            QVERIFY(!result.error.isEmpty());
        }
#endif
    }
};

QTEST_GUILESS_MAIN(CodecProbeTests)
#include "codec_probe_tests.moc"

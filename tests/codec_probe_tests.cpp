#include "7zip/MyVersion.h"
#include "archivecodecprobe.h"
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>
#ifdef Q_OS_WIN
#include "fixtures/delayed_process_start.h"
#endif

class CodecProbeTests : public QObject {
    Q_OBJECT

    static QString Fixture(const QTemporaryDir &temporary, const QString &mode) {
        const QFileInfo source(QString::fromUtf8(CODEC_PROBE_FIXTURE));
        const QString suffix = source.suffix().isEmpty() ? QString() : "." + source.suffix();
        const QString target = temporary.filePath("probe й " + mode + suffix);
        return QFile::copy(source.absoluteFilePath(), target) ? target : QString();
    }

  private slots:
    void deadlineDoesNotDependOnGuiTimerDelivery() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString engine = Fixture(temporary, "delayed");
        QVERIFY(!engine.isEmpty());
        ArchiveCodecProbe probe;
        int calls = 0;
        ArchiveCodecProbeResult result;
        QVERIFY(probe.Start(
            engine, temporary.filePath("missing module"),
            [&](auto value) {
                QCOMPARE(QThread::currentThread(), thread());
                result = std::move(value);
                ++calls;
            },
            50));
        // Withhold GUI timer/completion delivery past the deadline. The worker
        // can terminate the child before its exit receipt is written.
        QElapsedTimer wait;
        wait.start();
        while (!QFileInfo::exists(engine + ".finished") && wait.elapsed() < 1000)
            QThread::msleep(5);
        if (QFileInfo::exists(engine + ".finished"))
            QThread::msleep(100);
        QCOMPARE(calls, 0);
        QTRY_COMPARE(calls, 1);
        QCOMPARE(result.result, E_ABORT);
        QVERIFY(!result.catalog);
        QVERIFY(!result.error.isEmpty());
        QVERIFY(!probe.active());
    }
#ifdef Q_OS_WIN
    void delayedNativeProcessStartKeepsEventLoopResponsive_data() {
        QTest::addColumn<QString>("mode");
        for (const char *mode : {"failure", "cancel", "deadline", "destroy"})
            QTest::newRow(mode) << QString::fromLatin1(mode);
    }
    void delayedNativeProcessStartKeepsEventLoopResponsive() {
        QFETCH(QString, mode);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString engine = Fixture(temporary, mode == "failure" ? "matching" : "hang");
        QVERIFY(!engine.isEmpty());
        DelayedProcessStart delay(600);
        QVERIFY(delay.valid());
        auto probe = std::make_unique<ArchiveCodecProbe>();
        int calls = 0;
        ArchiveCodecProbeResult result;
        QElapsedTimer elapsed;
        elapsed.start();
        qint64 previous = 0, maximumGap = 0;
        QTimer heartbeat;
        heartbeat.setInterval(10);
        connect(&heartbeat, &QTimer::timeout, this, [&] {
            const auto now = elapsed.elapsed();
            maximumGap = qMax(maximumGap, now - previous);
            previous = now;
        });
        heartbeat.start();
        QVERIFY(probe->Start(
            engine, temporary.filePath("missing codec"),
            [&](auto value) {
                QCOMPARE(QThread::currentThread(), thread());
                result = std::move(value);
                ++calls;
            },
            mode == "deadline" ? 200 : 5000));
        QVERIFY2(elapsed.elapsed() < 250, "Native process creation blocked the caller");
        QTRY_VERIFY(delay.caller() != 0);
        QVERIFY(delay.caller() != GetCurrentThreadId());
        QVERIFY(probe->active());
        if (mode == "destroy") {
            heartbeat.stop();
            probe.reset();
            QCOMPARE(calls, 0);
        } else {
            if (mode == "cancel")
                probe->Cancel(); // Terminate the hanging child after delayed native creation.
            QTRY_COMPARE(calls, 1);
            heartbeat.stop();
            QCOMPARE(result.result, mode == "failure" ? E_FAIL : E_ABORT);
            if (mode == "failure") {
                QVERIFY(result.catalog);
                QVERIFY(!result.error.isEmpty());
            } else
                QVERIFY(!result.catalog);
            QVERIFY(!probe->active());
            QVERIFY2(maximumGap < 250, qPrintable(QString::number(maximumGap)));
        }
        QVERIFY(!delay.childRunning());
    }
#endif
    void nativeModuleLoadingKeepsEventLoopResponsive_data() {
        QTest::addColumn<QString>("mode");
        QTest::newRow("failure") << "failure";
        QTest::newRow("cancel") << "cancel";
        QTest::newRow("deadline") << "deadline";
        QTest::newRow("destroy") << "destroy";
    }
    void nativeModuleLoadingKeepsEventLoopResponsive() {
        QFETCH(QString, mode);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString engine = Fixture(temporary, "matching");
        const QString entered = temporary.filePath(QStringLiteral("начало 空"));
        const QString finished = temporary.filePath(QStringLiteral("конец 空"));
        const QByteArray oldEntered = qgetenv("SEVENZIP_SLOW_CODEC_ENTERED");
        const QByteArray oldFinished = qgetenv("SEVENZIP_SLOW_CODEC_FINISHED");
        const QByteArray oldDelay = qgetenv("SEVENZIP_SLOW_CODEC_DELAY");
        // Environment bytes use the local code page on Windows. Keep Unicode
        // filesystem paths intact with an ASCII transport to the test module.
        qputenv("SEVENZIP_SLOW_CODEC_ENTERED", entered.toUtf8().toBase64());
        qputenv("SEVENZIP_SLOW_CODEC_FINISHED", finished.toUtf8().toBase64());
        // Leave cold process startup headroom while still expiring during the
        // native query. The normal/cancel/destruction rows need only 600ms.
        qputenv("SEVENZIP_SLOW_CODEC_DELAY", mode == "deadline" ? "2100" : "600");
        const auto restore = qScopeGuard([&] {
            if (oldEntered.isNull())
                qunsetenv("SEVENZIP_SLOW_CODEC_ENTERED");
            else
                qputenv("SEVENZIP_SLOW_CODEC_ENTERED", oldEntered);
            if (oldFinished.isNull())
                qunsetenv("SEVENZIP_SLOW_CODEC_FINISHED");
            else
                qputenv("SEVENZIP_SLOW_CODEC_FINISHED", oldFinished);
            if (oldDelay.isNull())
                qunsetenv("SEVENZIP_SLOW_CODEC_DELAY");
            else
                qputenv("SEVENZIP_SLOW_CODEC_DELAY", oldDelay);
        });
        auto probe = std::make_unique<ArchiveCodecProbe>();
        ArchiveCodecProbeResult result;
        int calls = 0;
        QElapsedTimer elapsed;
        elapsed.start();
        qint64 previous = 0, maximumGap = 0;
        QTimer heartbeat;
        heartbeat.setInterval(10);
        connect(&heartbeat, &QTimer::timeout, this, [&] {
            const auto now = elapsed.elapsed();
            maximumGap = qMax(maximumGap, now - previous);
            previous = now;
        });
        heartbeat.start();
        QVERIFY(probe->Start(
            engine, QString::fromUtf8(SLOW_CODEC_FIXTURE),
            [&](auto value) {
                QCOMPARE(QThread::currentThread(), thread());
                result = std::move(value);
                ++calls;
            },
            mode == "deadline" ? 1500 : 5000));
        QTRY_VERIFY(QFileInfo::exists(entered));
        // Discovery remains owned and cancellable while the DLL query is running.
        QVERIFY(probe->active());
        QVERIFY(!QFileInfo::exists(finished));
        QVERIFY(!probe->Start(engine, "unused", {}));
        const auto processAndStart = elapsed.elapsed();
        if (mode == "destroy") {
            heartbeat.stop();
            probe.reset(); // Joins the loader; no queued completion may escape destruction.
            QVERIFY(QFileInfo::exists(finished));
            QCoreApplication::processEvents();
            QCOMPARE(calls, 0);
            return;
        }
        if (mode == "cancel")
            probe->Cancel();
        QTRY_COMPARE(calls, 1);
        heartbeat.stop();
        qInfo() << "native discovery ms: process/start" << processAndStart << "total"
                << elapsed.elapsed() << "heartbeat gap" << maximumGap;
        QVERIFY2(maximumGap < 250, qPrintable(QString::number(maximumGap)));
        QVERIFY(QFileInfo::exists(finished));
        QVERIFY(!probe->active());
        QCOMPARE(result.result, mode == "failure" ? E_FAIL : E_ABORT);
        QVERIFY(!result.error.isEmpty());
        if (mode != "failure")
            QVERIFY(!result.catalog);
        // Restart after joined cancellation/failure must deliver once again.
        QVERIFY(probe->Start(engine, temporary.filePath("missing module"), [&](auto) { ++calls; }));
        QTRY_COMPARE(calls, 2);
        QVERIFY(!probe->active());
    }

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
        QVERIFY(probe.active());
        QVERIFY(!probe.Start(engine, "unused", {}));
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
            QCOMPARE(result.catalog->version(), UInt32((MY_VER_MAJOR << 16) | MY_VER_MINOR));
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

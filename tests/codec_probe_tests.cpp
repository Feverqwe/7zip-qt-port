#include "archivecodecprobe.h"
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
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
    class SlowModule {
        const QByteArray entered_ = qgetenv("SEVENZIP_SLOW_CODEC_ENTERED");
        const QByteArray finished_ = qgetenv("SEVENZIP_SLOW_CODEC_FINISHED");
        const QByteArray delay_ = qgetenv("SEVENZIP_SLOW_CODEC_DELAY");
        static void Restore(const char *key, const QByteArray &value) {
            if (value.isNull())
                qunsetenv(key);
            else
                qputenv(key, value);
        }

      public:
        const QString Entered, Finished, Path;
        const bool Valid;
        SlowModule(const QTemporaryDir &temporary, int delay)
            : Entered(temporary.filePath(QStringLiteral("начало 空"))),
              Finished(temporary.filePath(QStringLiteral("конец 空"))),
              Path(temporary.filePath(QFileInfo(QString::fromUtf8(SLOW_CODEC_FIXTURE)).fileName())),
              Valid(QFile::copy(QString::fromUtf8(SLOW_CODEC_FIXTURE), Path)) {
            qputenv("SEVENZIP_SLOW_CODEC_ENTERED", Entered.toUtf8().toBase64());
            qputenv("SEVENZIP_SLOW_CODEC_FINISHED", Finished.toUtf8().toBase64());
            qputenv("SEVENZIP_SLOW_CODEC_DELAY", QByteArray::number(delay));
        }
        ~SlowModule() {
            Restore("SEVENZIP_SLOW_CODEC_ENTERED", entered_);
            Restore("SEVENZIP_SLOW_CODEC_FINISHED", finished_);
            Restore("SEVENZIP_SLOW_CODEC_DELAY", delay_);
        }
    };

  private slots:
    void nativeDiscoveryDoesNotRequireAConsole_data() {
        QTest::addColumn<QString>("mode");
        for (const QString &mode : {"missing", "hang", "wrong-version"})
            QTest::newRow(qPrintable(mode)) << mode;
    }
    void nativeDiscoveryDoesNotRequireAConsole() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; native discovery remains unverified");
#else
        QFETCH(QString, mode);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString engine =
            mode == "missing" ? temporary.filePath("missing engine") : Fixture(temporary, mode);
        QVERIFY(!engine.isEmpty());
        ArchiveCodecProbe probe;
        int calls = 0;
        ArchiveCodecProbeResult result;
        QVERIFY(probe.Start(engine, QString::fromUtf8(SEVENZIP_CODEC_MODULE), [&](auto value) {
            QCOMPARE(QThread::currentThread(), thread());
            result = std::move(value);
            ++calls;
        }));
        QTRY_COMPARE(calls, 1);
        QCOMPARE(result.result, S_OK);
        QVERIFY2(result.catalog, qPrintable(result.error));
        // A hanging/misreporting console must not prevent real native opening.
        CArchiveLink opened;
        const QString input = temporary.filePath("not an archive.zip");
        QFile file(input);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("ordinary bytes"), qint64(14));
        file.close();
        QCOMPARE(result.catalog->openArchive(opened, input, "zip", nullptr), S_FALSE);
        QVERIFY(!probe.active());
        QVERIFY(!QFileInfo::exists(engine + ".finished"));
#endif
    }

    void deadlineDoesNotDependOnGuiTimerDelivery() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        SlowModule module(temporary, 2100);
        QVERIFY(module.Valid);
        ArchiveCodecProbe probe;
        int calls = 0;
        ArchiveCodecProbeResult result;
        QVERIFY(probe.Start(
            temporary.filePath("absent console"), module.Path,
            [&](auto value) {
                QCOMPARE(QThread::currentThread(), thread());
                result = std::move(value);
                ++calls;
            },
            1500));
        // Withhold GUI timers/completion while the native query finishes.
        QElapsedTimer wait;
        wait.start();
        while (!QFileInfo::exists(module.Finished) && wait.elapsed() < 5000)
            QThread::msleep(5);
        QVERIFY(QFileInfo::exists(module.Entered));
        QVERIFY(QFileInfo::exists(module.Finished));
        QCOMPARE(calls, 0);
        QTRY_COMPARE(calls, 1);
        QCOMPARE(result.result, E_ABORT);
        QVERIFY(!result.catalog);
        QVERIFY(!result.error.isEmpty());
        QVERIFY(!probe.active());
    }
    void nativeModuleLoadingKeepsEventLoopResponsive_data() {
        QTest::addColumn<QString>("mode");
        for (const QString &mode : {"failure", "cancel", "deadline", "destroy"})
            QTest::newRow(qPrintable(mode)) << mode;
    }
    void nativeModuleLoadingKeepsEventLoopResponsive() {
        QFETCH(QString, mode);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        SlowModule module(temporary, mode == "deadline" ? 2100 : 600);
        QVERIFY(module.Valid);
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
        const QString engine = temporary.filePath("absent console");
        QVERIFY(probe->Start(
            engine, module.Path,
            [&](auto value) {
                QCOMPARE(QThread::currentThread(), thread());
                result = std::move(value);
                ++calls;
            },
            mode == "deadline" ? 1500 : 5000));
        QTRY_VERIFY(QFileInfo::exists(module.Entered));
        QVERIFY(probe->active());
        QVERIFY(!QFileInfo::exists(module.Finished));
        QVERIFY(!probe->Start(engine, "unused", {}));
        if (mode == "destroy") {
            heartbeat.stop();
            probe.reset();
            QVERIFY(QFileInfo::exists(module.Finished));
            QCoreApplication::processEvents();
            QCOMPARE(calls, 0);
            return;
        }
        if (mode == "cancel")
            probe->Cancel();
        QTRY_COMPARE(calls, 1);
        heartbeat.stop();
        QVERIFY2(maximumGap < 250, qPrintable(QString::number(maximumGap)));
        QVERIFY(QFileInfo::exists(module.Finished));
        QVERIFY(!probe->active());
        QCOMPARE(result.result, mode == "failure" ? E_FAIL : E_ABORT);
        QVERIFY(!result.error.isEmpty());
        if (mode != "failure")
            QVERIFY(!result.catalog);
        QVERIFY(probe->Start(engine, temporary.filePath("missing module"), [&](auto) { ++calls; }));
        QTRY_COMPARE(calls, 2);
        QVERIFY(!probe->active());
    }
    void cancellationBeforeLoadingPublishesNoCatalog() {
        QTemporaryDir temporary;
        ArchiveCodecProbe probe;
        int calls = 0;
        ArchiveCodecProbeResult result;
        QVERIFY(probe.Start(temporary.filePath("missing console"), "unused", [&](auto value) {
            result = std::move(value);
            ++calls;
        }));
        QVERIFY(!probe.Start("unused", "unused", {}));
        probe.Cancel();
        QTRY_COMPARE(calls, 1);
        QCOMPARE(result.result, E_ABORT);
        QVERIFY(!result.catalog);
        QVERIFY(!probe.active());
    }
};

QTEST_GUILESS_MAIN(CodecProbeTests)
#include "codec_probe_tests.moc"

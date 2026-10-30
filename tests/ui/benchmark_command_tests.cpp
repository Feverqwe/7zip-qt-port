#include "archive/codecs/archivecodeccatalog.h"
#include "archive/support/source_extract_fixture.h"
#include "filemanager/support/window_state_support.h"
#include "operations/qtoperationlauncher.h"
#include "support/operation_kill.h"
#include "support/operation_temp_cleanup.h"
#include "support/scoped_environment.h"
#include <QStandardPaths>
#include <utility>

namespace {
class BenchmarkBackend final : public ArchiveBackend {
  public:
    QString Barrier;
    QList<QtOperationResult> Results;
    bool CrashNext = false;
    unsigned Accepted = 0, Requests = 0;
    using ArchiveBackend::ArchiveBackend;
    QtOperationLauncher *startGuiBenchmark(bool total, QString &error) override {
        ++Requests;
        QtOperationJob job;
        job.Operation = QtOperationJob::Command::Benchmark;
        job.TotalMode = total;
        ScopedTestEnvironment barrier("SEVENZIP_OPERATION_BARRIER", Barrier);
        auto *operation =
            QtOperationLauncher::Launch(QStringLiteral(SEVENZIP_OPERATION_HOST), job, error);
        if (operation) {
            const bool crash = std::exchange(CrashNext, false);
            connect(operation, &QtOperationLauncher::accepted, this, [this, operation, crash] {
                ++Accepted;
                if (crash)
                    KillOperationProbe(operation->processId());
            });
            connect(operation, &QtOperationLauncher::finished, this,
                    [this](const QtOperationResult &result) { Results.append(result); });
        }
        return operation;
    }
};
} // namespace

class BenchmarkCommandTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    QString engine_, module_;
    quint32 version_ = 0;
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; Benchmark menu remains unverified");
#else
        engine_ = QStandardPaths::findExecutable("7z");
        QVERIFY(!engine_.isEmpty());
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        version_ = backend.codecCatalog()->version();
#endif
    }
    void cleanup() { QVERIFY(CleanupOperationTestTemps()); }
    void unavailableWithoutConfirmedProviders() {
        ArchiveBackend backend(nullptr, engine_);
        MainWindow window(&backend);
        window.show();
        QVERIFY(!action(window, "benchmarkAction")->isEnabled());
    }
    void managerCanCloseWhileNativeBenchmarkContinues_data() {
        QTest::addColumn<QString>("profile");
        QTest::addColumn<QtOperationResult::Outcome>("expected");
        QTest::newRow("success") << QString("bounded") << QtOperationResult::Outcome::Success;
        QTest::newRow("failure") << QString("failure") << QtOperationResult::Outcome::Failure;
        QTest::newRow("cancel") << QString("cancel") << QtOperationResult::Outcome::Cancelled;
    }
    void managerCanCloseWhileNativeBenchmarkContinues() {
        QFETCH(QString, profile);
        QFETCH(QtOperationResult::Outcome, expected);
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        ScopedTestEnvironment bounded("SEVENZIP_OPERATION_BENCHMARK_PROFILE", profile);
        BenchmarkBackend backend(nullptr, engine_);
        backend.Barrier = directory.filePath("continue");
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        {
            MainWindow window(&backend);
            window.show();
            auto *benchmark = action(window, "benchmarkAction");
            QTRY_VERIFY(benchmark->isEnabled());
            benchmark->trigger();
            QTRY_COMPARE(backend.Accepted, 1u);
            QCOMPARE(backend.Requests, 1u);
            QVERIFY(QApplication::activeModalWidget() == nullptr);
            QVERIFY(window.close());
        }
        QVERIFY(backend.Results.isEmpty());
        QVERIFY(SourceExtractFixture::writeFile(backend.Barrier, "continue"));
        QTRY_COMPARE_WITH_TIMEOUT(backend.Results.size(), 1, 15000);
        QCOMPARE(backend.Results[0].Status, expected);
    }
    void childCrashAllowsRetry() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        ScopedTestEnvironment bounded("SEVENZIP_OPERATION_BENCHMARK_PROFILE", "bounded");
        BenchmarkBackend backend(nullptr, engine_);
        backend.CrashNext = true;
        backend.Barrier = directory.filePath("unreleased");
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        auto *benchmark = action(window, "benchmarkAction");
        QTRY_VERIFY(benchmark->isEnabled());
        unsigned crashMessages = 0;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                ++crashMessages;
                box->accept();
            }
        });
        responder.start(10);
        benchmark->trigger();
        QTRY_COMPARE(backend.Results.size(), 1);
        QCOMPARE(backend.Results[0].Status, QtOperationResult::Outcome::Crashed);
        QCOMPARE(crashMessages, 1u);
        QVERIFY(window.isVisible());
        QTRY_VERIFY(benchmark->isEnabled());
        backend.Barrier.clear();
        benchmark->trigger();
        QTRY_COMPARE_WITH_TIMEOUT(backend.Results.size(), 2, 15000);
        QCOMPARE(backend.Results[1].Status, QtOperationResult::Outcome::Success);
    }
};
QTEST_MAIN(BenchmarkCommandTests)
#include "benchmark_command_tests.moc"

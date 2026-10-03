#include "archivecodeccatalog.h"
#include "benchmark_services_fixture.h"
#include "window_state_support.h"
#include <QStandardPaths>

namespace {
class BenchmarkBackend final : public ArchiveBackend {
    class Services final : public IQtBenchmarkServices {
        BenchmarkBackend &owner_;

      public:
        explicit Services(BenchmarkBackend &owner) : owner_(owner) {}
        ~Services() override { owner_.JoinedAtRelease = owner_.Fixture.Active == 0; }
        bool GetRamSize(size_t &size) const override { return owner_.Fixture.GetRamSize(size); }
        HRESULT Run(const CExternalCodecs *codecs, IBenchPrintCallback *print,
                    IBenchCallback *results, const CObjectVector<CProperty> &props,
                    UInt32 iterations, bool multiDict, IBenchFreqCallback *frequency) override {
            return owner_.Fixture.Run(codecs, print, results, props, iterations, multiDict,
                                      frequency);
        }
    };

  public:
    FixtureBenchmarkServices Fixture;
    bool JoinedAtRelease = false;
    explicit BenchmarkBackend(const QString &engine) : ArchiveBackend(nullptr, engine) {
        Fixture.Hold = true;
        Fixture.Ram = size_t(8) << 30; // No allocation: permit original machine defaults.
    }
    std::unique_ptr<IQtBenchmarkServices> benchmarkServices() const override {
        return std::make_unique<Services>(*const_cast<BenchmarkBackend *>(this));
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
        engine_ = QStandardPaths::findExecutable("7zz");
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
    void unavailableWithoutConfirmedProviders() {
        ArchiveBackend backend(nullptr, engine_);
        MainWindow window(&backend);
        window.show();
        QVERIFY(!action(window, "benchmarkAction")->isEnabled());
    }
    void realMenuJoinsAndRestoresWindow_data() {
        QTest::addColumn<bool>("failure");
        QTest::newRow("cancel") << false;
        QTest::newRow("engine-failure") << true;
    }
    void realMenuJoinsAndRestoresWindow() {
        QFETCH(bool, failure);
        BenchmarkBackend backend(engine_);
        if (failure)
            backend.Fixture.Result = E_FAIL;
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        auto *benchmark = action(window, "benchmarkAction");
        QTRY_VERIFY(benchmark->isEnabled());
        QStringList errors;
        bool opened = false;
        bool attemptedClose = false, closeWasDeferred = false;
        QSignalSpy engines(&backend, &ArchiveBackend::engineStarted);
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errors << box->text();
                box->accept();
            } else if (auto *view = qobject_cast<QDialog *>(QApplication::activeModalWidget());
                       view && view->objectName() == "IDD_BENCH") {
                opened = true;
                if ((!failure && backend.Fixture.Active) || !errors.isEmpty()) {
                    attemptedClose = true;
                    closeWasDeferred = !window.close() && window.isVisible();
                    view->close();
                }
            }
        });
        responder.start(1);
        benchmark->trigger();
        responder.stop();
        QVERIFY(opened);
        QVERIFY(attemptedClose && closeWasDeferred);
        QCOMPARE(backend.Fixture.Calls.load(), 1u);
        QCOMPARE(backend.Fixture.Active.load(), 0u);
        QCOMPARE(backend.Fixture.Canceled.load(), failure ? 0u : 1u);
        QVERIFY(backend.JoinedAtRelease);
        QCOMPARE(errors.isEmpty(), !failure);
        QCOMPARE(engines.size(), 0); // Original native Bench, no CLI benchmark process.
        QVERIFY(QApplication::activeModalWidget() == nullptr);
        QVERIFY(window.isVisible());
        QTRY_VERIFY(benchmark->isEnabled());
        window.close();
        QVERIFY(!window.isVisible());
    }
};
QTEST_MAIN(BenchmarkCommandTests)
#include "benchmark_command_tests.moc"

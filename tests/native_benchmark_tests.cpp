#include "CPP/7zip/UI/Common/Bench.h"
#include "CPP/7zip/UI/GUI/BenchmarkDialog.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include <QApplication>
#include <QDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTimer>
#include <QtTest>

class BenchResults final : public IBenchCallback {
  public:
    unsigned EncCalls = 0, DecCalls = 0;
    bool Cancel = false;
    CBenchInfo Enc, Dec;
    HRESULT SetEncodeResult(const CBenchInfo &info, bool final) override {
        ++EncCalls;
        if (final)
            Enc = info;
        return Cancel ? E_ABORT : S_OK;
    }
    HRESULT SetDecodeResult(const CBenchInfo &info, bool final) override {
        ++DecCalls;
        if (final)
            Dec = info;
        return Cancel ? E_ABORT : S_OK;
    }
};

class NativeBenchmarkTests final : public QObject {
    Q_OBJECT
    ArchiveBackend backend_{nullptr, QStandardPaths::findExecutable("7zz")};
    static CObjectVector<CProperty> boundedProperties() {
        CObjectVector<CProperty> props;
        for (const auto &pair : {qMakePair(L"mt", L"1"), qMakePair(L"d18", L""),
                                 qMakePair(L"timems", L"50"), qMakePair(L"tic", L"1")}) {
            CProperty &prop = props.AddNew();
            prop.Name = pair.first;
            prop.Value = pair.second;
        }
        return props;
    }

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official codec module unavailable; native benchmark remains unverified");
#else
        QSignalSpy ready(&backend_, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend_.discoverCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE)));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
#endif
    }
    void sourceCallbacksExposeActualEngineResults() {
        const auto catalog = backend_.codecCatalog();
        CExternalCodecs providers;
        providers.GetCodecs =
            static_cast<ICompressCodecsInfo *>(&const_cast<CCodecs &>(catalog->nativeCodecs()));
        QCOMPARE(providers.Load(), S_OK);
        BenchResults results;
        QCOMPARE(Bench(&providers, nullptr, &results, boundedProperties(), 1, false), S_OK);
        QVERIFY(results.EncCalls > 0);
        QVERIFY(results.DecCalls > 0);
        QVERIFY(results.Enc.Get_UnpackSize_Full() > 0);
        QVERIFY(results.Dec.Get_UnpackSize_Full() > 0);
        QVERIFY(results.Enc.GetUnpackSizeSpeed() > 0);
        QVERIFY(results.Dec.GetUnpackSizeSpeed() > 0);
        QVERIFY(GetBenchMemoryUsage(1, 5, UInt64(1) << 18, false) < (UInt64(16) << 20));
    }
    void cancelStopsOriginalEngineCallback() {
        const auto catalog = backend_.codecCatalog();
        CExternalCodecs providers;
        providers.GetCodecs =
            static_cast<ICompressCodecsInfo *>(&const_cast<CCodecs &>(catalog->nativeCodecs()));
        QCOMPARE(providers.Load(), S_OK);
        BenchResults results;
        results.Cancel = true;
        QCOMPARE(Bench(&providers, nullptr, &results, boundedProperties(), 1, false), E_ABORT);
        QVERIFY(results.EncCalls > 0);
    }
    void originalDialogPresentsCompletedNativeResultsAndJoins() {
        const auto catalog = backend_.codecCatalog();
        CExternalCodecs providers;
        providers.GetCodecs =
            static_cast<ICompressCodecsInfo *>(&const_cast<CCodecs &>(catalog->nativeCodecs()));
        QCOMPARE(providers.Load(), S_OK);
        QString rating;
        QStringList errors;
        bool timedOut = false;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errors.append(box->text());
                box->accept();
                return;
            }
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog || dialog->objectName() != "IDD_BENCH")
                return;
            auto *stop = dialog->findChild<QPushButton *>("IDB_STOP");
            if (stop && !stop->isEnabled()) {
                rating = dialog->findChild<QLabel *>("benchmarkValue113")->text();
                dialog->reject();
            }
        });
        QTimer watchdog;
        watchdog.setSingleShot(true);
        connect(&watchdog, &QTimer::timeout, this, [&] {
            timedOut = true;
            if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget()))
                dialog->reject();
        });
        responder.start(1);
        // The original GUI also runs CPU-frequency calibration. timems bounds
        // codec work, not that loop (Bench grows it to 2048 million commands).
        watchdog.start(20000);
        QCOMPARE(Benchmark(&providers, boundedProperties(), 1), S_OK);
        QVERIFY(!timedOut);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
        QVERIFY(rating.endsWith(" GIPS"));
        QVERIFY(QApplication::activeModalWidget() == nullptr);
    }
};
QTEST_MAIN(NativeBenchmarkTests)
#include "native_benchmark_tests.moc"

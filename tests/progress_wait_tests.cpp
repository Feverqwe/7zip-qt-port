#include "Windows/Synchronization.h"
#include "Windows/Thread.h"
#include "qtprogresswait.h"
#include <QtTest>

namespace {
struct CWorker {
    CQtProgressStartWait wait;
    NWindows::NSynchronization::CManualResetEvent ready, release;
    NWindows::CThread thread;
    bool requested = false;
    static THREAD_FUNC_DECL Run(void *parameter) {
        auto &worker = *static_cast<CWorker *>(parameter);
        worker.ready.Set();
        worker.release.Lock();
        if (worker.requested)
            worker.wait.SignalRequested();
        else
            worker.wait.SignalFinished();
        return 0;
    }
    ~CWorker() {
        if (release.IsCreated())
            release.Set();
        if (thread.IsCreated())
            thread.Wait_Close();
    }
};
} // namespace

class ProgressWaitTests : public QObject {
    Q_OBJECT
  private slots:
    void latchedSignalsAndFinishPrecedence() {
        using EResult = CQtProgressStartWait::EResult;
        CQtProgressStartWait wait;
        QCOMPARE(wait.Wait(0), EResult::Timeout);
        wait.SignalRequested();
        QCOMPARE(wait.Wait(0), EResult::Requested);
        wait.SignalRequested();
        QCOMPARE(wait.Wait(0), EResult::Requested);
        wait.SignalFinished();
        QCOMPARE(wait.Wait(0), EResult::Finished);
        wait.SignalFinished();
        QCOMPARE(wait.Wait(0), EResult::Finished);
        CQtProgressStartWait finished;
        finished.SignalFinished();
        finished.SignalRequested();
        QCOMPARE(finished.Wait(0), EResult::Finished);
    }
    void nativeWorkerWakeAndJoin_data() {
        QTest::addColumn<bool>("requested");
        QTest::newRow("creation-request") << true;
        QTest::newRow("worker-completion") << false;
    }
    void nativeWorkerWakeAndJoin() {
        QFETCH(bool, requested);
        CWorker worker;
        worker.requested = requested;
        QCOMPARE(worker.ready.Create(), WRes(0));
        QCOMPARE(worker.release.Create(), WRes(0));
        QCOMPARE(worker.thread.Create(CWorker::Run, &worker), WRes(0));
        QCOMPARE(worker.ready.Lock(), WRes(0));
        QCOMPARE(worker.wait.Wait(1), CQtProgressStartWait::EResult::Timeout);
        QCOMPARE(worker.release.Set(), WRes(0));
        QCOMPARE(worker.wait.Wait(3000), requested ? CQtProgressStartWait::EResult::Requested
                                                   : CQtProgressStartWait::EResult::Finished);
        QCOMPARE(worker.thread.Wait_Close(), WRes(0));
        QVERIFY(!worker.thread.IsCreated());
    }
};

QTEST_GUILESS_MAIN(ProgressWaitTests)
#include "progress_wait_tests.moc"

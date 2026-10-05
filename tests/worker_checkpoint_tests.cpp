#include "qtworkerprogress.h"
#include <QScopeGuard>
#include <QSemaphore>
#include <QThread>
#include <QtTest>
#include <atomic>

namespace {
class CheckpointThread final : public QThread {
    QtWorkerProgress &progress_;
    void run() override {
        Entered.release();
        Result = progress_.checkpoint();
        Returned.store(true);
    }

  public:
    QSemaphore Entered;
    std::atomic<bool> Returned{false};
    HRESULT Result = E_FAIL;
    explicit CheckpointThread(QtWorkerProgress &progress) : progress_(progress) {}
    ~CheckpointThread() override {
        progress_.cancel();
        wait();
    }
};
} // namespace

class WorkerCheckpointTests final : public QObject {
    Q_OBJECT
  private slots:
    void stopTakesPrecedenceAndNewOwnerStartsClean() {
        QtWorkerProgress stopped;
        QCOMPARE(stopped.checkpoint(), S_OK);
        stopped.cancel();
        stopped.setPaused(true);
        QCOMPARE(stopped.checkpoint(), E_ABORT);
        stopped.setPaused(false);
        QCOMPARE(stopped.checkpoint(), E_ABORT);
        QtWorkerProgress next;
        QCOMPARE(next.checkpoint(), S_OK);
    }
    void pausedCheckpointResumes() {
        QtWorkerProgress progress;
        progress.setPaused(true);
        CheckpointThread worker(progress);
        worker.start();
        QVERIFY(worker.Entered.tryAcquire(1, 1000));
        QTest::qWait(20);
        QVERIFY(!worker.Returned.load());
        progress.setPaused(false);
        QVERIFY(worker.wait(1000));
        QCOMPARE(worker.Result, S_OK);
    }
    void cancelWakesPausedCheckpoint() {
        QtWorkerProgress progress;
        progress.setPaused(true);
        CheckpointThread worker(progress);
        worker.start();
        QVERIFY(worker.Entered.tryAcquire(1, 1000));
        QTest::qWait(20);
        QVERIFY(!worker.Returned.load());
        progress.cancel();
        QVERIFY(worker.wait(1000));
        QCOMPARE(worker.Result, E_ABORT);
    }
    void resumeBeforeWaitIsNotLost() {
        QtWorkerProgress progress;
        for (unsigned i = 0; i < 32; ++i) {
            progress.setPaused(true);
            // Resume can occur before the checkpoint starts waiting. The
            // source event retains that signal; the source flags decide exit.
            progress.setPaused(false);
            QCOMPARE(progress.checkpoint(), S_OK);
        }
        progress.setPaused(true);
        CheckpointThread worker(progress);
        worker.start();
        QVERIFY(worker.Entered.tryAcquire(1, 1000));
        progress.cancel();
        QVERIFY(worker.wait(1000));
        QCOMPARE(worker.Result, E_ABORT);
    }
    void cancelledOwnerDoesNotPublish() {
        QtWorkerProgress progress;
        bool called = false;
        QCOMPARE(progress.withPublicationGuard([&] {
            called = true;
            return S_FALSE;
        }),
                 S_FALSE);
        QVERIFY(called);
        called = false;
        progress.cancel();
        QCOMPARE(progress.withPublicationGuard([&] {
            called = true;
            return S_OK;
        }),
                 E_ABORT);
        QVERIFY(!called);
    }
    void cancellationWaitsForPublicationAlreadyInProgress() {
        QtWorkerProgress progress;
        QSemaphore publicationEntered, finishPublication, cancelEntered;
        std::atomic<bool> cancelled{false};
        HRESULT result = E_FAIL;
        const auto publisher = std::unique_ptr<QThread>(QThread::create([&] {
            result = progress.withPublicationGuard([&] {
                publicationEntered.release();
                finishPublication.acquire();
                return S_FALSE;
            });
        }));
        const auto canceller = std::unique_ptr<QThread>(QThread::create([&] {
            cancelEntered.release();
            progress.cancel();
            cancelled.store(true);
        }));
        const auto join = qScopeGuard([&] {
            finishPublication.release();
            publisher->wait();
            canceller->wait();
        });
        publisher->start();
        QVERIFY(publicationEntered.tryAcquire(1, 1000));
        canceller->start();
        QVERIFY(cancelEntered.tryAcquire(1, 1000));
        QTest::qWait(20);
        QVERIFY(!cancelled.load());
        finishPublication.release();
        QVERIFY(publisher->wait(1000));
        QVERIFY(canceller->wait(1000));
        QCOMPARE(result, S_FALSE);
        QVERIFY(cancelled.load());
        QCOMPARE(progress.checkpoint(), E_ABORT);
    }
};
QTEST_GUILESS_MAIN(WorkerCheckpointTests)
#include "worker_checkpoint_tests.moc"

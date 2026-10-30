#include "filemanager/panels/Panel.h"
#include "operations/qtexternalguicall.h"
#include <QScopeGuard>
#include <QThread>
#include <QWidget>
#include <QtTest>
#include <atomic>
#include <stdexcept>
#include <thread>

class ExternalGuiCallTests final : public QObject {
    Q_OBJECT
  private slots:
    void exceptionsReturnToWaitingWorker_data() {
        QTest::addColumn<bool>("predicateFails");
        QTest::newRow("body") << false;
        QTest::newRow("readiness-predicate") << true;
    }
    void exceptionsReturnToWaitingWorker() {
        QFETCH(bool, predicateFails);
        QWidget window;
        CExitEventLauncher launcher;
        std::atomic_bool completed{false};
        bool caught = false, deliveredOnGui = false;
        std::thread worker([&] {
            try {
                QtExternalGuiCall(
                    &window, launcher,
                    [&]() -> int {
                        deliveredOnGui = QThread::currentThread() == window.thread();
                        throw std::runtime_error("GUI request failed");
                    },
                    [&] {
                        if (predicateFails) {
                            deliveredOnGui = QThread::currentThread() == window.thread();
                            throw std::runtime_error("GUI request failed");
                        }
                        return true;
                    });
            } catch (const std::runtime_error &error) {
                caught = QByteArray(error.what()) == "GUI request failed";
            }
            completed.store(true);
        });
        const auto join = qScopeGuard([&] {
            launcher.Exit(false);
            worker.join();
        });
        QTRY_VERIFY(completed.load());
        QVERIFY(caught);
        QVERIFY(deliveredOnGui);
    }
    void cancelBeforeReadinessDoesNotRunBody() {
        QWidget window;
        CExitEventLauncher launcher;
        std::atomic_bool completed{false}, readinessObserved{false};
        bool invoked = false;
        int result = -1;
        std::thread worker([&] {
            result = QtExternalGuiCall(
                &window, launcher,
                [&] {
                    invoked = true;
                    return 1;
                },
                [&] {
                    readinessObserved.store(true);
                    return false;
                });
            completed.store(true);
        });
        const auto join = qScopeGuard([&] {
            launcher.Exit(false);
            worker.join();
        });
        QTRY_VERIFY(readinessObserved.load());
        launcher.Exit(false);
        QTRY_VERIFY(completed.load());
        QCOMPARE(result, 0);
        QVERIFY(!invoked);
    }
};

QTEST_MAIN(ExternalGuiCallTests)
#include "external_gui_call_tests.moc"

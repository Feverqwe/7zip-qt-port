#include "operations/qtexternalguicall.h"
#include "filemanager/panels/Panel.h"
#include <QCoreApplication>
#include <QMetaObject>
#include <QSemaphore>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include <atomic>
#include <exception>
#include <memory>

namespace {
struct Request {
    QSemaphore ready;
    // Pending -> Running or Cancelled. A delivered body keeps its worker
    // record alive until completion; modal closure uses CanCloseNow.
    std::atomic_int phase{0};
    int result = 0;
    std::exception_ptr error;
};

void Deliver(const std::shared_ptr<Request> &request, QObject *context,
             const std::function<int()> &body, const std::function<bool()> &ready) {
    int pending = 0;
    if (!request->phase.compare_exchange_strong(pending, 3)) {
        request->ready.release();
        return;
    }
    try {
        if (ready && !ready()) {
            request->phase.store(0);
            QTimer::singleShot(25, context, [request, context, body, ready] {
                Deliver(request, context, body, ready);
            });
            return;
        }
        request->phase.store(1);
        request->result = body();
    } catch (...) {
        // Retire delivery even if readiness or rescheduling failed first.
        request->phase.store(1);
        request->error = std::current_exception();
    }
    request->ready.release();
}
} // namespace

int QtExternalGuiCall(QWidget *window, CExitEventLauncher &launcher,
                      const std::function<int()> &body, const std::function<bool()> &ready) {
    if (launcher.Exiting())
        return 0;
    QObject *context = window ? static_cast<QObject *>(window) : QCoreApplication::instance();
    if (!context)
        return 0;
    if (QThread::currentThread() == context->thread())
        return body();
    const auto request = std::make_shared<Request>();
    QMetaObject::invokeMethod(
        context, [request, context, body, ready] { Deliver(request, context, body, ready); },
        Qt::QueuedConnection);
    while (!request->ready.tryAcquire(1, 25)) {
        if (launcher.Exiting()) {
            int pending = 0;
            if (request->phase.compare_exchange_strong(pending, 2) || pending == 2)
                return 0;
        }
    }
    if (request->error)
        std::rethrow_exception(request->error);
    return request->result;
}

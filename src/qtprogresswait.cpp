#include "qtprogresswait.h"
#include <QDeadlineTimer>
#include <QMutexLocker>

void CQtProgressStartWait::SignalRequested() {
    QMutexLocker lock(&_mutex);
    _requested = true;
    _condition.wakeAll();
}
void CQtProgressStartWait::SignalFinished() {
    QMutexLocker lock(&_mutex);
    _finished = true;
    _condition.wakeAll();
}
CQtProgressStartWait::EResult CQtProgressStartWait::Wait(unsigned milliseconds) {
    QMutexLocker lock(&_mutex);
    const QDeadlineTimer deadline(milliseconds, Qt::PreciseTimer);
    while (!_finished && !_requested) {
        if (!_condition.wait(&_mutex, deadline))
            break;
    }
    // WaitForMultipleObjects gives the lowest handle index precedence.
    if (_finished)
        return EResult::Finished;
    if (_requested)
        return EResult::Requested;
    return EResult::Timeout;
}

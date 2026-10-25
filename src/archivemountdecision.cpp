#include "archivemountdecision.h"
#include <QMutexLocker>

void ArchiveMountDecision::answer(bool allow) {
    QMutexLocker lock(&mutex_);
    if (!answer_)
        answer_ = allow;
    ready_.wakeAll();
}
bool ArchiveMountDecision::wait(const std::atomic_bool &cancelled) {
    QMutexLocker lock(&mutex_);
    while (!answer_ && !cancelled)
        ready_.wait(&mutex_, 50);
    return !cancelled && answer_.value_or(false);
}

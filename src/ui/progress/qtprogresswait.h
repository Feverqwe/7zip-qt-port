// Narrow portable service for ProgressDialog2's Win32 two-handle startup wait.
#pragma once
#include <QMutex>
#include <QWaitCondition>

class CQtProgressStartWait {
    QMutex _mutex;
    QWaitCondition _condition;
    bool _requested = false;
    bool _finished = false;

  public:
    enum class EResult { Finished, Requested, Timeout };
    void SignalRequested();
    void SignalFinished();
    EResult Wait(unsigned milliseconds);
};

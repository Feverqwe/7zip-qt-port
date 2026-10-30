#pragma once
#include "Common/MyWindows.h"
#include <functional>
#include <memory>

// One checkpoint thread per joined operation. The source CProgressSync owns
// pause/stop state; this adapter keeps its widget declarations out of workers.
class QtWorkerProgress final {
    struct State;
    std::unique_ptr<State> state_;

  public:
    QtWorkerProgress();
    ~QtWorkerProgress();
    void cancel();
    void setPaused(bool paused);
    HRESULT checkpoint();
    // Publication and cancel keep their existing ordering. The joined caller's
    // action must not re-enter this progress owner.
    HRESULT withPublicationGuard(const std::function<HRESULT()> &action);
};

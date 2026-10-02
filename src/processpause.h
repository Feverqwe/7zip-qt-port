// Narrow native service for a live child owned by ArchiveBackend/QProcess.
#pragma once
#include "Common/MyTypes.h"
#include "Common/MyWindows.h"
#include <memory>

class CProcessPause {
    struct CState;
    std::unique_ptr<CState> _state;

  public:
    CProcessPause();
    ~CProcessPause();
    // The caller retains process ownership and resets after exit, before PID reuse.
    void Attach(UInt32 ownedProcessId);
    HRESULT SetPaused(bool paused);
    void ResetAfterExit();
};

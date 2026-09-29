// Native POSIX presentation service for upstream ProgressDialog2 taskbar calls.
#pragma once
#include "Common/MyTypes.h"
#include <memory>

class QWidget;

#ifndef _WIN32
// Native taskbar ABI constants from upstream MyWindowsNew.h (0766b73).
typedef enum TBPFLAG {
    TBPF_NOPROGRESS = 0,
    TBPF_INDETERMINATE = 0x1,
    TBPF_NORMAL = 0x2,
    TBPF_ERROR = 0x4,
    TBPF_PAUSED = 0x8
} TBPFLAG;

// GUI-thread owner. Cocoa presents the selected window's progress in the
// application's Dock badge; the OS defines its appearance.
class CQtTaskbarProgress {
    class CImpl;
    std::unique_ptr<CImpl> _impl;

  public:
    CQtTaskbarProgress();
    ~CQtTaskbarProgress();
    HRESULT HrInit();
    HRESULT SetProgressState(QWidget *window, TBPFLAG state);
    HRESULT SetProgressValue(QWidget *window, UInt64 completed, UInt64 total);
};
#endif

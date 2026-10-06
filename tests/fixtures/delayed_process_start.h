#pragma once

#include <atomic>
#include <windows.h>

// Own-process fixture: delay Qt's imported CreateProcessW, then call the real API.
// No product injection point or system-wide hook is involved.
class DelayedProcessStart final {
    void **slot_ = nullptr;
    void *original_ = nullptr;
    DWORD protection_ = 0;
    unsigned delayMs_;
    std::atomic<DWORD> caller_{0};
    std::atomic<DWORD> child_{0};
    static DelayedProcessStart *active_;
    static BOOL WINAPI Create(LPCWSTR application, LPWSTR arguments, LPSECURITY_ATTRIBUTES process,
                              LPSECURITY_ATTRIBUTES thread, BOOL inherit, DWORD flags,
                              LPVOID environment, LPCWSTR directory, LPSTARTUPINFOW startup,
                              LPPROCESS_INFORMATION result);

  public:
    explicit DelayedProcessStart(unsigned delayMs);
    ~DelayedProcessStart();
    bool valid() const { return slot_ != nullptr; }
    DWORD caller() const { return caller_.load(); }
    bool childRunning() const;
};

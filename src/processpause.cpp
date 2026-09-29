#include "processpause.h"
#ifdef _WIN32
#include "Windows/Thread.h"
#include <tlhelp32.h>
#include <vector>
#else
#include <cerrno>
#include <climits>
#include <csignal>
#endif

struct CProcessPause::CState {
    UInt32 ProcessId = 0;
    bool Paused = false;
#ifdef _WIN32
    struct CThreadEntry {
        DWORD Id;
        std::unique_ptr<NWindows::CThread> Thread;
    };
    std::vector<CThreadEntry> Threads;
    HRESULT Resume() {
        HRESULT result = S_OK;
        for (auto it = Threads.begin(); it != Threads.end();) {
            if (it->Thread->Resume() == DWORD(-1)) {
                DWORD exitCode = STILL_ACTIVE;
                const DWORD error = ::GetLastError();
                if (!::GetExitCodeThread(*it->Thread, &exitCode) || exitCode == STILL_ACTIVE) {
                    result = HRESULT_FROM_WIN32(error);
                    ++it;
                    continue;
                }
            }
            // Release precisely our single suspend increment, not another owner's.
            it = Threads.erase(it);
        }
        // A failed resume can leave retained increments on only some threads.
        // That is an error/retry state, not a successfully paused process.
        Paused = false;
        return result;
    }
    HRESULT Suspend() {
        for (unsigned pass = 0; pass != 3; ++pass) {
            HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (snapshot == INVALID_HANDLE_VALUE)
                return HRESULT_FROM_WIN32(::GetLastError());
            THREADENTRY32 entry{};
            entry.dwSize = sizeof(entry);
            bool added = false;
            HRESULT result = S_OK;
            if (::Thread32First(snapshot, &entry)) {
                do {
                    if (entry.th32OwnerProcessID != ProcessId)
                        continue;
                    bool known = false;
                    for (const auto &thread : Threads)
                        if (thread.Id == entry.th32ThreadID)
                            known = true;
                    if (known)
                        continue;
                    HANDLE handle = ::OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION,
                                                 FALSE, entry.th32ThreadID);
                    if (!handle) {
                        const DWORD error = ::GetLastError();
                        if (error == ERROR_INVALID_PARAMETER)
                            continue; // Thread exited after the snapshot.
                        result = HRESULT_FROM_WIN32(error);
                        break;
                    }
                    auto thread = std::make_unique<NWindows::CThread>();
                    thread->Attach(handle);
                    if (thread->Suspend() == DWORD(-1)) {
                        result = HRESULT_FROM_WIN32(::GetLastError());
                        break;
                    }
                    Threads.push_back({entry.th32ThreadID, std::move(thread)});
                    added = true;
                } while (::Thread32Next(snapshot, &entry));
                if (result == S_OK && ::GetLastError() != ERROR_NO_MORE_FILES)
                    result = HRESULT_FROM_WIN32(::GetLastError());
            } else
                result = HRESULT_FROM_WIN32(::GetLastError());
            ::CloseHandle(snapshot);
            if (result != S_OK)
                return result;
            if (!added) {
                if (Threads.empty())
                    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                Paused = true;
                return S_OK;
            }
        }
        return HRESULT_FROM_WIN32(ERROR_RETRY);
    }
#endif
};

CProcessPause::CProcessPause() : _state(std::make_unique<CState>()) {}
CProcessPause::~CProcessPause() { SetPaused(false); }
void CProcessPause::Attach(UInt32 ownedProcessId) {
    // Caller attaches only a newly started child after the previous exit/reset.
    _state->ProcessId = ownedProcessId;
}
void CProcessPause::ResetAfterExit() {
#ifdef _WIN32
    _state->Threads.clear();
#endif
    _state->ProcessId = 0;
    _state->Paused = false;
}
HRESULT CProcessPause::SetPaused(bool paused) {
    if (_state->Paused == paused
#ifdef _WIN32
        && (paused || _state->Threads.empty())
#endif
    )
        return S_OK;
    if (_state->ProcessId == 0)
        return E_INVALIDARG;
#ifdef _WIN32
    if (!paused)
        return _state->Resume();
    const HRESULT result = _state->Suspend();
    if (result != S_OK) {
        const HRESULT rollback = _state->Resume();
        return rollback == S_OK ? result : rollback;
    }
#else
    if (_state->ProcessId > UInt32(INT_MAX))
        return E_INVALIDARG;
    if (::kill(static_cast<pid_t>(_state->ProcessId), paused ? SIGSTOP : SIGCONT) != 0)
        return HRESULT_FROM_WIN32(errno);
#endif
    _state->Paused = paused;
    return S_OK;
}

#include "Common/MyTypes.h"
#include "Windows/Handle.h"
#include "processpause.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <new>
#include <string>

namespace {
thread_local int allocationsBeforeFailure = -1;
thread_local bool allocationFailed = false;
} // namespace

// This executable has no Qt event loop or other allocating parent threads.
// Arm only around the public pause call; every other allocation is ordinary.
void *operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        allocationsBeforeFailure = -1;
        allocationFailed = true;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
    if (void *memory = std::malloc(size ? size : 1))
        return memory;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void *memory, std::size_t) noexcept { std::free(memory); }

namespace {
DWORD WINAPI tick(void *event) {
    for (;;) {
        ::SetEvent(event);
        ::Sleep(10);
    }
}

struct Child {
    NWindows::CHandle Process;
    ~Child() {
        if (Process.IsCreated()) {
            ::TerminateProcess(Process, 0);
            ::WaitForSingleObject(Process, 5000);
        }
    }
};

bool check(bool condition, int budget, const char *message) {
    if (!condition)
        std::fprintf(stderr, "allocation budget %d: %s\n", budget, message);
    return condition;
}

bool attempt(int budget, bool &injected) {
    SECURITY_ATTRIBUTES attributes{sizeof(attributes), nullptr, TRUE};
    NWindows::CHandle events[2];
    for (auto &event : events) {
        event.Attach(::CreateEventW(&attributes, FALSE, FALSE, nullptr));
        if (!check(event.IsCreated(), budget, "cannot create tick event"))
            return false;
    }
    wchar_t executable[32768];
    if (!check(::GetModuleFileNameW(nullptr, executable, 32768) != 0, budget,
               "cannot locate child executable"))
        return false;
    std::wstring command = L"\"" + std::wstring(executable) + L"\" --child " +
                           std::to_wstring(reinterpret_cast<std::uintptr_t>(HANDLE(events[0]))) +
                           L" " +
                           std::to_wstring(reinterpret_cast<std::uintptr_t>(HANDLE(events[1])));
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION info{};
    if (!check(::CreateProcessW(executable, command.data(), nullptr, nullptr, TRUE,
                                CREATE_NO_WINDOW, nullptr, nullptr, &startup, &info),
               budget, "cannot start ticking child"))
        return false;
    Child child;
    child.Process.Attach(info.hProcess);
    ::CloseHandle(info.hThread);
    for (auto &event : events)
        if (!check(::WaitForSingleObject(event, 5000) == WAIT_OBJECT_0, budget,
                   "child did not start both tick threads"))
            return false;
    DWORD handlesBefore = 0;
    if (!check(::GetProcessHandleCount(::GetCurrentProcess(), &handlesBefore), budget,
               "cannot read initial handle count"))
        return false;
    bool valid = true;
    {
        CProcessPause pause;
        pause.Attach(info.dwProcessId);
        HRESULT result = E_UNEXPECTED;
        allocationFailed = false;
        allocationsBeforeFailure = budget;
        try {
            result = pause.SetPaused(true);
        } catch (...) {
            // Keep checking rollback and handles even when the HRESULT contract fails.
        }
        allocationsBeforeFailure = -1;
        injected = allocationFailed;
        valid &= check(result == (injected ? E_OUTOFMEMORY : S_OK), budget,
                       "pause did not return the expected HRESULT");
        valid &= check(pause.SetPaused(false) == S_OK, budget, "resume failed");
    }
    // Discard ticks published before suspension/resume. Each thread must now
    // produce a fresh tick, including after a failed pause and owner destruction.
    for (auto &event : events)
        ::ResetEvent(event);
    for (auto &event : events)
        valid &= check(::WaitForSingleObject(event, 1000) == WAIT_OBJECT_0, budget,
                       "a child thread remained suspended");
    DWORD handlesAfter = 0;
    valid &= check(::GetProcessHandleCount(::GetCurrentProcess(), &handlesAfter) &&
                       handlesBefore == handlesAfter,
                   budget, "pause leaked a snapshot or thread handle");
    return valid;
}
} // namespace

int wmain(int argc, wchar_t **argv) {
    if (argc == 4 && std::wstring(argv[1]) == L"--child") {
        HANDLE first = reinterpret_cast<HANDLE>(std::wcstoull(argv[2], nullptr, 10));
        HANDLE second = reinterpret_cast<HANDLE>(std::wcstoull(argv[3], nullptr, 10));
        NWindows::CHandle worker;
        worker.Attach(::CreateThread(nullptr, 0, tick, second, 0, nullptr));
        if (!worker.IsCreated())
            return 2;
        return int(tick(first));
    }
    bool valid = true;
    for (int budget = 0;; ++budget) {
        bool injected = false;
        valid &= attempt(budget, injected);
        if (!injected)
            return valid ? 0 : 1;
    }
}

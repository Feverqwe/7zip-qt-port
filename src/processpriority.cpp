#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include "processpriority.h"
#ifdef _WIN32
#include "Windows/ProcessUtils.h"
#else
#include <cerrno>
#include <climits>
#include <unistd.h>
#ifdef __APPLE__
#include <libproc.h>
#include <sys/proc_info.h>
#include <sys/resource.h>
#else
#include "Common/MyVector.h"
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sched.h>
#endif
#endif

namespace NProcessPriority {
#ifdef _WIN32
namespace {
HRESULT Open(UInt32 processId, DWORD access, NWindows::CProcess &process, HANDLE &handle) {
    handle = ::GetCurrentProcess();
    if (processId != 0) {
        if (!process.Open(access, false, processId))
            return HRESULT_FROM_WIN32(::GetLastError());
        handle = process;
    }
    return S_OK;
}
} // namespace
HRESULT GetBackground(UInt32 processId, bool &background) {
    NWindows::CProcess process;
    HANDLE handle;
    const HRESULT result = Open(processId, PROCESS_QUERY_INFORMATION, process, handle);
    if (result != S_OK)
        return result;
    const DWORD priority = ::GetPriorityClass(handle);
    if (priority == 0)
        return HRESULT_FROM_WIN32(::GetLastError());
    background = priority == IDLE_PRIORITY_CLASS;
    return S_OK;
}
HRESULT SetBackground(UInt32 processId, bool background) {
    NWindows::CProcess process;
    HANDLE handle;
    const HRESULT result = Open(processId, PROCESS_SET_INFORMATION, process, handle);
    if (result != S_OK)
        return result;
    if (!::SetPriorityClass(handle, background ? IDLE_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS))
        return HRESULT_FROM_WIN32(::GetLastError());
    return S_OK;
}
#elif defined(__APPLE__)
HRESULT GetBackground(UInt32 processId, bool &background) {
    if (processId > UInt32(INT_MAX))
        return E_INVALIDARG;
    // getpriority(PRIO_DARWIN_PROCESS, child) reads the caller's policy in
    // Darwin. Query the target's BSD process record, including both enforcement
    // sources. Apple proc_info.h defines these flags in its PRIVATE section.
    constexpr UInt32 kDarwinBackground = 0x8000;
    constexpr UInt32 kExternalDarwinBackground = 0x10000;
    proc_bsdinfo info{};
    errno = 0;
    const int result = proc_pidinfo(processId == 0 ? getpid() : int(processId), PROC_PIDTBSDINFO, 0,
                                    &info, sizeof(info));
    if (result != sizeof(info))
        return HRESULT_FROM_WIN32(errno != 0 ? errno : EIO);
    background = (info.pbi_flags & (kDarwinBackground | kExternalDarwinBackground)) != 0;
    return S_OK;
}
HRESULT SetBackground(UInt32 processId, bool background) {
    if (processId > UInt32(INT_MAX))
        return E_INVALIDARG;
    if (setpriority(PRIO_DARWIN_PROCESS, id_t(processId), background ? PRIO_DARWIN_BG : 0) != 0)
        return HRESULT_FROM_WIN32(errno);
    bool actual;
    const HRESULT result = GetBackground(processId, actual);
    if (result != S_OK)
        return result;
    if (actual != background)
        return HRESULT_FROM_WIN32(EACCES);
    return S_OK;
}
#elif defined(__linux__)
namespace {
struct CThreadPolicy {
    pid_t Id;
    int Policy;
};
using CThreadPolicies = CRecordVector<CThreadPolicy>;

int BasePolicy(int policy) { return policy & ~SCHED_RESET_ON_FORK; }

HRESULT Snapshot(UInt32 processId, CThreadPolicies &threads) {
    if (processId > UInt32(INT_MAX))
        return E_INVALIDARG;
    const pid_t pid = processId == 0 ? getpid() : pid_t(processId);
    char path[64];
    std::snprintf(path, sizeof(path), "/proc/%ld/task", long(pid));
    DIR *directory = opendir(path);
    if (!directory)
        return HRESULT_FROM_WIN32(errno);
    struct CCloseDirectory {
        DIR *Directory;
        ~CCloseDirectory() { closedir(Directory); }
    } closer{directory};
    threads.Clear();
    for (;;) {
        errno = 0;
        const dirent *entry = readdir(directory);
        if (!entry) {
            if (errno != 0)
                return HRESULT_FROM_WIN32(errno);
            break;
        }
        char *end;
        const long id = std::strtol(entry->d_name, &end, 10);
        if (end == entry->d_name || *end != 0 || id <= 0 || id > INT_MAX)
            continue;
        const int policy = sched_getscheduler(pid_t(id));
        if (policy < 0) {
            if (errno == ESRCH)
                continue; // A thread may exit while the directory is enumerated.
            return HRESULT_FROM_WIN32(errno);
        }
        if (BasePolicy(policy) != SCHED_OTHER && BasePolicy(policy) != SCHED_BATCH)
            return HRESULT_FROM_WIN32(ENOTSUP); // Preserve unrelated real-time/idle policies.
        threads.Add({pid_t(id), policy});
    }
    return threads.IsEmpty() ? HRESULT_FROM_WIN32(ESRCH) : S_OK;
}

HRESULT UniformPolicy(const CThreadPolicies &threads, int &policy) {
    policy = BasePolicy(threads[0].Policy);
    FOR_VECTOR(i, threads)
    if (BasePolicy(threads[i].Policy) != policy)
        return HRESULT_FROM_WIN32(ENOTSUP);
    return S_OK;
}

class CRestorePolicy {
    const CThreadPolicies &_threads;
    int _policy;

  public:
    bool Committed = false;
    CRestorePolicy(const CThreadPolicies &threads, int policy)
        : _threads(threads), _policy(policy) {}
    ~CRestorePolicy() {
        if (!Committed) {
            const sched_param parameter{};
            FOR_VECTOR(i, _threads)
            sched_setscheduler(_threads[i].Id, _policy | (_threads[i].Policy & SCHED_RESET_ON_FORK),
                               &parameter);
        }
    }
};
} // namespace

HRESULT GetBackground(UInt32 processId, bool &background) {
    try {
        CThreadPolicies threads;
        HRESULT result = Snapshot(processId, threads);
        if (result != S_OK)
            return result;
        int policy;
        result = UniformPolicy(threads, policy);
        if (result != S_OK)
            return result;
        background = policy == SCHED_BATCH;
        return S_OK;
    } catch (...) {
        return E_OUTOFMEMORY;
    }
}
HRESULT SetBackground(UInt32 processId, bool background) {
    try {
        CThreadPolicies threads;
        HRESULT result = Snapshot(processId, threads);
        if (result != S_OK)
            return result;
        int original;
        result = UniformPolicy(threads, original);
        if (result != S_OK)
            return result;
        CRestorePolicy restore(threads, original);
        const int target = background ? SCHED_BATCH : SCHED_OTHER;
        const sched_param parameter{};
        // Recheck for threads created while the class changes. Newly created
        // pthreads inherit their creator's scheduler; retain them for rollback.
        for (unsigned pass = 0; pass < 3; ++pass) {
            FOR_VECTOR(i, threads)
            if (sched_setscheduler(threads[i].Id,
                                   target | (threads[i].Policy & SCHED_RESET_ON_FORK),
                                   &parameter) != 0 &&
                errno != ESRCH)
                return HRESULT_FROM_WIN32(errno);
            CThreadPolicies current;
            result = Snapshot(processId, current);
            if (result != S_OK)
                return result;
            bool finished = true;
            FOR_VECTOR(i, current) {
                bool found = false;
                FOR_VECTOR(j, threads)
                if (threads[j].Id == current[i].Id) {
                    found = true;
                    break;
                }
                if (!found)
                    threads.Add(current[i]);
                if (BasePolicy(current[i].Policy) != target)
                    finished = false;
            }
            if (finished) {
                restore.Committed = true;
                return S_OK;
            }
        }
        return HRESULT_FROM_WIN32(EAGAIN);
    } catch (...) {
        return E_OUTOFMEMORY;
    }
}
#else
#error A native background priority service is required for this platform.
#endif
} // namespace NProcessPriority

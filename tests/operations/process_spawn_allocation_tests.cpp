#include "platform/process/qtexternalprocess.h"
#include <QFileInfo>
#include <QScopeGuard>
#include <QThread>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
thread_local bool injectAfterSpawn = false;
thread_local bool failNextAllocation = false;
thread_local pid_t spawned = 0;
} // namespace

void *operator new(std::size_t size) {
    if (failNextAllocation) {
        failNextAllocation = false;
        throw std::bad_alloc();
    }
    if (void *memory = std::malloc(size ? size : 1))
        return memory;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *memory) noexcept { std::free(memory); }
void operator delete[](void *memory) noexcept { std::free(memory); }
void operator delete(void *memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void *memory, std::size_t) noexcept { std::free(memory); }

// The fixture passes its absolute executable path, so posix_spawn performs the
// same native launch without PATH lookup. Arm failure only after the real child
// exists; no production hook or child-program allocation is changed.
extern "C" int posix_spawnp(pid_t *pid, const char *file, const posix_spawn_file_actions_t *actions,
                            const posix_spawnattr_t *attributes, char *const arguments[],
                            char *const environment[])
#ifdef __linux__
    noexcept
#endif
{
    const int result = ::posix_spawn(pid, file, actions, attributes, arguments, environment);
    if (result == 0) {
        spawned = *pid;
        if (injectAfterSpawn) {
            injectAfterSpawn = false;
            failNextAllocation = true;
        }
    }
    return result;
}

namespace {
bool reaped(pid_t id) {
    // Observe reaping without stealing it from the owner's asynchronous reaper.
    for (unsigned attempt = 0; attempt < 500; ++attempt) {
        siginfo_t information{};
        if (::waitid(P_PID, id_t(id), &information, WEXITED | WNOHANG | WNOWAIT) < 0 &&
            errno == ECHILD)
            return true;
        QThread::msleep(10);
    }
    while (::waitpid(id, nullptr, 0) < 0 && errno == EINTR) {
    }
    std::fputs("process owner left its child unreaped\n", stderr);
    return false;
}

bool metadataFailure(const UString &program) {
    const UString parameters(L"--child");
    const UStringVector paths;
    QtNativeProcessServices services;
    IQtProcessHandle *process = nullptr;
    bool failed = false;
    HRESULT result = E_FAIL;
    injectAfterSpawn = true;
    try {
        result = services.Create(program, parameters, paths, process);
    } catch (const std::bad_alloc &) {
        failed = true;
    }
    injectAfterSpawn = failNextAllocation = false;
    std::unique_ptr<IQtProcessHandle> owner(process);
    if (spawned <= 0) {
        std::fputs("post-launch allocation failure was not injected\n", stderr);
        return false;
    }
    const bool retained =
        !failed && result == E_OUTOFMEMORY && owner && owner->Id() == UInt32(spawned);
    owner.reset();
    if (!retained)
        std::fputs("failed process adoption did not return its launched-process owner\n", stderr);
    return reaped(spawned) && retained;
}

bool closeWithUnavailableAllocation(const UString &program) {
    int control[2];
    if (::pipe(control) != 0)
        return false;
    const auto closePipe = qScopeGuard([&] {
        if (control[1] >= 0) {
            ::write(control[1], "x", 1);
            ::close(control[1]);
        }
        if (control[0] >= 0)
            ::close(control[0]);
    });
    const UString parameters(QString("--child-wait %1").arg(control[0]).toStdWString().c_str());
    QtNativeProcessServices services;
    IQtProcessHandle *process = nullptr;
    const UStringVector paths;
    const HRESULT result = services.Create(program, parameters, paths, process);
    std::unique_ptr<IQtProcessHandle> owner(process);
    if (result != S_OK || !owner || owner->Id() != UInt32(spawned) ||
        owner->Status() != QtProcessStatus::Running)
        return false;
    const pid_t id = spawned;
    // The child is blocked on a pipe. Releasing its owner must remain safe
    // without allocating a reaper and must return before the child exits.
    failNextAllocation = true;
    owner.reset();
    failNextAllocation = false;
    const bool released = ::write(control[1], "x", 1) == 1;
    return reaped(id) && released;
}
} // namespace

int main(int argc, char **argv) {
    if (argc == 2 && QByteArray(argv[1]) == "--child")
        return 0;
    if (argc == 3 && QByteArray(argv[1]) == "--child-wait") {
        ::alarm(30); // Bound fixture lifetime if a failing parent is terminated.
        char value;
        while (::read(std::atoi(argv[2]), &value, 1) < 0 && errno == EINTR) {
        }
        return 0;
    }
    const UString program(
        QFileInfo(QString::fromLocal8Bit(argv[0])).absoluteFilePath().toStdWString().c_str());
    const bool metadata = metadataFailure(program);
    const bool close = closeWithUnavailableAllocation(program);
    return metadata && close ? 0 : 1;
}

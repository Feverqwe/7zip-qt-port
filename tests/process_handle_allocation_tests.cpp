#include "qtexternalprocess.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>

namespace {
thread_local bool failNextAllocation = false;
} // namespace

// Only the public service call is armed. This standalone executable has no
// event loop or allocating parent threads, and uses real native process handles.
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

namespace {
template <class Operation> bool allocationFailure(const char *name, Operation operation) {
    DWORD before = 0, after = 0;
    if (!::GetProcessHandleCount(::GetCurrentProcess(), &before))
        return false;
    bool failed = false;
    failNextAllocation = true;
    try {
        operation();
    } catch (const std::bad_alloc &) {
        failed = true;
    }
    failNextAllocation = false;
    if (!::GetProcessHandleCount(::GetCurrentProcess(), &after))
        return false;
    if (!failed || after != before) {
        std::fprintf(stderr, "%s: allocation exception=%d, native handles before=%lu after=%lu\n",
                     name, int(failed), static_cast<unsigned long>(before),
                     static_cast<unsigned long>(after));
        return false;
    }
    return true;
}
} // namespace

int main() {
    QtNativeProcessServices services;
    const UInt32 id = services.CurrentProcessId();
    // Warm Qt/native lookup state before measuring ownership through failure.
    {
        CObjectVector<CSnapshotProcess> records;
        services.GetSnapshot(records);
        std::unique_ptr<IQtProcessHandle> process(services.OpenProcess(id));
        if (records.IsEmpty() || !process || process->Id() != id)
            return 1;
    }
    const bool snapshot = allocationFailure("snapshot", [&] {
        CObjectVector<CSnapshotProcess> records;
        services.GetSnapshot(records);
    });
    const bool process = allocationFailure("open process", [&] {
        std::unique_ptr<IQtProcessHandle> handle(services.OpenProcess(id));
    });
    return snapshot && process ? 0 : 1;
}

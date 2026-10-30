#include "support/delayed_process_start.h"
#include <cstring>

DelayedProcessStart *DelayedProcessStart::active_ = nullptr;

DelayedProcessStart::DelayedProcessStart(unsigned delayMs) : delayMs_(delayMs) {
    if (active_)
        return;
    HMODULE qt = GetModuleHandleW(L"Qt6Cored.dll");
    if (!qt)
        qt = GetModuleHandleW(L"Qt6Core.dll");
    if (!qt)
        return;
    auto *base = reinterpret_cast<unsigned char *>(qt);
    const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return;
    const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return;
    const auto &imports = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!imports.VirtualAddress)
        return;
    const auto *descriptor =
        reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR *>(base + imports.VirtualAddress);
    for (; descriptor->Name; ++descriptor) {
        if (!descriptor->OriginalFirstThunk)
            continue;
        const auto *names =
            reinterpret_cast<const IMAGE_THUNK_DATA *>(base + descriptor->OriginalFirstThunk);
        auto *addresses = reinterpret_cast<IMAGE_THUNK_DATA *>(base + descriptor->FirstThunk);
        for (; names->u1.AddressOfData; ++names, ++addresses) {
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))
                continue;
            const auto *name =
                reinterpret_cast<const IMAGE_IMPORT_BY_NAME *>(base + names->u1.AddressOfData);
            if (std::strcmp(reinterpret_cast<const char *>(name->Name), "CreateProcessW") != 0)
                continue;
            auto **slot = reinterpret_cast<void **>(&addresses->u1.Function);
            DWORD protection;
            if (!VirtualProtect(slot, sizeof(void *), PAGE_READWRITE, &protection))
                return;
            original_ = *slot;
            protection_ = protection;
            active_ = this;
            InterlockedExchangePointer(slot, reinterpret_cast<void *>(&Create));
            slot_ = slot;
            DWORD ignored;
            VirtualProtect(slot_, sizeof(void *), protection_, &ignored);
            return;
        }
    }
}

DelayedProcessStart::~DelayedProcessStart() {
    if (!slot_)
        return;
    DWORD ignored;
    if (VirtualProtect(slot_, sizeof(void *), PAGE_READWRITE, &ignored)) {
        InterlockedExchangePointer(slot_, original_);
        VirtualProtect(slot_, sizeof(void *), protection_, &ignored);
    }
    active_ = nullptr;
}

BOOL WINAPI DelayedProcessStart::Create(LPCWSTR application, LPWSTR arguments,
                                        LPSECURITY_ATTRIBUTES process, LPSECURITY_ATTRIBUTES thread,
                                        BOOL inherit, DWORD flags, LPVOID environment,
                                        LPCWSTR directory, LPSTARTUPINFOW startup,
                                        LPPROCESS_INFORMATION result) {
    auto &fixture = *active_;
    fixture.caller_.store(GetCurrentThreadId());
    Sleep(fixture.delayMs_);
    using CreateFunction = decltype(&CreateProcessW);
    const BOOL created = reinterpret_cast<CreateFunction>(fixture.original_)(
        application, arguments, process, thread, inherit, flags, environment, directory, startup,
        result);
    if (created)
        fixture.child_.store(result->dwProcessId);
    return created;
}

bool DelayedProcessStart::childRunning() const {
    const DWORD id = child_.load();
    if (!id)
        return false;
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, id);
    if (!process)
        return false;
    const bool running = WaitForSingleObject(process, 0) == WAIT_TIMEOUT;
    CloseHandle(process);
    return running;
}

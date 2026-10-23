#pragma once
#include <QtGlobal>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <csignal>
#endif

inline bool KillOperationProbe(quint64 pid) {
    if (!pid)
        return false;
#ifdef Q_OS_WIN
    HANDLE child = OpenProcess(PROCESS_TERMINATE, FALSE, DWORD(pid));
    if (!child)
        return false;
    const bool result = TerminateProcess(child, 0xc0000005u);
    CloseHandle(child);
    return result;
#else
    return ::kill(pid_t(pid), SIGKILL) == 0;
#endif
}

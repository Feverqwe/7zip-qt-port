#pragma once

#include <QScopeGuard>
#include <QString>
#include <atomic>
#include <chrono>
#include <cwchar>
#include <functional>
#include <future>
#include <thread>
#include <windows.h>

namespace NativePropertiesFixture {
struct ObservedPropertySheet {
    QString name;
    HWND owner = nullptr;
    HWND window = nullptr;
    bool tabs = false;
    bool visible = false;
    QString diagnostic;
};

inline BOOL CALLBACK FindPropertySheet(HWND window, LPARAM parameter) {
    auto &sheet = *reinterpret_cast<ObservedPropertySheet *>(parameter);
    wchar_t title[1024] = {};
    GetWindowTextW(window, title, 1024);
    if (!QString::fromWCharArray(title).contains(sheet.name))
        return TRUE;
    DWORD process = 0;
    GetWindowThreadProcessId(window, &process);
    wchar_t className[64] = {};
    GetClassNameW(window, className, 64);
    sheet.diagnostic = QString("class=%1 pid=%2 visible=%3 owner=%4 expectedOwner=%5 title=%6")
                           .arg(QString::fromWCharArray(className))
                           .arg(process)
                           .arg(IsWindowVisible(window))
                           .arg(quintptr(GetWindow(window, GW_OWNER)))
                           .arg(quintptr(sheet.owner))
                           .arg(QString::fromWCharArray(title));
    // The shell can host a sheet in another process with a proxy owner. The
    // unique disposable filename identifies this fixture across that boundary.
    if (std::wcscmp(className, L"#32770") != 0)
        return TRUE;
    // A native Windows property sheet has tabs. The former Qt collector has
    // neither this native class nor the selected file in its caption.
    HWND tabs = FindWindowExW(window, nullptr, L"SysTabControl32", nullptr);
    if (!tabs)
        return TRUE;
    sheet.window = window;
    sheet.tabs = true;
    sheet.visible = IsWindowVisible(window);
    return FALSE;
}

inline ObservedPropertySheet ObserveAndClose(const QString &name, HWND owner,
                                             const std::atomic<bool> &stop) {
    ObservedPropertySheet sheet{name, owner};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(12);
    while (!stop && std::chrono::steady_clock::now() < deadline) {
        EnumWindows(FindPropertySheet, reinterpret_cast<LPARAM>(&sheet));
        if (sheet.window && sheet.visible) {
            PostMessageW(sheet.window, WM_CLOSE, 0, 0);
            return sheet;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    // A failing visibility regression still owns this uniquely named sheet.
    // Close it so failed fixtures do not leak hidden windows in the guest.
    if (sheet.window)
        PostMessageW(sheet.window, WM_CLOSE, 0, 0);
    return sheet;
}
} // namespace NativePropertiesFixture

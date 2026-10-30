// Test-only entry adapter around the unmodified pinned FM.cpp body.
// The build script places FM.original.cpp beside this file in a private snapshot.
#include "StdAfx.h"
#define WinMain SevenzipReferenceOriginalWinMain
#include "FM.original.cpp"
#undef WinMain
#include "../../../Windows/Registry.h"
#include "ReferenceKey.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command, int show) {
    wchar_t computer[MAX_COMPUTERNAME_LENGTH + 1] = {};
    wchar_t expected[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD length = MAX_COMPUTERNAME_LENGTH + 1;
    const DWORD expectedLength = GetEnvironmentVariableW(L"SEVENZIP_WINDOWS_RUNNER_HOST", expected,
                                                         MAX_COMPUTERNAME_LENGTH + 1);
    if (!expectedLength || expectedLength > MAX_COMPUTERNAME_LENGTH ||
        !GetComputerNameW(computer, &length) || _wcsicmp(computer, expected) != 0)
        return 90;
    const wchar_t *const path = kReferenceRegistryPath;
    HKEY root = nullptr;
    DWORD disposition = 0;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, path, 0, nullptr, REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS, nullptr, &root, &disposition) != ERROR_SUCCESS)
        return 91;
    if (disposition != REG_CREATED_NEW_KEY) {
        RegCloseKey(root);
        return 92;
    }
    HKEY settings = nullptr;
    const DWORD ownerPid = GetCurrentProcessId();
    if (RegSetValueExW(root, L"SevenzipReferenceOwnerPID", 0, REG_DWORD,
                       reinterpret_cast<const BYTE *>(&ownerPid),
                       sizeof(ownerPid)) != ERROR_SUCCESS) {
        RegCloseKey(root);
        RegDeleteKeyW(HKEY_CURRENT_USER, path);
        return 94;
    }
    const DWORD enabled = 1;
    LSTATUS status = RegCreateKeyExW(root, L"FM", 0, nullptr, REG_OPTION_NON_VOLATILE,
                                     KEY_SET_VALUE, nullptr, &settings, nullptr);
    if (status == ERROR_SUCCESS) {
        status = RegSetValueExW(settings, L"ShowSystemMenu", 0, REG_DWORD,
                                reinterpret_cast<const BYTE *>(&enabled), sizeof(enabled));
        RegCloseKey(settings);
    }
    int result = 93;
    if (status == ERROR_SUCCESS) {
        result = SevenzipReferenceOriginalWinMain(instance, previous, command, show);
    }
    RegCloseKey(root);
    NWindows::NRegistry::CKey parent;
    parent.Attach(HKEY_CURRENT_USER);
    parent.RecurseDeleteKey(GetSystemString(path));
    parent.Detach();
    return result;
}

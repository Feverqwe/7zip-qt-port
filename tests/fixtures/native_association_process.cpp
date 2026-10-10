// Private, bounded ShellExecute fixture; never registers a file association.
#include <string>
#include <windows.h>

namespace {
bool Write(const std::wstring &path, const std::string &bytes) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    const bool result = WriteFile(file, bytes.data(), DWORD(bytes.size()), &written, nullptr) &&
                        written == bytes.size();
    CloseHandle(file);
    return result;
}
} // namespace

int main() {
    wchar_t controlBuffer[32768], module[32768];
    const DWORD length = GetEnvironmentVariableW(L"SEVENZIP_ASSOC_CONTROL", controlBuffer, 32768);
    if (!length || length >= 32768)
        return 2;
    const DWORD moduleLength = GetModuleFileNameW(nullptr, module, 32768);
    if (!moduleLength || moduleLength >= 32768)
        return 3;
    const int size =
        WideCharToMultiByte(CP_UTF8, 0, module, int(moduleLength), nullptr, 0, nullptr, nullptr);
    if (!size)
        return 4;
    std::string path(size, '\0');
    if (!WideCharToMultiByte(CP_UTF8, 0, module, int(moduleLength), path.data(), size, nullptr,
                             nullptr))
        return 4;
    const std::wstring control(controlBuffer, length);
    if (!Write(control + L".pid", std::to_string(GetCurrentProcessId())) ||
        !Write(control + L".ready", path))
        return 5;
    const DWORD start = GetTickCount();
    while (GetFileAttributesW((control + L".exit").c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (GetTickCount() - start >= 20000)
            return 6;
        Sleep(25);
    }
    return Write(control + L".finished", "done") ? 0 : 7;
}

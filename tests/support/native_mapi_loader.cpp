#include "support/native_mapi_loader.h"
#include <QDir>
#include <QFileInfo>

HMODULE LoadPrivateMAPIProvider(const QString &requested) {
    const QString provider = QFileInfo(requested).canonicalFilePath();
    if (provider.isEmpty() ||
        QFileInfo(provider).fileName().compare("Mapi32.dll", Qt::CaseInsensitive))
        return nullptr;
    HMODULE module = LoadLibraryExW(
        reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(provider).utf16()), nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!module)
        return nullptr;
    const auto identity =
        reinterpret_cast<DWORD(WINAPI *)()>(GetProcAddress(module, "SevenzipMAPIProvider"));
    wchar_t path[32768]{};
    if (GetModuleHandleW(L"Mapi32.dll") != module || !identity || identity() != 0x375a4d50 ||
        !GetModuleFileNameW(module, path, 32768) ||
        QFileInfo(QString::fromWCharArray(path))
            .canonicalFilePath()
            .compare(provider, Qt::CaseInsensitive)) {
        FreeLibrary(module);
        return nullptr;
    }
    return module;
}

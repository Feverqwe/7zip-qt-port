// Public DLL consumer of the authoritative Windows module-path implementation.
#include "platform/loader/DLL.h"
#include <algorithm>
#include <cwchar>

extern "C" __declspec(dllexport) unsigned sevenzip_module_path(wchar_t *buffer, unsigned capacity) {
    FString path;
    if (!NWindows::NDLL::MyGetModuleFileName(path))
        return 0;
    const unsigned required = path.Len() + 1;
    if (buffer && capacity >= required)
        std::copy_n(path.Ptr(), required, buffer);
    return required;
}

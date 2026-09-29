// Adapted from 7-Zip CPP/Windows/DLL.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Library lifetime/loading and POSIX procedure lookup retain source bodies.
// Module-path helpers remain mandatory separate platform/resource scope.
#include "Windows/StdAfx.h"

#include "DLL.h"

#ifdef _WIN32

#ifndef _UNICODE
extern bool g_IsNT;
#endif

namespace NWindows {
namespace NDLL {

bool CLibrary::Free() throw() {
    if (_module == NULL)
        return true;
    if (!::FreeLibrary(_module))
        return false;
    _module = NULL;
    return true;
}

bool CLibrary::LoadEx(CFSTR path, DWORD flags) throw() {
    if (!Free())
        return false;
#ifndef _UNICODE
    if (!g_IsNT) {
        _module = ::LoadLibraryEx(fs2fas(path), NULL, flags);
    } else
#endif
    {
        _module = ::LoadLibraryExW(fs2us(path), NULL, flags);
    }
    return (_module != NULL);
}

bool CLibrary::Load(CFSTR path) throw() {
    if (!Free())
        return false;
#ifndef _UNICODE
    if (!g_IsNT) {
        _module = ::LoadLibrary(fs2fas(path));
    } else
#endif
    {
        _module = ::LoadLibraryW(fs2us(path));
    }
    return (_module != NULL);
}

} // namespace NDLL
} // namespace NWindows

#else

#include <dlfcn.h>
#include <stdlib.h>

void *GetProcAddress(HMODULE module, LPCSTR procName) {
    void *ptr = NULL;
    if (module)
        ptr = dlsym(module, procName);
    return ptr;
}

namespace NWindows {
namespace NDLL {

bool CLibrary::Free() throw() {
    if (!_module)
        return true;
    const int ret = dlclose(_module);
    if (ret != 0)
        return false;
    _module = NULL;
    return true;
}

bool CLibrary::Load(CFSTR path) throw() {
    if (!Free())
        return false;

    int options = 0;

#ifdef RTLD_LOCAL
    options |= RTLD_LOCAL;
#endif

#ifdef RTLD_NOW
    options |= RTLD_NOW;
#endif

#ifdef RTLD_GROUP
#if !(defined(hpux) || defined(__hpux))
    options |= RTLD_GROUP; // mainly for solaris but not for HPUX
#endif
#endif

    _module = dlopen(path, options);
    return (_module != NULL);
}

} // namespace NDLL
} // namespace NWindows

#endif

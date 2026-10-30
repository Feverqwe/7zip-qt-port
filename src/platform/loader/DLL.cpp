// Adapted from 7-Zip CPP/Windows/DLL.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Library lifetime/loading and POSIX procedure lookup retain source bodies.
// Windows module-path and directory/fallback rules retain source code.
// Qt supplies the POSIX executable-path service.
#include "Windows/StdAfx.h"

#include "platform/loader/DLL.h"
#include <QCoreApplication>
#include <QFile>

#if defined(_WIN32) && !defined(_UNICODE)
extern bool g_IsNT;
#endif

namespace NWindows {
namespace NDLL {

bool MyGetModuleFileName(FString &path) {
#ifdef _WIN32
    // Resolve the module containing this implementation instead of requiring
    // every EXE/DLL consumer to define upstream's mutable g_hInstance global.
    HMODULE hModule = NULL;
    path.Empty();
    if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                  GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              reinterpret_cast<LPCWSTR>(&MyGetModuleFileName), &hModule))
        return false;
#ifndef _UNICODE
    if (!g_IsNT) {
        TCHAR s[MAX_PATH + 2];
        s[0] = 0;
        const DWORD size = ::GetModuleFileName(hModule, s, MAX_PATH + 1);
        if (size <= MAX_PATH && size != 0) {
            path = fas2fs(s);
            return true;
        }
    } else
#endif
    {
        WCHAR s[MAX_PATH + 2];
        s[0] = 0;
        const DWORD size = ::GetModuleFileNameW(hModule, s, MAX_PATH + 1);
        if (size <= MAX_PATH && size != 0) {
            path = us2fs(s);
            return true;
        }
    }
    return false;
#else
    const QString executable = QCoreApplication::applicationFilePath();
    path.Empty();
    if (executable.isEmpty())
        return false;
    path = QFile::encodeName(executable).constData();
    return true;
#endif
}

// Original upstream directory/fallback body, shared with the POSIX adapter.
FString GetModuleDirPrefix() {
    FString s;
    if (MyGetModuleFileName(s)) {
        const int pos = s.ReverseFind_PathSepar();
        if (pos >= 0)
            s.DeleteFrom((unsigned)(pos + 1));
    }
    if (s.IsEmpty())
        s = "." STRING_PATH_SEPARATOR;
    return s;
}

} // namespace NDLL
} // namespace NWindows

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

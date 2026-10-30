// Adapted from 7-Zip CPP/Windows/DLL.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// POSIX parent links need the original Windows transfer/copy-guard methods.
#ifndef ZIP7_INC_WINDOWS_DLL_H
#define ZIP7_INC_WINDOWS_DLL_H

#include "Common/MyString.h"

#ifndef _WIN32
typedef void *HMODULE;
void *GetProcAddress(HMODULE module, LPCSTR procName);
#endif

namespace NWindows {
namespace NDLL {

#ifdef _WIN32
/* Win32: Don't call CLibrary::Free() and FreeLibrary() from another
    FreeLibrary() code: detaching code in DLL entry-point or in
    destructors of global objects in DLL module. */
class CLibrary {
    HMODULE _module;

  public:
    CLibrary() : _module(NULL) {}
    ~CLibrary() { Free(); }

    CLibrary(const CLibrary &c) : _module(NULL) {
        if (c._module) {
            // we need non const to reference from original item
            // c._module = NULL;
            throw 20230102;
        }
    }

    HMODULE Get_HMODULE() const { return _module; }
    bool IsLoaded() const { return (_module != NULL); }

    void Attach(HMODULE m) {
        Free();
        _module = m;
    }
    HMODULE Detach() {
        const HMODULE m = _module;
        _module = NULL;
        return m;
    }

    bool Free() throw();
    bool LoadEx(CFSTR path, DWORD flags = LOAD_LIBRARY_AS_DATAFILE) throw();
    bool Load(CFSTR path) throw();
};

#else

class CLibrary {
    HMODULE _module;

  public:
    CLibrary() : _module(NULL) {}
    ~CLibrary() { Free(); }

    // The source File Manager's parent links transfer libraries after copying
    // an empty link. Extend POSIX with the original Windows guard/transfer bodies.
    CLibrary(const CLibrary &c) : _module(NULL) {
        if (c._module) {
            // we need non const to reference from original item
            // c._module = NULL;
            throw 20230102;
        }
    }

    HMODULE Get_HMODULE() const { return _module; }
    bool IsLoaded() const { return (_module != NULL); }

    void Attach(HMODULE m) {
        Free();
        _module = m;
    }
    HMODULE Detach() {
        const HMODULE m = _module;
        _module = NULL;
        return m;
    }

    bool Free() throw();
    bool Load(CFSTR path) throw();
};

#endif

bool MyGetModuleFileName(FString &path);
FString GetModuleDirPrefix();

} // namespace NDLL
} // namespace NWindows

#endif

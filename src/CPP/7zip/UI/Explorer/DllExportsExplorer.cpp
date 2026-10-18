// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// DLLExportsExplorer.cpp
//
// Notes:
// Win2000:
// If I register at HKCR\Folder\ShellEx then DLL is locked.
// otherwise it unloads after explorer closing.
// but if I call menu for desktop items it's locked all the time

#include "Common/MyWindows.h"

#if defined(__clang__) && __clang_major__ >= 4
#pragma GCC diagnostic ignored "-Wnonportable-system-include-path"
#endif
// <olectl.h> : in new Windows Kit 10.0.2**** (NTDDI_WIN10_MN is defined)
// <OleCtl.h> : in another Windows Kit versions
#if defined(NTDDI_WIN10_MN) || defined(__MINGW32__) || defined(__MINGW64__)
#include <olectl.h>
#else
#include <OleCtl.h>
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <shlguid.h>
#else
#include <ShlGuid.h>
#endif

#include "Common/ComTry.h"

#include "Windows/DLL.h"
#include "Windows/ErrorMsg.h"
#include "Windows/NtCheck.h"
#include "Windows/Registry.h"

#include "ContextMenu.h"

// ClassDefs is the shared owner of portable interface GUIDs. Initialize only
// this shell class after the shared interface declarations have been included.
#include "Common/MyInitGuid.h"
// MyWindows intentionally refreshes Z7_DEFINE_GUID outside its include guard.
#include "Common/MyWindows.h"

static LPCTSTR const k_ShellExtName = TEXT("Sevenzip Desktop Shell Extension");
static LPCTSTR const k_Approved =
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved");

// {52D06B47-DA74-4A5F-8B5E-4494C729CC9C}
static LPCTSTR const k_Clsid = TEXT("{52D06B47-DA74-4A5F-8B5E-4494C729CC9C}");

Z7_DEFINE_GUID(CLSID_CZipContextMenu, 0x52D06B47, 0xDA74, 0x4A5F, 0x8B, 0x5E, 0x44, 0x94, 0xC7,
               0x29, 0xCC, 0x9C);

using namespace NWindows;

extern HINSTANCE g_hInstance;
HINSTANCE g_hInstance = NULL;

extern HWND g_HWND;
HWND g_HWND = NULL;

extern LONG g_DllRefCount;
// The shared ContextMenu module owns this DLL image's reference count.

extern bool g_DisableUserQuestions;
// MyMessages owns this image's source question flag.

// #define ODS(sz) OutputDebugStringW(L#sz)
#define ODS(sz)

class CShellExtClassFactory Z7_final : public IClassFactory, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1_MT(IClassFactory)

    STDMETHOD(CreateInstance)(LPUNKNOWN, REFIID, void **) Z7_override Z7_final;
    STDMETHOD(LockServer)(BOOL) Z7_override Z7_final;

  public:
    CShellExtClassFactory() { InterlockedIncrement(&g_DllRefCount); }
    ~CShellExtClassFactory() { InterlockedDecrement(&g_DllRefCount); }
};

Z7_COMWF_B CShellExtClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void **ppvObj) {
    ODS("CShellExtClassFactory::CreateInstance()\r\n");
    /*
    char s[64];
    ConvertUInt32ToHex(riid.Data1, s);
    OutputDebugStringA(s);
    */
    *ppvObj = NULL;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CZipContextMenu *shellExt;
    try {
        shellExt = new CZipContextMenu();
    } catch (...) {
        return E_OUTOFMEMORY;
    }
    if (!shellExt)
        return E_OUTOFMEMORY;

    IContextMenu *ctxm = shellExt;
    const HRESULT res = ctxm->QueryInterface(riid, ppvObj);
    if (res != S_OK)
        delete shellExt;
    return res;
}

Z7_COMWF_B CShellExtClassFactory::LockServer(BOOL fLock) {
    if (fLock)
        InterlockedIncrement(&g_DllRefCount);
    else
        InterlockedDecrement(&g_DllRefCount);
    return S_OK;
}

#if defined(_UNICODE) && !defined(_WIN64) && !defined(UNDER_CE)
#define NT_CHECK_FAIL_ACTION return FALSE;
#endif

extern "C" BOOL WINAPI DllMain(
#ifdef UNDER_CE
    HANDLE hInstance
#else
    HINSTANCE hInstance
#endif
    ,
    DWORD dwReason, LPVOID);

extern "C" BOOL WINAPI DllMain(
#ifdef UNDER_CE
    HANDLE hInstance
#else
    HINSTANCE hInstance
#endif
    ,
    DWORD dwReason, LPVOID) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        g_hInstance = (HINSTANCE)hInstance;
        ODS("In DLLMain, DLL_PROCESS_ATTACH\r\n");
        NT_CHECK
    } else if (dwReason == DLL_PROCESS_DETACH) {
        ODS("In DLLMain, DLL_PROCESS_DETACH\r\n");
    }
    return TRUE;
}

// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void) {
    ODS("In DLLCanUnloadNow\r\n");
    /*
    if (g_DllRefCount == 0)
      ODS( "g_DllRefCount == 0");
    else
      ODS( "g_DllRefCount != 0");
    */
    return (InterlockedCompareExchange(&g_DllRefCount, 0, 0) == 0 ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) {
    ODS("In DllGetClassObject\r\n");
    *ppv = NULL;
    if (IsEqualIID(rclsid, CLSID_CZipContextMenu)) {
        CShellExtClassFactory *cf;
        try {
            cf = new CShellExtClassFactory;
        } catch (...) {
            return E_OUTOFMEMORY;
        }
        if (!cf)
            return E_OUTOFMEMORY;
        IClassFactory *cf2 = cf;
        const HRESULT res = cf2->QueryInterface(riid, ppv);
        if (res != S_OK)
            delete cf;
        return res;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
    // return _Module.GetClassObject(rclsid, riid, ppv);
}

static BOOL RegisterServer() {
    ODS("RegisterServer\r\n");
    FString modulePath;
    if (!NDLL::MyGetModuleFileName(modulePath))
        return FALSE;
    const UString modulePathU = fs2us(modulePath);

    CSysString s("CLSID\\");
    s += k_Clsid;

    {
        NRegistry::CKey key;
        if (key.Create(HKEY_CLASSES_ROOT, s, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE) != NOERROR)
            return FALSE;
        key.SetValue(NULL, k_ShellExtName);
        NRegistry::CKey keyInproc;
        if (keyInproc.Create(key, TEXT("InprocServer32"), NULL, REG_OPTION_NON_VOLATILE,
                             KEY_WRITE) != NOERROR)
            return FALSE;
        keyInproc.SetValue(NULL, modulePathU);
        keyInproc.SetValue(TEXT("ThreadingModel"), TEXT("Apartment"));
    }

#if !defined(_WIN64) && !defined(UNDER_CE)
    if (IsItWindowsNT())
#endif
    {
        NRegistry::CKey key;
        if (key.Create(HKEY_LOCAL_MACHINE, k_Approved, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE) ==
            NOERROR)
            key.SetValue(k_Clsid, k_ShellExtName);
    }

    ODS("RegisterServer :: return TRUE");
    return TRUE;
}

STDAPI DllRegisterServer(void) { return RegisterServer() ? S_OK : SELFREG_E_CLASS; }

static BOOL UnregisterServer() {
    CSysString s("CLSID\\");
    s += k_Clsid;

    RegDeleteKey(HKEY_CLASSES_ROOT, s + TEXT("\\InprocServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, s);

#if !defined(_WIN64) && !defined(UNDER_CE)
    if (IsItWindowsNT())
#endif
    {
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, k_Approved, 0, KEY_SET_VALUE, &hKey) == NOERROR) {
            RegDeleteValue(hKey, k_Clsid);
            RegCloseKey(hKey);
        }
    }

    return TRUE;
}

STDAPI DllUnregisterServer(void) { return UnregisterServer() ? S_OK : SELFREG_E_CLASS; }

// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "RegistryContextMenu.h"
using namespace NQtShellRegistry;
#ifndef _WIN32
static const LONG ERROR_SUCCESS = 0;
#endif
#define Z7_SHELL_TEXT(s) L##s

// does extension can work, if Approved is removed ?
// CLISID (and Approved ?) items are separated for 32-bit and 64-bit code.
// shellex items shared by 32-bit and 64-bit code?

#define k_Clsid_A "{52D06B47-DA74-4A5F-8B5E-4494C729CC9C}"

static const wchar_t *const k_Clsid = kSevenzipShellClsid;
static const wchar_t *const k_ShellExtName = Z7_SHELL_TEXT("Sevenzip Desktop Shell Extension");

static const wchar_t *const k_Approved =
    Z7_SHELL_TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved");
static const wchar_t *const k_Inproc = Z7_SHELL_TEXT("InprocServer32");

static const char *const k_KeyPostfix_ContextMenu =
    "\\shellex\\ContextMenuHandlers\\SevenzipDesktop";
static const char *const k_KeyPostfix_DragDrop = "\\shellex\\DragDropHandlers\\SevenzipDesktop";

static const char *const k_KeyName_File = "*";
static const char *const k_KeyName_Folder = "Folder";
static const char *const k_KeyName_Directory = "Directory";
static const char *const k_KeyName_Drive = "Drive";

static const char *const k_shellex_Prefixes[] = {k_KeyName_File, k_KeyName_Folder,
                                                 k_KeyName_Directory, k_KeyName_Drive};

static const bool k_shellex_Statuses[2][4] = {{true, true, true, false},
                                              {false, false, true, true}};

static LONG MyRegistry_DeleteKey_HKCR(const UString &name, UInt32 wow, IServices &services) {
    return services.Delete(ERoot::Classes, name, wow);
}

// static NSynchronization::CCriticalSection g_CS;

static AString Get_ContextMenuHandler_KeyName(const char *keyName) {
    return (AString)keyName + k_KeyPostfix_ContextMenu;
}

/*
static UString Get_DragDropHandler_KeyName(const wchar_t * keyName)
  { return (AString)keyName + k_KeyPostfix_DragDrop); }
*/

static bool CheckHandlerCommon(const AString &keyName, UInt32 wow, IServices &services) {
    CKey key(services);
    if (key.Open(ERoot::Classes, (UString)keyName, kRead | wow) != ERROR_SUCCESS)
        return false;
    UString value;
    if (key.QueryValue(NULL, value) != ERROR_SUCCESS)
        return false;
    return StringsAreEqualNoCase_Ascii(value, k_Clsid_A);
}

bool CheckContextMenuHandler(const UString &path, UInt32 wow, IServices &services) {
    // NSynchronization::CCriticalSectionLock lock(g_CS);

    UString s("CLSID\\");
    s += k_Clsid_A;
    s += "\\InprocServer32";

    {
        CKey key(services);
        if (key.Open(ERoot::Classes, s, kRead | wow) != ERROR_SUCCESS)
            return false;
        UString regPath;
        if (key.QueryValue(NULL, regPath) != ERROR_SUCCESS)
            return false;
        if (!path.IsEqualTo_NoCase(regPath))
            return false;
    }

    return CheckHandlerCommon(Get_ContextMenuHandler_KeyName(k_KeyName_File), wow, services);
    /*
      && CheckHandlerCommon(Get_ContextMenuHandler_KeyName(k_KeyName_Directory), wow)
      // && CheckHandlerCommon(Get_ContextMenuHandler_KeyName(k_KeyName_Folder))

      && CheckHandlerCommon(Get_DragDropHandler_KeyName(k_KeyName_Directory), wow)
      && CheckHandlerCommon(Get_DragDropHandler_KeyName(k_KeyName_Drive), wow);
    */
}

static LONG MyCreateKey(CKey &key, ERoot parentKey, const UString &keyName, UInt32 wow) {
    return key.Create(parentKey, keyName, 0, 0, kAllAccess | wow);
}
static LONG MyCreateKey(CKey &key, CKey &parentKey, const UString &keyName, UInt32 wow) {
    return key.Create(parentKey, keyName, 0, 0, kAllAccess | wow);
}

LONG SetContextMenuHandler(bool setMode, const UString &path, UInt32 wow, IServices &services) {
    // NSynchronization::CCriticalSectionLock lock(g_CS);

    LONG res;

    {
        UString s("CLSID\\");
        s += k_Clsid_A;

        if (setMode) {
            {
                CKey key(services);
                res = MyCreateKey(key, ERoot::Classes, s, wow);
                if (res == ERROR_SUCCESS) {
                    key.SetValue(NULL, UString(k_ShellExtName));
                    CKey keyInproc(services);
                    res = MyCreateKey(keyInproc, key, k_Inproc, wow);
                    if (res == ERROR_SUCCESS) {
                        res = keyInproc.SetValue(NULL, path);
                        keyInproc.SetValue(Z7_SHELL_TEXT("ThreadingModel"),
                                           UString(Z7_SHELL_TEXT("Apartment")));
                    }
                }
            }

            {
                CKey key(services);
                if (MyCreateKey(key, ERoot::Machine, k_Approved, wow) == ERROR_SUCCESS)
                    key.SetValue(k_Clsid, UString(k_ShellExtName));
            }
        } else {
            UString s2(s);
            s2 += "\\InprocServer32";

            MyRegistry_DeleteKey_HKCR(s2, wow, services);
            res = MyRegistry_DeleteKey_HKCR(s, wow, services);
        }
    }

    // shellex items probably are shared beween 32-bit and 64-bit apps. So we don't delete items for
    // delete operation.
    if (setMode)
        for (unsigned i = 0; i < 2; i++) {
            for (unsigned k = 0; k < Z7_ARRAY_SIZE(k_shellex_Prefixes); k++) {
                UString s(k_shellex_Prefixes[k]);
                s += (i == 0 ? k_KeyPostfix_ContextMenu : k_KeyPostfix_DragDrop);
                if (k_shellex_Statuses[i][k]) {
                    CKey key(services);
                    MyCreateKey(key, ERoot::Classes, s, wow);
                    key.SetValue(NULL, UString(k_Clsid));
                } else
                    MyRegistry_DeleteKey_HKCR(s, wow, services);
            }
        }

    return res;
}

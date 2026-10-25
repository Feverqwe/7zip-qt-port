// Disposable Explorer-style host. Only this host and its helper belong to the
// kill-on-close job; a failed test cannot leave a modal 7zG process behind.
#include <cstdio>
#include <shlobj.h>
#include <windows.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class PrivateClassRegistration {
    static constexpr const wchar_t *path =
        L"Software\\Classes\\CLSID\\{52D06B47-DA74-4A5F-8B5E-4494C729CC9C}";
    bool owned = false;

  public:
    bool create(const wchar_t *library) {
        // Registration fixtures run only on the authorized disposable guest.
        wchar_t machine[MAX_COMPUTERNAME_LENGTH + 1]{};
        wchar_t expected[MAX_COMPUTERNAME_LENGTH + 1]{};
        DWORD length = MAX_COMPUTERNAME_LENGTH + 1;
        const DWORD expectedLength = GetEnvironmentVariableW(L"SEVENZIP_WINDOWS_RUNNER_HOST",
                                                             expected, MAX_COMPUTERNAME_LENGTH + 1);
        if (!expectedLength || expectedLength > MAX_COMPUTERNAME_LENGTH ||
            !GetComputerNameW(machine, &length) || _wcsicmp(machine, expected) != 0)
            return false;
        HKEY key = nullptr;
        DWORD disposition = 0;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, path, 0, nullptr, REG_OPTION_NON_VOLATILE,
                            KEY_CREATE_SUB_KEY | KEY_SET_VALUE, nullptr, &key,
                            &disposition) != ERROR_SUCCESS)
            return false;
        // Never overwrite or delete an existing project registration.
        owned = disposition == REG_CREATED_NEW_KEY;
        if (!owned) {
            RegCloseKey(key);
            return false;
        }
        const DWORD owner = GetCurrentProcessId();
        if (RegSetValueExW(key, L"SevenzipTestHostPID", 0, REG_DWORD,
                           reinterpret_cast<const BYTE *>(&owner),
                           sizeof(owner)) != ERROR_SUCCESS) {
            RegCloseKey(key);
            return false;
        }
        HKEY server = nullptr;
        const LONG opened =
            RegCreateKeyExW(key, L"InprocServer32", 0, nullptr, REG_OPTION_NON_VOLATILE,
                            KEY_SET_VALUE, nullptr, &server, nullptr);
        RegCloseKey(key);
        if (opened != ERROR_SUCCESS)
            return false;
        const LONG image =
            RegSetValueExW(server, nullptr, 0, REG_SZ, reinterpret_cast<const BYTE *>(library),
                           DWORD((wcslen(library) + 1) * sizeof(wchar_t)));
        constexpr wchar_t threading[] = L"Apartment";
        const LONG model =
            RegSetValueExW(server, L"ThreadingModel", 0, REG_SZ,
                           reinterpret_cast<const BYTE *>(threading), sizeof(threading));
        RegCloseKey(server);
        return image == ERROR_SUCCESS && model == ERROR_SUCCESS;
    }
    bool remove() {
        if (!owned)
            return true;
        const LONG result = RegDeleteTreeW(HKEY_CURRENT_USER, path);
        if (result == ERROR_SUCCESS)
            owned = false;
        return result == ERROR_SUCCESS;
    }
    ~PrivateClassRegistration() { remove(); }
};

int wmain(int argc, wchar_t **argv) {
    if ((argc != 4 && argc != 5) || (argc == 5 && wcscmp(argv[4], L"--registered") != 0) ||
        FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
        return 2;
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!job ||
        !SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) ||
        !AssignProcessToJobObject(job, GetCurrentProcess()))
        return 3;
    const HMODULE library = LoadLibraryExW(argv[1], nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!library)
        return 4;
    using GetClass = HRESULT(STDAPICALLTYPE *)(REFCLSID, REFIID, void **);
    using CanUnload = HRESULT(STDAPICALLTYPE *)();
    const auto getClass = reinterpret_cast<GetClass>(GetProcAddress(library, "DllGetClassObject"));
    const auto canUnload = reinterpret_cast<CanUnload>(GetProcAddress(library, "DllCanUnloadNow"));
    if (!getClass || !canUnload)
        return 5;
    const GUID classID = {
        0x52D06B47, 0xDA74, 0x4A5F, {0x8B, 0x5E, 0x44, 0x94, 0xC7, 0x29, 0xCC, 0x9C}};
    PrivateClassRegistration registration;
    if (argc == 5 && !registration.create(argv[1]))
        return 12;
    {
        ComPtr<IClassFactory> factory;
        ComPtr<IContextMenu> menu;
        ComPtr<IShellExtInit> selection;
        ComPtr<IShellItem> item;
        ComPtr<IDataObject> data;
        HRESULT created = E_FAIL;
        if (argc == 5)
            created = CoCreateInstance(classID, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&menu));
        else if (getClass(classID, IID_IClassFactory,
                          reinterpret_cast<void **>(factory.GetAddressOf())) == S_OK)
            created = factory->CreateInstance(nullptr, IID_PPV_ARGS(&menu));
        // Activation has consumed the private mapping. Remove it before any
        // helper/dialog wait, so even forced test termination leaves no key.
        if (!registration.remove())
            return 13;
        if (FAILED(created) || FAILED(menu.As(&selection)) ||
            FAILED(SHCreateItemFromParsingName(argv[2], nullptr, IID_PPV_ARGS(&item))) ||
            FAILED(item->BindToHandler(nullptr, BHID_DataObject, IID_PPV_ARGS(&data))) ||
            FAILED(selection->Initialize(nullptr, data.Get(), nullptr)))
            return 6;
        HMENU popup = CreatePopupMenu();
        if (!popup)
            return 7;
        const HRESULT queried = menu->QueryContextMenu(popup, 0, 100, 999, CMF_NORMAL);
        DestroyMenu(popup);
        if (FAILED(queried) || HRESULT_CODE(queried) == 0)
            return 8;
        CMINVOKECOMMANDINFOEX invoke{};
        invoke.cbSize = sizeof(invoke);
        invoke.fMask = CMIC_MASK_UNICODE;
        invoke.lpVerbW = argv[3];
        invoke.lpVerb = "invalid ANSI fallback";
        if (menu->InvokeCommand(reinterpret_cast<LPCMINVOKECOMMANDINFO>(&invoke)) != S_OK)
            return 9;
    }
    CoFreeUnusedLibrariesEx(0, 0);
    if (!registration.remove())
        return 13;
    // The sender has received the original map event. The launched helper must
    // now own its paths, independently of the released COM object and DLL.
    if (canUnload() != S_OK || !FreeLibrary(library))
        return 10;
    struct Processes {
        DWORD assigned;
        DWORD count;
        ULONG_PTR ids[16];
    } processes{};
    if (!QueryInformationJobObject(job, JobObjectBasicProcessIdList, &processes, sizeof(processes),
                                   nullptr) ||
        processes.count < 2) {
        std::fprintf(stderr, "Source map sender returned; job has %lu processes\n",
                     static_cast<unsigned long>(processes.count));
        return 11;
    }
    DWORD helper = 0;
    for (DWORD i = 0; i < processes.count; ++i) {
        const HANDLE process =
            OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, DWORD(processes.ids[i]));
        if (!process)
            continue;
        wchar_t image[32768]{};
        DWORD length = 32768;
        if (QueryFullProcessImageNameW(process, 0, image, &length)) {
            const wchar_t *name = wcsrchr(image, L'\\');
            if (name && _wcsicmp(name + 1, L"7zG.exe") == 0)
                helper = DWORD(processes.ids[i]);
        }
        CloseHandle(process);
    }
    if (!helper)
        return 14;
    std::printf("helper=%lu\n", static_cast<unsigned long>(helper));
    std::fflush(stdout);
    std::getchar(); // Test joins the helper before allowing this host to exit.
    CoUninitialize();
    // CloseHandle would kill this host too. Process exit closes the job after
    // returning our status and kills any unexpected surviving helper.
    return 0;
}

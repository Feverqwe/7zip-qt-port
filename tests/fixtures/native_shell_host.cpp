// A native host fixture deliberately has no Qt application or Qt linkage.
// It consumes the real DLL through the same public COM boundary as Explorer.
#include <cstdio>
#include <shlobj.h>
#include <windows.h>

int wmain(int argc, wchar_t **argv) {
    if (argc != 3)
        return 2;
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(initialized))
        return 3;
    const HMODULE library = LoadLibraryExW(argv[1], nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!library)
        return 4;
    using CanUnload = HRESULT(STDAPICALLTYPE *)();
    using GetClass = HRESULT(STDAPICALLTYPE *)(REFCLSID, REFIID, void **);
    const auto canUnload = reinterpret_cast<CanUnload>(GetProcAddress(library, "DllCanUnloadNow"));
    const auto getClass = reinterpret_cast<GetClass>(GetProcAddress(library, "DllGetClassObject"));
    if (!canUnload || !getClass || canUnload() != S_OK)
        return 5;
    const GUID classID = {
        0x52D06B47, 0xDA74, 0x4A5F, {0x8B, 0x5E, 0x44, 0x94, 0xC7, 0x29, 0xCC, 0x9C}};
    IClassFactory *factory = nullptr;
    if (getClass(classID, IID_IClassFactory, reinterpret_cast<void **>(&factory)) != S_OK)
        return 6;
    IExplorerCommand *root = nullptr;
    if (factory->CreateInstance(nullptr, IID_IExplorerCommand, reinterpret_cast<void **>(&root)) !=
        S_OK)
        return 7;
    IShellItem *item = nullptr;
    if (SHCreateItemFromParsingName(argv[2], nullptr, IID_IShellItem,
                                    reinterpret_cast<void **>(&item)) != S_OK)
        return 8;
    IShellItemArray *items = nullptr;
    if (SHCreateShellItemArrayFromShellItem(item, IID_IShellItemArray,
                                            reinterpret_cast<void **>(&items)) != S_OK)
        return 9;
    LPWSTR title = nullptr;
    if (root->GetTitle(items, &title) != S_OK || !title || wcscmp(title, L"7-Zip") != 0)
        return 10;
    CoTaskMemFree(title);
    IEnumExplorerCommand *enumeration = nullptr;
    if (root->EnumSubCommands(&enumeration) != S_OK)
        return 11;
    IExplorerCommand *child = nullptr;
    ULONG fetched = 0;
    if (enumeration->Next(1, &child, &fetched) != S_OK || fetched != 1 || !child)
        return 12;
    factory->Release();
    root->Release();
    enumeration->Release();
    items->Release();
    item->Release();
    if (canUnload() != S_FALSE)
        return 13;
    title = nullptr;
    if (child->GetTitle(nullptr, &title) != S_OK || !title || !*title)
        return 14;
    CoTaskMemFree(title);
    child->Release();
    if (canUnload() != S_OK)
        return 15;
    if (!FreeLibrary(library))
        return 16;
    CoUninitialize();
    std::puts("native host: factory, selection, child lifetime and unload passed");
    return 0;
}

// Test-only facade over the unchanged official 7z module. All archive work is
// delegated; withholding ISetCompressCodecsInfo exercises DLL-global SetCodecs.
#include "Common/MyInitGuid.h"

#include "7zip/Archive/IArchive.h"
#include "7zip/ICoder.h"
#include "7zip/PropID.h"
#include "Common/MyCom.h"
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#ifndef _WIN32
#include <dlfcn.h>
#include <unistd.h>
#endif

namespace {
void LocateFixture() {}

std::filesystem::path FixtureFolder() {
#ifdef _WIN32
    HMODULE fixture = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&LocateFixture), &fixture))
        return {};
    wchar_t path[32768];
    const DWORD length = GetModuleFileNameW(fixture, path, 32768);
    if (!length || length == 32768)
        return {};
    return std::filesystem::path(std::wstring(path, length)).parent_path();
#else
    Dl_info info{};
    if (!dladdr(reinterpret_cast<void *>(&LocateFixture), &info))
        return {};
    return std::filesystem::path(info.dli_fname).parent_path();
#endif
}

void *CoreFunction(const char *name) {
    const auto folder = FixtureFolder();
    if (folder.empty())
        return nullptr;
#ifdef _WIN32
    const auto core = folder.parent_path() / "7z.dll";
    return reinterpret_cast<void *>(GetProcAddress(GetModuleHandleW(core.c_str()), name));
#else
    const auto path = folder.parent_path() / "7z.so";
    void *module = dlopen(path.c_str(), RTLD_NOW | RTLD_NOLOAD);
    if (!module)
        return nullptr;
    void *function = dlsym(module, name);
    dlclose(module); // CCodecs already owns the module; no extra global lease.
    return function;
#endif
}

HRESULT CoreProperty(PROPID property, PROPVARIANT *value) {
    const auto count =
        reinterpret_cast<Func_GetNumberOfFormats>(CoreFunction("GetNumberOfFormats"));
    const auto get =
        reinterpret_cast<Func_GetHandlerProperty2>(CoreFunction("GetHandlerProperty2"));
    UInt32 formats = 0;
    if (!count || !get || count(&formats) != S_OK)
        return E_FAIL;
    for (UInt32 i = 0; i < formats; ++i) {
        PROPVARIANT name{};
        const HRESULT result = get(i, NArchive::NHandlerPropID::kName, &name);
        const bool is7z =
            result == S_OK && name.vt == VT_BSTR && std::wcscmp(name.bstrVal, L"7z") == 0;
        if (name.vt == VT_BSTR)
            SysFreeString(name.bstrVal);
        if (is7z)
            return get(i, property, value);
    }
    return E_FAIL;
}

class CGlobalOnlyArchive final : public IInArchive, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IInArchive)
    CMyComPtr<IInArchive> archive_;
    std::filesystem::path propertyControl_;

  public:
    explicit CGlobalOnlyArchive(IInArchive *archive) : archive_(archive) {
        // Optional private test resource; existing codec consumers have none.
        std::ifstream path(FixtureFolder() / "resources/fixture/property-control-path.txt",
                           std::ios::binary);
        const std::string utf8(std::istreambuf_iterator<char>(path), {});
        if (!utf8.empty())
            propertyControl_ = std::filesystem::u8path(utf8);
    }
    Z7_COM7F_IMF(Open(IInStream *stream, const UInt64 *limit, IArchiveOpenCallback *callback))
    override { return archive_->Open(stream, limit, callback); }
    Z7_COM7F_IMF(Close()) override { return archive_->Close(); }
    Z7_COM7F_IMF(GetNumberOfItems(UInt32 *count)) override {
        return archive_->GetNumberOfItems(count);
    }
    Z7_COM7F_IMF(GetProperty(UInt32 index, PROPID property, PROPVARIANT *value)) override {
        if (property == kpidAttrib && !propertyControl_.empty()) {
            std::ifstream control(propertyControl_);
            UInt64 faultProcess = 0;
            UInt32 faultIndex = 0, result = 0;
#ifdef _WIN32
            const UInt64 process = GetCurrentProcessId();
#else
            const UInt64 process = getpid();
#endif
            if (control >> faultProcess >> faultIndex >> result && faultProcess == process &&
                index == faultIndex) {
                auto receipt = propertyControl_;
                receipt += ".hit";
                std::ofstream(receipt, std::ios::app) << index << ' ' << result << '\n';
                *value = PROPVARIANT{};
                return static_cast<HRESULT>(static_cast<Int32>(result));
            }
        }
        return archive_->GetProperty(index, property, value);
    }
    Z7_COM7F_IMF(Extract(const UInt32 *indices, UInt32 count, Int32 test,
                         IArchiveExtractCallback *callback))
    override { return archive_->Extract(indices, count, test, callback); }
    Z7_COM7F_IMF(GetArchiveProperty(PROPID property, PROPVARIANT *value)) override {
        return archive_->GetArchiveProperty(property, value);
    }
    Z7_COM7F_IMF(GetNumberOfProperties(UInt32 *count)) override {
        return archive_->GetNumberOfProperties(count);
    }
    Z7_COM7F_IMF(GetPropertyInfo(UInt32 index, BSTR *name, PROPID *property, VARTYPE *type))
    override { return archive_->GetPropertyInfo(index, name, property, type); }
    Z7_COM7F_IMF(GetNumberOfArchiveProperties(UInt32 *count)) override {
        return archive_->GetNumberOfArchiveProperties(count);
    }
    Z7_COM7F_IMF(GetArchivePropertyInfo(UInt32 index, BSTR *name, PROPID *property, VARTYPE *type))
    override { return archive_->GetArchivePropertyInfo(index, name, property, type); }
};
} // namespace

STDAPI GetModuleProp(PROPID property, PROPVARIANT *value) {
    const auto get = reinterpret_cast<Func_GetModuleProp>(CoreFunction("GetModuleProp"));
    return get ? get(property, value) : E_FAIL;
}
STDAPI GetNumberOfFormats(UInt32 *count) {
    *count = 1;
    return S_OK;
}
STDAPI GetHandlerProperty2(UInt32 index, PROPID property, PROPVARIANT *value) {
    if (index)
        return E_INVALIDARG;
    *value = PROPVARIANT{};
    if (property == NArchive::NHandlerPropID::kName ||
        property == NArchive::NHandlerPropID::kExtension) {
        value->vt = VT_BSTR;
        value->bstrVal = SysAllocString(
            property == NArchive::NHandlerPropID::kName ? L"SevenzipFixture7z" : L"fixture7z");
        return value->bstrVal ? S_OK : E_OUTOFMEMORY;
    }
    if (property == NArchive::NHandlerPropID::kUpdate) {
        value->vt = VT_BOOL;
        value->boolVal = VARIANT_FALSE;
        return S_OK;
    }
    return CoreProperty(property, value);
}
STDAPI SetCodecs(ICompressCodecsInfo *codecs) {
    const auto set = reinterpret_cast<Func_SetCodecs>(CoreFunction("SetCodecs"));
    return set ? set(codecs) : E_FAIL;
}
STDAPI CreateObject(const GUID *classID, const GUID *iid, void **object) {
    *object = nullptr;
    std::ifstream asset(FixtureFolder() / "resources/fixture/marker.txt", std::ios::binary);
    if (!asset || std::string(std::istreambuf_iterator<char>(asset), {}) != "fixture resource")
        return E_FAIL;
    if (*iid != IID_IInArchive)
        return E_NOINTERFACE;
    const auto create = reinterpret_cast<Func_CreateObject>(CoreFunction("CreateObject"));
    if (!create)
        return E_FAIL;
    CMyComPtr<IInArchive> archive;
    const HRESULT result = create(classID, iid, reinterpret_cast<void **>(&archive));
    if (result != S_OK)
        return result;
    CMyComPtr<IInArchive> facade = new CGlobalOnlyArchive(archive);
    *object = facade.Detach();
    return S_OK;
}

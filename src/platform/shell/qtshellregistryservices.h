#pragma once
#include "Common/MyString.h"
#include "Common/MyWindows.h"
#include <memory>

namespace NQtShellRegistry {
enum class ERoot { Classes, Machine };
inline constexpr UInt32 kRead = 0x20019;
inline constexpr UInt32 kAllAccess = 0xf003f;
inline constexpr UInt32 kWow64 = 0x100;
inline constexpr UInt32 kWow32 = 0x200;

class IKey {
  public:
    virtual ~IKey() = default;
    virtual LONG QueryValue(const wchar_t *name, UString &value) = 0;
    virtual LONG SetValue(const wchar_t *name, const UString &value) = 0;
    virtual LONG Create(const UString &path, UInt32 access, std::unique_ptr<IKey> &key) = 0;
};
class IServices {
  public:
    virtual ~IServices() = default;
    virtual LONG Open(ERoot root, const UString &path, UInt32 access, bool create,
                      std::unique_ptr<IKey> &key) = 0;
    virtual LONG Delete(ERoot root, const UString &path, UInt32 wow) = 0;
};
// Registry handle transport only; registration policy stays in copied source.
class CKey {
    IServices &_services;
    std::unique_ptr<IKey> _key;

  public:
    explicit CKey(IServices &services) : _services(services) {}
    LONG Open(ERoot root, const UString &path, UInt32 access) {
        return _services.Open(root, path, access, false, _key);
    }
    LONG Create(ERoot root, const UString &path, UInt32, UInt32, UInt32 access) {
        return _services.Open(root, path, access, true, _key);
    }
    LONG Create(CKey &parent, const UString &path, UInt32, UInt32, UInt32 access) {
        return parent._key ? parent._key->Create(path, access, _key) : LONG(6);
    }
    LONG QueryValue(const wchar_t *name, UString &value) {
        return _key ? _key->QueryValue(name, value) : LONG(6);
    }
    LONG SetValue(const wchar_t *name, const UString &value) {
        return _key ? _key->SetValue(name, value) : LONG(6);
    }
};
} // namespace NQtShellRegistry

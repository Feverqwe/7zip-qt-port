// SPDX-License-Identifier: GPL-2.0-only
// Owned archive values and filename projection around Linux xattr operations.
#include "erofsarchiveattributes.h"
#include <cerrno>
#include <cstring>
#include <new>
namespace {
struct Context {
    std::vector<ErofsAttribute> &values;
    HRESULT result = S_OK;
};
int Emit(void *opaque, const char *name, const void *value, size_t size) noexcept {
    auto &context = *static_cast<Context *>(opaque);
    try {
        for (const auto &item : context.values)
            if (item.rawName == name)
                return 0;
        ErofsAttribute item;
        item.rawName = name;
        item.name = L"$XATTR.";
        const wchar_t *hex = L"0123456789ABCDEF";
        for (const unsigned char *p = (const unsigned char *)name; *p; ++p) {
            unsigned char c = *p;
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                c == '.' || c == '_' || c == '-')
                item.name += (wchar_t)c;
            else {
                item.name += L'%';
                item.name += hex[c >> 4];
                item.name += hex[c & 15];
            }
        }
        const UString base = item.name;
        for (unsigned suffix = 0;;) {
            bool collision = false;
            for (const auto &other : context.values)
                if (other.name.IsEqualTo_NoCase(item.name))
                    collision = true;
            if (!collision)
                break;
            item.name = base;
            item.name += L'#';
            item.name.Add_UInt32(++suffix);
        }
        const Byte *bytes = static_cast<const Byte *>(value);
        item.value.assign(bytes, bytes + size);
        context.values.push_back(std::move(item));
        return 0;
    } catch (const std::bad_alloc &) {
        context.result = E_OUTOFMEMORY;
    } catch (...) {
        context.result = E_FAIL;
    }
    return -EIO;
}
} // namespace
HRESULT ReadErofsAttributes(ErofsArchiveReader &reader, erofs_file *file,
                            std::vector<ErofsAttribute> &values) {
    Context context{values};
    const int result = erofs_read_xattrs(file, &context, Emit);
    return context.result != S_OK ? context.result : reader.Result(result);
}

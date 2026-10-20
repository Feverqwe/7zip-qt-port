// SPDX-License-Identifier: GPL-2.0-only
// Shared archive metadata ownership and escaped names for GPL format modules.
#include "archiveattributes.h"
#include <new>
#include <utility>
HRESULT AppendArchiveAttribute(std::vector<ArchiveAttribute> &values, const char *name,
                               const void *value, size_t size) noexcept {
    try {
        for (const auto &item : values)
            if (item.rawName == name)
                return S_OK;
        ArchiveAttribute item;
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
            for (const auto &other : values)
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
        values.push_back(std::move(item));
        return S_OK;
    } catch (const std::bad_alloc &) {
        return E_OUTOFMEMORY;
    } catch (...) {
        return E_FAIL;
    }
}

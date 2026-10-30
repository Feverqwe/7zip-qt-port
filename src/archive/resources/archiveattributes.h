// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_ARCHIVE_ATTRIBUTES_H
#define SEVENZIP_ARCHIVE_ATTRIBUTES_H
#include "7zip/Archive/IArchive.h"
#include "Common/MyString.h"
#include <vector>
struct ArchiveAttribute {
    AString rawName;
    UString name;
    std::vector<Byte> value;
};
HRESULT AppendArchiveAttribute(std::vector<ArchiveAttribute> &, const char *, const void *,
                               size_t) noexcept;
#endif

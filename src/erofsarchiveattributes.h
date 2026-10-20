// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_EROFS_ARCHIVE_ATTRIBUTES_H
#define SEVENZIP_EROFS_ARCHIVE_ATTRIBUTES_H
#include "Common/MyString.h"
#include "erofsarchivereader.h"
#include <vector>
struct ErofsAttribute {
    AString rawName;
    UString name;
    std::vector<Byte> value;
};
HRESULT ReadErofsAttributes(ErofsArchiveReader &, erofs_file *, std::vector<ErofsAttribute> &);
#endif

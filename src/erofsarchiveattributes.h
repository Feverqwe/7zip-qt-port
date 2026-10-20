// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_EROFS_ARCHIVE_ATTRIBUTES_H
#define SEVENZIP_EROFS_ARCHIVE_ATTRIBUTES_H
#include "Common/MyString.h"
#include "archiveattributes.h"
#include "erofsarchivereader.h"
#include <vector>
using ErofsAttribute = ArchiveAttribute;
HRESULT ReadErofsAttributes(ErofsArchiveReader &, erofs_file *, std::vector<ErofsAttribute> &);
#endif

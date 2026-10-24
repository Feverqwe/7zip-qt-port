// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_BTRFS_ARCHIVE_ATTRIBUTES_H
#define SEVENZIP_BTRFS_ARCHIVE_ATTRIBUTES_H
#include "Common/MyString.h"
#include "archiveattributes.h"
#include "btrfsarchivereader.h"
#include <vector>
using BtrfsAttribute = ArchiveAttribute;
HRESULT ReadBtrfsAttributes(BtrfsArchiveReader &, btrfs_file *, std::vector<BtrfsAttribute> &);
#endif

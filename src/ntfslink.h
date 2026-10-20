// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_NTFS_LINK_H
#define SEVENZIP_NTFS_LINK_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
enum sevenzip_ntfs_reparse_type {
    SEVENZIP_NTFS_REPARSE_NONE,
    SEVENZIP_NTFS_REPARSE_COMPRESSED,
    SEVENZIP_NTFS_REPARSE_DEDUPLICATED,
    SEVENZIP_NTFS_REPARSE_LINK
};
int sevenzip_ntfs_classify_reparse(const void *, size_t, unsigned *);
int sevenzip_ntfs_readlink(const void *, size_t, const char *, char *, size_t);
#ifdef __cplusplus
}
#endif
#endif

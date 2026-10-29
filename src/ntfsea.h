// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_NTFS_EA_H
#define SEVENZIP_NTFS_EA_H
#include "ntfsdecoder.h"
#ifdef __cplusplus
extern "C" {
#endif
struct sevenzip_ntfs_ea_input {
    const void *info;
    size_t info_size;
    const void *resident;
    uint64_t data_size;
    uint32_t max_size;
    int nonresident;
    sevenzip_ntfs_read_fn read;
    void *opaque;
};
struct sevenzip_ntfs_wsl_perm {
    uint32_t uid, gid, mode, device;
    int present;
};
int sevenzip_ntfs_attribute_limits(const void *, size_t, uint32_t *, uint64_t *);
int sevenzip_ntfs_wsl_read(const struct sevenzip_ntfs_ea_input *, struct sevenzip_ntfs_wsl_perm *);
typedef int (*sevenzip_ntfs_ea_visit_fn)(void *, const char *, size_t, const void *, size_t);
int sevenzip_ntfs_ea_visit(const struct sevenzip_ntfs_ea_input *, sevenzip_ntfs_ea_visit_fn,
                           void *);
#ifdef __cplusplus
}
#endif
#endif

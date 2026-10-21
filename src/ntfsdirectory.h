// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_NTFS_DIRECTORY_H
#define SEVENZIP_NTFS_DIRECTORY_H
#include "ntfssecurity.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef int (*sevenzip_ntfs_directory_emit_fn)(void *, uint64_t, const void *, size_t);
struct sevenzip_ntfs_directory_input {
    const void *root;
    size_t root_size;
    uint64_t bitmap_size, bitmap_valid, allocation_size, parent_ref;
    unsigned cluster_bits;
    sevenzip_ntfs_security_read_fn read;
    sevenzip_ntfs_directory_emit_fn emit;
    void *opaque;
};
int sevenzip_ntfs_directory_read(const struct sevenzip_ntfs_directory_input *);
#ifdef __cplusplus
}
#endif
#endif

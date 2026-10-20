// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_NTFS_SECURITY_H
#define SEVENZIP_NTFS_SECURITY_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int (*sevenzip_ntfs_security_read_fn)(void *, unsigned, uint64_t, void *, size_t);
struct sevenzip_ntfs_security_input {
    const void *root;
    size_t root_size;
    uint64_t sds_size, allocation_size;
    unsigned cluster_bits;
    sevenzip_ntfs_security_read_fn read;
    void *opaque;
};
struct sevenzip_ntfs_security;
int sevenzip_ntfs_security_create(const struct sevenzip_ntfs_security_input *,
                                  struct sevenzip_ntfs_security **);
void sevenzip_ntfs_security_destroy(struct sevenzip_ntfs_security *);
int sevenzip_ntfs_security_read(struct sevenzip_ntfs_security *, uint32_t, void *, size_t,
                                size_t *);
#ifdef __cplusplus
}
#endif
#endif

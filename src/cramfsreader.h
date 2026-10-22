// SPDX-License-Identifier: MIT
// Stream boundary for the Linux cramfs read-side adaptation.
#ifndef SEVENZIP_CRAMFS_READER_H
#define SEVENZIP_CRAMFS_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct cramfs_source {
    const unsigned char *data;
    size_t size;
    int big_endian;
    void *context;
    int (*inflate)(void *, void *, size_t, const void *, size_t);
    int (*read)(void *, uint32_t, void *, size_t);
};
struct cramfs_extent {
    uint32_t start, length;
    int uncompressed;
};
int cramfs_map_block(const struct cramfs_source *, uint32_t table, uint32_t file_size,
                     uint32_t block, struct cramfs_extent *);
int cramfs_read_block(const struct cramfs_source *, uint32_t table, uint32_t file_size,
                      uint32_t block, unsigned char page[4096]);
#ifdef __cplusplus
}
#endif
#endif

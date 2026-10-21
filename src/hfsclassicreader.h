// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_HFS_CLASSIC_READER_H
#define SEVENZIP_HFS_CLASSIC_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sevenzip_hfs_classic;
typedef int (*sevenzip_hfs_classic_read_fn)(void *, uint64_t, void *, size_t);
struct sevenzip_hfs_classic_volume {
    uint32_t block_size, blocks, free_blocks, files, folders, created, modified;
    uint64_t allocation_start;
    int unclean;
};
struct sevenzip_hfs_classic_item {
    uint32_t id, parent, mode, created, modified;
    uint32_t size[2], blocks[2];
    uint8_t name[31], name_length;
    uint8_t finder_type[4], finder_creator[4];
};
int sevenzip_hfs_classic_open(sevenzip_hfs_classic_read_fn read, void *context,
                              uint64_t size, struct sevenzip_hfs_classic **state,
                              struct sevenzip_hfs_classic_volume *volume);
/* Returns one item, zero at catalog EOF, or a negative errno. */
int sevenzip_hfs_classic_next(struct sevenzip_hfs_classic *state,
                              struct sevenzip_hfs_classic_item *item);
int sevenzip_hfs_classic_map(struct sevenzip_hfs_classic *state, int resource,
                             uint32_t logical_block, uint32_t *physical_block);
void sevenzip_hfs_classic_close(struct sevenzip_hfs_classic *state);
#ifdef __cplusplus
}
#endif
#endif

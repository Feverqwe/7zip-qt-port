// SPDX-License-Identifier: MIT
// Read-only archive boundary for Linux ext4 metadata services.
#ifndef SEVENZIP_EXT_READER_H
#define SEVENZIP_EXT_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct ext_volume;
struct ext_node;
uint16_t crc16(uint16_t, const unsigned char *, size_t);
struct ext_time { int64_t seconds; uint32_t nsec; int defined; };
struct ext_node_info {
    uint64_t size;
    uint32_t checksum_seed, inline_size;
    struct ext_time atime, mtime, change_time, birth_time;
};
/* All buffers remain owned by the caller except the returned opaque objects. */
int ext_linux_open(const void *superblock, struct ext_volume **);
void ext_linux_close(struct ext_volume *);
uint64_t ext_linux_descriptor_block(struct ext_volume *, unsigned int number);
int ext_linux_group_check(struct ext_volume *, uint32_t group, const void *descriptor);
int ext_linux_bitmap_check(struct ext_volume *, const void *descriptor,
                           const void *bitmap, int inode_bitmap);
int ext_linux_inode_open(struct ext_volume *, uint32_t ino, const void *raw,
                         struct ext_node **, struct ext_node_info *);
void ext_linux_inode_close(struct ext_node *);
int ext_linux_inline_read(struct ext_node *, void *output, unsigned int size);
typedef int (*ext_dir_emit)(void *, uint32_t ino, unsigned int type, const char *, unsigned int);
int ext_linux_inline_directory(struct ext_node *, ext_dir_emit, void *context);
int ext_linux_xattr_check(struct ext_node *, uint64_t block, const void *data);
int ext_linux_extent_check(struct ext_volume *, uint32_t seed, const void *block);
int ext_linux_directory_check(struct ext_volume *, uint32_t seed, uint32_t flags, const void *block);
#ifdef __cplusplus
}
#endif
#endif

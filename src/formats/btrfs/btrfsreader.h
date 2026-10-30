/* SPDX-License-Identifier: MIT */
/* Read-only stream boundary for the Linux Btrfs adaptation. */
#ifndef SEVENZIP_BTRFS_READER_H
#define SEVENZIP_BTRFS_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct btrfs_volume;
struct btrfs_file;
struct btrfs_device_info {
    uint64_t devid, bytes, incompat;
    uint8_t uuid[16], fsid[16], metadata_uuid[16];
};
int btrfs_inspect_device(const void *superblock, size_t, struct btrfs_device_info *);
/* NULL when the on-disk feature set can use this read path. */
const char *btrfs_read_limitation(uint64_t incompat);
int btrfs_checksum_native(unsigned int, const void *, size_t, uint8_t result[32]);
int btrfs_decode_native(void *, unsigned int algorithm, unsigned int sectorsize, int inline_extent,
                        const void *, size_t, void *, size_t);
struct btrfs_source {
    void *context;
    uint64_t size;
    /* Device 0 denotes the initially supplied stream before its ID is known.
     * Callbacks return zero on success or a negative errno on failure. */
    int (*read)(void *, uint64_t devid, const uint8_t uuid[16], uint64_t, void *, size_t);
    int (*open_device)(void *, uint64_t devid, const uint8_t uuid[16]);
    int (*poll)(void *);
    int (*checksum)(unsigned int, const void *, size_t, uint8_t result[32]);
    int (*decode)(void *, unsigned int, unsigned int sectorsize, int inline_extent, const void *,
                  size_t, void *, size_t);
};
struct btrfs_volume_info {
    uint64_t size, used, generation, devices, incompat;
    uint32_t sectorsize, nodesize;
    uint16_t checksum;
    uint8_t uuid[16];
    char label[257];
    int log_pending;
};
struct btrfs_file_info {
    uint64_t root, ino, size, packed_size, flags, nlink;
    int64_t mtime, atime, ctime, otime;
    uint32_t nsec, atime_nsec, ctime_nsec, otime_nsec, mode, uid, gid;
    uint64_t rdev;
};
int btrfs_open_volume(const struct btrfs_source *, struct btrfs_volume **);
void btrfs_close_volume(struct btrfs_volume *);
void btrfs_get_volume_info(const struct btrfs_volume *, struct btrfs_volume_info *);
/* Subvolume IDs are part of file identity. Inode 0 requests that root's directory. */
int btrfs_open_file(struct btrfs_volume *, uint64_t root, uint64_t ino, struct btrfs_file **);
void btrfs_close_file(struct btrfs_file *);
void btrfs_get_file_info(const struct btrfs_file *, struct btrfs_file_info *);
int btrfs_pread_file(struct btrfs_file *, uint64_t, void *, size_t, size_t *);
typedef int (*btrfs_dir_emit)(void *, const char *, size_t, uint64_t root, uint64_t ino,
                              unsigned int type);
int btrfs_read_directory(struct btrfs_file *, void *, btrfs_dir_emit);
typedef int (*btrfs_xattr_emit)(void *, const char *, const void *, size_t);
int btrfs_read_xattrs(struct btrfs_file *, void *, btrfs_xattr_emit);
#ifdef __cplusplus
}
#endif
#endif

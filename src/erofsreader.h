// SPDX-License-Identifier: MIT
// Read-only stream boundary for the Linux EROFS adaptation.
#ifndef SEVENZIP_EROFS_READER_H
#define SEVENZIP_EROFS_READER_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct erofs_volume;
int erofs_decode_native(unsigned int algorithm, unsigned int dictionary, const void *, size_t,
                         void *, size_t, int partial);
// Files borrow the volume; close them before closing their volume.
struct erofs_file;
struct erofs_source {
    void *context;
    uint64_t size;
    // Device 0 is the primary image. External-device mode uses the slot index + 1.
    int (*read)(void *, unsigned int, uint64_t, void *, size_t);
    int (*poll)(void *);
    int (*decode)(void *, unsigned int algorithm, unsigned int dictionary,
                  const void *, size_t, void *, size_t, int partial);
    int (*open_device)(void *, unsigned int, const char *tag, uint64_t size);
    int external_devices;
};
struct erofs_item {
    uint64_t nid, size, packed_size;
    int64_t mtime;
    uint32_t nsec, mode, uid, gid, nlink;
    // Linux internal dev_t: 12-bit major above a 20-bit minor; zero for FIFO/socket.
    uint32_t rdev;
    unsigned int layout;
};
struct erofs_volume_info {
    uint64_t size, root_nid, inode_count;
    uint32_t block_size, compat, incompat;
    char name[17];
    uint8_t uuid[16]; // Raw filesystem UUID bytes, in on-disk order.
    unsigned int extra_devices;
};
int erofs_open_volume(const struct erofs_source *, struct erofs_volume **);
void erofs_close_volume(struct erofs_volume *);
void erofs_get_volume_info(const struct erofs_volume *, struct erofs_volume_info *);
int erofs_device_tag(const struct erofs_volume *, unsigned int, char tag[65]);
int erofs_open_file(struct erofs_volume *, uint64_t nid, struct erofs_file **);
void erofs_close_file(struct erofs_file *);
void erofs_get_file_info(const struct erofs_file *, struct erofs_item *);
int erofs_pread_file(struct erofs_file *, uint64_t, void *, size_t, size_t *);
// Callback: 0 to continue; nonzero to stop. Its result is returned to the caller.
// The last argument uses Linux DT_* dirent types; names are borrowed during emit.
int erofs_read_directory(struct erofs_file *, void *,
                         int (*emit)(void *, const char *, size_t, uint64_t, unsigned int));
// Values use Linux public namespaces and VFS ACL serialization. Borrowed during emit.
typedef int (*erofs_xattr_emit)(void *, const char *, const void *, size_t);
int erofs_read_xattrs(struct erofs_file *, void *, erofs_xattr_emit);
#ifdef __cplusplus
}
#endif
#endif

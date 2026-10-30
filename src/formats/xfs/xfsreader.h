// SPDX-License-Identifier: GPL-3.0-or-later
// Local stream/ownership boundary around the GRUB XFS C reader.
#ifndef SEVENZIP_XFS_READER_H
#define SEVENZIP_XFS_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct xfs_volume xfs_volume;
enum xfs_error { XFS_OK, XFS_BAD_FS, XFS_RANGE, XFS_UNSUPPORTED, XFS_NO_MEMORY, XFS_IO };
typedef struct {
    void *context;
    uint64_t size;
    int (*read)(void *, uint64_t, void *, size_t);
    int (*poll)(void *);
} xfs_source;
typedef struct {
    uint64_t inode, size, blocks;
    int64_t mtime;
    uint32_t mode;
} xfs_entry;
typedef int (*xfs_visit)(void *, const char *, const xfs_entry *);
int xfs_open(const xfs_source *, xfs_volume **);
void xfs_close(xfs_volume *);
uint64_t xfs_root(const xfs_volume *);
uint64_t xfs_size(const xfs_volume *);
uint32_t xfs_block_size(const xfs_volume *);
int xfs_list(xfs_volume *, uint64_t, xfs_visit, void *);
int xfs_read(xfs_volume *, uint64_t, uint64_t, void *, size_t, size_t *);
#ifdef __cplusplus
}
#endif
#endif

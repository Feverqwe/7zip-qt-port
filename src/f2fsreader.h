// SPDX-License-Identifier: MIT
#ifndef SEVENZIP_F2FS_READER_H
#define SEVENZIP_F2FS_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { F2FS_BAD = 1, F2FS_UNSUPPORTED = 2, F2FS_IO = 3, F2FS_MEMORY = 4 };
struct f2fs_volume;
struct f2fs_file;
struct f2fs_source {
    void *context;
    uint64_t size;
    int (*read)(void *, uint64_t, void *, size_t);
    int (*poll)(void *);
};
struct f2fs_item {
    uint64_t size, mtime, atime, ctime;
    uint32_t ino, mode, uid, gid;
    int unsupported;
};
int f2fs_open_volume(const struct f2fs_source *, struct f2fs_volume **);
void f2fs_close_volume(struct f2fs_volume *);
uint32_t f2fs_root(const struct f2fs_volume *);
int f2fs_open_file(struct f2fs_volume *, uint32_t, struct f2fs_file **);
void f2fs_close_file(struct f2fs_file *);
void f2fs_file_info(const struct f2fs_file *, struct f2fs_item *);
int f2fs_pread(struct f2fs_file *, uint64_t, void *, size_t, size_t *);
int f2fs_directory(struct f2fs_file *, void *, int (*)(void *, const char *, uint32_t));
#ifdef __cplusplus
}
#endif
#endif

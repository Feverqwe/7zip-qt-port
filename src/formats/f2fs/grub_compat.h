// SPDX-License-Identifier: GPL-3.0-or-later
// fshelp node/hook declarations adapted from GRUB include/grub/fshelp.h.
// Copyright (C) 2004-2008 Free Software Foundation, Inc.
// Narrow read-only host boundary. No GRUB device writes, VFS or module loader.
#ifndef SEVENZIP_F2FS_GRUB_COMPAT_H
#define SEVENZIP_F2FS_GRUB_COMPAT_H
#include "formats/f2fs/f2fsreader.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t grub_uint8_t;
typedef uint16_t grub_uint16_t;
typedef uint32_t grub_uint32_t;
typedef uint64_t grub_uint64_t;
typedef uint64_t grub_disk_addr_t;
typedef uint64_t grub_off_t;
typedef size_t grub_size_t;
typedef ptrdiff_t grub_ssize_t;
typedef int grub_err_t;
typedef void *grub_disk_read_hook_t;
#define GRUB_PACKED
#define GRUB_DISK_SECTOR_BITS 9
#define GRUB_ERR_NONE 0
#define GRUB_ERR_BAD_FS F2FS_BAD
#define GRUB_ERR_OUT_OF_RANGE F2FS_BAD
#define N_(x) x
#define grub_memcpy memcpy
#define grub_memset memset
#define grub_free free
#ifdef _MSC_VER
__declspec(thread) static int grub_errno;
#else
static _Thread_local int grub_errno;
#endif
static int grub_error(int error, const char *message) {
    (void)message;
    return grub_errno = error;
}
static void *grub_malloc(size_t size) {
    void *p = malloc(size);
    if (!p)
        grub_errno = F2FS_MEMORY;
    return p;
}
static void *grub_zalloc(size_t size) {
    void *p = grub_malloc(size);
    if (p)
        memset(p, 0, size);
    return p;
}
// Supported hosts are little endian (the disk structs are packed below).
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error F2FS host endian adapter required
#endif
#define grub_le_to_cpu16(x) (x)
#define grub_le_to_cpu32(x) (x)
#define grub_le_to_cpu64(x) (x)
#define grub_cpu_to_le32_compile_time(x) (x)
#define grub_cpu_to_le64_compile_time(x) (x)
#define grub_add(a, b, out) ((*(out) = (a) + (b)), *(out) < (a))
struct grub_disk {
    struct f2fs_source source;
    grub_disk_read_hook_t read_hook;
    void *read_hook_data;
};
typedef struct grub_disk *grub_disk_t;
static int grub_disk_read(grub_disk_t disk, uint64_t sector, size_t offset, size_t size,
                          void *output) {
    if (grub_errno)
        return grub_errno;
    if (disk->source.poll && disk->source.poll(disk->source.context))
        return grub_errno = F2FS_IO;
    if (sector > (UINT64_MAX - offset) / 512)
        return grub_errno = F2FS_BAD;
    uint64_t pos = sector * 512 + offset;
    if (pos > disk->source.size || size > disk->source.size - pos)
        return grub_errno = F2FS_BAD;
    if (disk->source.read(disk->source.context, pos, output, size))
        return grub_errno = F2FS_IO;
    return 0;
}
typedef struct grub_fshelp_node *grub_fshelp_node_t;
enum grub_fshelp_filetype {
    GRUB_FSHELP_UNKNOWN,
    GRUB_FSHELP_REG,
    GRUB_FSHELP_DIR,
    GRUB_FSHELP_SYMLINK
};
typedef int (*grub_fshelp_iterate_dir_hook_t)(const char *, enum grub_fshelp_filetype,
                                              grub_fshelp_node_t, void *);
static grub_ssize_t grub_fshelp_read_file(
    grub_disk_t, grub_fshelp_node_t, grub_disk_read_hook_t, void *, grub_off_t, grub_size_t, char *,
    grub_disk_addr_t (*)(grub_fshelp_node_t, grub_disk_addr_t), grub_off_t, int, grub_disk_addr_t);
#endif

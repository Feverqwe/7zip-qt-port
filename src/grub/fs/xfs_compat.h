// SPDX-License-Identifier: GPL-3.0-or-later
// Narrow host services for the GRUB read-only filesystem code.
// Types/fshelp declarations and overflow semantics: docs/xfs-grub-baseline.json.
// Copyright (C) 2002,2004,2005,2006,2007,2008,2009,2020 Free Software Foundation, Inc.
// GRUB is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License, version 3 or (at your option) later.
// Distributed WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See third_party/grub/COPYING.
#ifndef SEVENZIP_XFS_COMPAT_H
#define SEVENZIP_XFS_COMPAT_H
#include "xfsreader.h"
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t grub_uint8_t;
typedef uint16_t grub_uint16_t;
typedef uint32_t grub_uint32_t;
typedef uint64_t grub_uint64_t;
typedef int64_t grub_int64_t;
typedef uint64_t grub_disk_addr_t;
typedef uint64_t grub_off_t;
typedef size_t grub_size_t;
typedef intptr_t grub_ssize_t;
typedef uintptr_t grub_addr_t;
typedef int grub_err_t;
typedef void (*grub_disk_read_hook_t)(void);
#define GRUB_PACKED
#define GRUB_DISK_SECTOR_BITS 9
#define GRUB_INT32_MIN INT32_MIN
#define NSEC_PER_SEC 1000000000
#define PRIuGRUB_UINT64_T PRIu64
#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))
#define N_(s) s
enum {
    GRUB_ERR_NONE = XFS_OK,
    GRUB_ERR_BAD_FS = XFS_BAD_FS,
    GRUB_ERR_OUT_OF_RANGE = XFS_RANGE,
    GRUB_ERR_NOT_IMPLEMENTED_YET = XFS_UNSUPPORTED,
    GRUB_ERR_OUT_OF_MEMORY = XFS_NO_MEMORY,
    GRUB_ERR_READ_ERROR = XFS_IO
};
struct grub_disk {
    xfs_source source;
    int error;
    grub_disk_read_hook_t read_hook;
    void *read_hook_data;
};
typedef struct grub_disk *grub_disk_t;
static inline uint64_t grub_divmod64(uint64_t value, uint64_t divisor, uint64_t *remainder) {
    if (remainder)
        *remainder = value % divisor;
    return value / divisor;
}
#define grub_errno (disk->error)
#define grub_error(code, ...) (disk->error = (code))
#define grub_dprintf(...) ((void)0)
// Preserve the first failure; unlike a bootloader, archive listing must not
// clear an unreadable child inode and report a complete successful listing.
#define grub_print_error() ((void)0)
static inline void *xfs_alloc(grub_disk_t disk, size_t size) {
    void *p = malloc(size);
    if (!p)
        disk->error = GRUB_ERR_OUT_OF_MEMORY;
    return p;
}
static inline void *xfs_zalloc(grub_disk_t disk, size_t size) {
    void *p = calloc(1, size);
    if (!p)
        disk->error = GRUB_ERR_OUT_OF_MEMORY;
    return p;
}
static inline void *xfs_realloc(grub_disk_t disk, void *old, size_t size) {
    void *p = realloc(old, size);
    if (!p)
        disk->error = GRUB_ERR_OUT_OF_MEMORY;
    return p;
}
#define grub_malloc(n) xfs_alloc(disk, (n))
#define grub_zalloc(n) xfs_zalloc(disk, (n))
#define grub_realloc(p, n) xfs_realloc(disk, (p), (n))
#define grub_free free
#define grub_strncmp strncmp
#define grub_memcpy memcpy
#define grub_memset memset
#define grub_add(a, b, out)                                                                        \
    ((uint64_t)(a) > UINT64_MAX - (uint64_t)(b) ? 1 : (*(out) = (a) + (b), 0))
#define grub_mul(a, b, out)                                                                        \
    ((uint64_t)(b) && (uint64_t)(a) > UINT64_MAX / (uint64_t)(b) ? 1 : (*(out) = (a) * (b), 0))
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define grub_be_to_cpu16(x) ((uint16_t)(x))
#define grub_be_to_cpu32(x) ((uint32_t)(x))
#define grub_be_to_cpu64(x) ((uint64_t)(x))
#else
#ifdef _MSC_VER
#include <stdlib.h>
#define __builtin_bswap16 _byteswap_ushort
#define __builtin_bswap32 _byteswap_ulong
#define __builtin_bswap64 _byteswap_uint64
#endif
#define grub_be_to_cpu16(x) __builtin_bswap16((uint16_t)(x))
#define grub_be_to_cpu32(x) __builtin_bswap32((uint32_t)(x))
#define grub_be_to_cpu64(x) __builtin_bswap64((uint64_t)(x))
#endif
#define grub_cpu_to_be16_compile_time grub_be_to_cpu16
#define grub_cpu_to_be32_compile_time grub_be_to_cpu32
#define grub_cpu_to_be64_compile_time grub_be_to_cpu64
#define grub_cpu_to_be64 grub_be_to_cpu64
static inline uint16_t grub_get_unaligned16(const void *p) {
    uint16_t v;
    memcpy(&v, p, 2);
    return v;
}
static inline uint32_t grub_get_unaligned32(const void *p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}
static inline uint64_t grub_get_unaligned64(const void *p) {
    uint64_t v;
    memcpy(&v, p, 8);
    return v;
}
static inline char *xfs_strndup(grub_disk_t disk, const char *s, size_t n) {
    char *p = xfs_alloc(disk, n + 1);
    if (p) {
        memcpy(p, s, n);
        p[n] = 0;
    }
    return p;
}
#define grub_strndup(s, n) xfs_strndup(disk, (s), (n))
int grub_disk_read(grub_disk_t, uint64_t, uint64_t, size_t, void *);
int xfs_metadata_checksum(void *, size_t, size_t);
typedef struct grub_fshelp_node *grub_fshelp_node_t;
enum grub_fshelp_filetype {
    GRUB_FSHELP_UNKNOWN,
    GRUB_FSHELP_REG,
    GRUB_FSHELP_DIR,
    GRUB_FSHELP_SYMLINK
};
typedef int (*grub_fshelp_iterate_dir_hook_t)(const char *, enum grub_fshelp_filetype,
                                              grub_fshelp_node_t, void *);
grub_ssize_t grub_fshelp_read_file(grub_disk_t, grub_fshelp_node_t, grub_disk_read_hook_t, void *,
                                   grub_off_t, grub_size_t, char *,
                                   grub_disk_addr_t (*)(grub_fshelp_node_t, grub_disk_addr_t),
                                   grub_off_t, int, grub_disk_addr_t);
#endif

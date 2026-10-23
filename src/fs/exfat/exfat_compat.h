// SPDX-License-Identifier: GPL-2.0-or-later
// Userspace replacements for the read-side Linux platform services.
#ifndef SEVENZIP_EXFAT_COMPAT_H
#define SEVENZIP_EXFAT_COMPAT_H
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/linux/string.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef uint8_t __u8;
typedef uint16_t __le16;
typedef uint32_t __le32;
typedef uint64_t __le64;
typedef uint64_t sector_t;
typedef int64_t exfat_blkcnt_t;
#define blkcnt_t exfat_blkcnt_t
typedef int64_t exfat_loff_t;
#define loff_t exfat_loff_t
struct timespec64 {
    int64_t tv_sec;
    long tv_nsec;
};
#define __packed
#define SECTOR_SIZE 512
#define PAGE_SIZE 4096
#define BITS_PER_BYTE 8
#define GFP_NOFS 0
#define unlikely(x) (x)
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define DIV_ROUND_UP(x, y) (((x) + (y) - 1) / (y))
#ifdef min
#undef min
#endif
#define min(x, y) ((x) < (y) ? (x) : (y))
#define min_t(t, x, y) min((t)(x), (t)(y))
#define MAX_LFS_FILESIZE INT64_MAX
#define kmalloc_objs(obj, count, flags) calloc((count), sizeof(obj))
#define kvmalloc_objs(type, count) calloc((count), sizeof(type))
#define kfree free
#define kvfree free

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define le16_to_cpu(x) __builtin_bswap16(x)
#define cpu_to_le16(x) __builtin_bswap16(x)
#define le32_to_cpu(x) __builtin_bswap32(x)
#define le64_to_cpu(x) __builtin_bswap64(x)
#else
#define le16_to_cpu(x) (x)
#define cpu_to_le16(x) (x)
#define le32_to_cpu(x) (x)
#define le64_to_cpu(x) (x)
#endif

struct super_block {
    void *s_fs_info;
    unsigned long s_blocksize;
    unsigned int s_blocksize_bits;
    int64_t s_maxbytes;
    uint64_t image_size;
    void *context;
    int (*read)(void *, uint64_t, void *, size_t);
    int (*poll)(void *);
    bool directory_error;
    char error[256];
};
struct buffer_head {
    char *b_data;
    uint64_t b_blocknr;
};
struct buffer_head *sb_bread(struct super_block *, sector_t);
void brelse(struct buffer_head *);
#define __brelse brelse
#define bforget brelse
#define buffer_uptodate(bh) ((bh) != NULL)
int sb_set_blocksize(struct super_block *, unsigned int);
#define sb_min_blocksize sb_set_blocksize
static inline bool is_power_of_2(unsigned int n) { return n && !(n & (n - 1)); }
static inline bool test_bit_le(unsigned int bit, const void *data) {
    return (((const uint8_t *)data)[bit / 8] & (1u << (bit % 8))) != 0;
}
// Readahead is a kernel performance service; userspace reads on demand.
#define exfat_blk_readahead(...) (0)
#define exfat_err(sb, ...) ((void)snprintf((sb)->error, sizeof((sb)->error), __VA_ARGS__))
#define exfat_fs_error exfat_err
#define exfat_fs_error_ratelimit exfat_err
#define exfat_warn(...) ((void)0)
#endif

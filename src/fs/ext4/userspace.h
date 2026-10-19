/* SPDX-License-Identifier: GPL-2.0-only */
/* Host services for the transferred Linux ext4 read-side routines. */
#ifndef SEVENZIP_EXT4_USERSPACE_H
#define SEVENZIP_EXT4_USERSPACE_H
#include "../../extreader.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
typedef uint8_t u8, __u8;
typedef uint16_t u16, __u16, __le16;
typedef uint32_t u32, __u32, __le32, ext4_group_t;
typedef uint64_t u64, __u64, __le64, ext4_fsblk_t, sector_t;
typedef int64_t loff_t, time64_t;
#ifndef ENODATA
#define ENODATA 61
#endif
#define EFSCORRUPTED EIO
#define EFSBADCRC EIO
#define __nonstring
#define likely(x) (x)
#define unlikely(x) (x)
#define min_t(type, a, b) ((type)(a) < (type)(b) ? (type)(a) : (type)(b))
#define BUG_ON(condition) do { if (condition) return -EFSCORRUPTED; } while (0)
#define GFP_NOFS 0
#define kmalloc(size, flags) malloc(size)
#define kfree free
#define down_read(...) ((void)0)
#define up_read(...) ((void)0)
#define lock_buffer(...) ((void)0)
#define unlock_buffer(...) ((void)0)
static inline u16 ext_load_le16(const void *address) {
    const u8 *p = address;
    return (u16)p[0] | (u16)p[1] << 8;
}
static inline u32 ext_load_le32(const void *address) {
    const u8 *p = address;
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}
static inline u16 cpu_to_le16(u16 value) {
    u16 result; u8 *p = (u8 *)&result;
    p[0] = (u8)value; p[1] = (u8)(value >> 8); return result;
}
static inline u32 cpu_to_le32(u32 value) {
    u32 result; u8 *p = (u8 *)&result;
    for (unsigned int i = 0; i < 4; ++i) p[i] = (u8)(value >> (i * 8));
    return result;
}
static inline u64 cpu_to_le64(u64 value) {
    u64 result; u8 *p = (u8 *)&result;
    for (unsigned int i = 0; i < 8; ++i) p[i] = (u8)(value >> (i * 8));
    return result;
}
static inline u16 le16_to_cpu(u16 value) { return ext_load_le16(&value); }
static inline u32 le32_to_cpu(u32 value) { return ext_load_le32(&value); }
u32 crc32c(u32, const void *, size_t);
static inline u32 ext4_chksum(u32 seed, const void *data, size_t size) {
    return crc32c(seed, data, size);
}
u16 crc16(u16, const u8 *, size_t);
struct timespec64 { time64_t tv_sec; long tv_nsec; };
struct super_block;
struct inode;
struct file;
struct buffer_head { char *b_data; size_t b_size; u64 b_blocknr; bool verified; };
#define buffer_verified(bh) ((bh)->verified)
#define set_buffer_verified(bh) ((bh)->verified = true)
#define brelse(...) ((void)0)
#endif

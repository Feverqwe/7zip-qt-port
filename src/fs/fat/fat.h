/* SPDX-License-Identifier: GPL-2.0-only */
/* Userspace services for the transferred FAT/NLS read-side routines. */
#ifndef SEVENZIP_LINUX_FAT_H
#define SEVENZIP_LINUX_FAT_H
#include "../../fatreader.h"
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include "../nls/types.h"
typedef uint8_t u8, __u8;
typedef uint16_t u16, __le16;
typedef uint32_t u32, __u32, __le32;
typedef uint64_t u64, sector_t;
typedef int64_t time64_t, __kernel_off_t;
static inline u16 fat_load_le16(const void *address) {
    const u8 *p = (const u8 *)address;
    return p[0] | (u16)p[1] << 8;
}
static inline u16 le16_to_cpu(u16 value) { return fat_load_le16(&value); }
static inline u16 cpu_to_le16(u16 value) {
    u16 result;
    u8 *p = (u8 *)&result;
    p[0] = (u8)value;
    p[1] = (u8)(value >> 8);
    return result;
}
static inline u16 __be16_to_cpu(u16 value) {
    const u8 *p = (const u8 *)&value;
    return (u16)p[0] << 8 | p[1];
}
#define __le16_to_cpu le16_to_cpu
#define get_unaligned_le16 fat_load_le16
static inline u32 fat_load_le32(const void *address) {
    const u8 *p = (const u8 *)address;
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}
static inline u32 le32_to_cpu(u32 value) { return fat_load_le32(&value); }
#define get_unaligned_le32 fat_load_le32
static inline bool is_power_of_2(unsigned value) { return value && !(value & (value - 1)); }
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define likely(x) (x)
#define unlikely(x) (x)
#define fat_msg(...) ((void)0)
#define EXPORT_SYMBOL(...)
#define KERN_ERR ""
#define KERN_WARNING ""
#define KERN_INFO ""
#define VFAT_SFN_DISPLAY_LOWER 0x0001
#define VFAT_SFN_DISPLAY_WIN95 0x0002
#define VFAT_SFN_DISPLAY_WINNT 0x0004
#define KB_IN_SECTORS 2
static inline unsigned char *hex_byte_pack(unsigned char *out, unsigned int value) {
    static const char digits[] = "0123456789abcdef";
    *out++ = (unsigned char)digits[(value >> 4) & 15];
    *out++ = (unsigned char)digits[value & 15];
    return out;
}
struct nls_table *fat_cp437_table(void);
struct timespec64 { time64_t tv_sec; long tv_nsec; };
struct fat_mount_options {
    int isvfat, nocase, utf8, unicode_xlate, tz_set, time_offset;
    unsigned short shortname;
};
struct msdos_sb_info {
    struct fat_mount_options options;
    struct nls_table *nls_disk, *nls_io;
};
struct super_block {
    uint64_t size;
    struct msdos_sb_info *s_fs_info;
    struct super_block *s_bdev;
};
#define MSDOS_SB(sb) ((sb)->s_fs_info)
#define bdev_nr_sectors(dev) ((dev)->size / 512)
/* Archive dates remain local wall-clock values until the host time adapter. */
static const struct { int tz_minuteswest; } sys_tz = {0};
#include "../../include/uapi/linux/msdos_fs.h"
static inline int fat_valid_media(u8 media)
{
    return 0xf8 <= media || media == 0xf0;
}
_Static_assert(sizeof(struct msdos_dir_entry) == 32, "FAT directory layout");
_Static_assert(offsetof(struct fat_boot_sector, fat32.root_cluster) == 44, "FAT32 BPB layout");
#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Userspace scalar and byte-buffer services for the Linux Btrfs read path. */
#ifndef SEVENZIP_BTRFS_COMPAT_H
#define SEVENZIP_BTRFS_COMPAT_H
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8, __u8;
typedef uint16_t u16, __u16, __le16;
typedef uint32_t u32, __u32, __le32;
typedef uint64_t u64, __u64, __le64;
typedef int64_t s64, __s64;
typedef int32_t s32, __s32;
#ifdef _MSC_VER
#define BTRFS_PACKED
#else
#define BTRFS_PACKED __attribute__((packed))
#endif

/* Public ABI constants from include/uapi/linux/btrfs.h. */
#define BTRFS_LABEL_SIZE 256
#define BTRFS_FSID_SIZE 16
#define BTRFS_UUID_SIZE 16

#define BTRFS_FEATURE_COMPAT_RO_FREE_SPACE_TREE (1ULL << 0)
#define BTRFS_FEATURE_COMPAT_RO_FREE_SPACE_TREE_VALID (1ULL << 1)
#define BTRFS_FEATURE_COMPAT_RO_VERITY (1ULL << 2)
#define BTRFS_FEATURE_COMPAT_RO_BLOCK_GROUP_TREE (1ULL << 3)
#define BTRFS_FEATURE_INCOMPAT_MIXED_BACKREF (1ULL << 0)
#define BTRFS_FEATURE_INCOMPAT_DEFAULT_SUBVOL (1ULL << 1)
#define BTRFS_FEATURE_INCOMPAT_MIXED_GROUPS (1ULL << 2)
#define BTRFS_FEATURE_INCOMPAT_COMPRESS_LZO (1ULL << 3)
#define BTRFS_FEATURE_INCOMPAT_COMPRESS_ZSTD (1ULL << 4)
#define BTRFS_FEATURE_INCOMPAT_BIG_METADATA (1ULL << 5)
#define BTRFS_FEATURE_INCOMPAT_EXTENDED_IREF (1ULL << 6)
#define BTRFS_FEATURE_INCOMPAT_RAID56 (1ULL << 7)
#define BTRFS_FEATURE_INCOMPAT_SKINNY_METADATA (1ULL << 8)
#define BTRFS_FEATURE_INCOMPAT_NO_HOLES (1ULL << 9)
#define BTRFS_FEATURE_INCOMPAT_METADATA_UUID (1ULL << 10)
#define BTRFS_FEATURE_INCOMPAT_RAID1C34 (1ULL << 11)
#define BTRFS_FEATURE_INCOMPAT_ZONED (1ULL << 12)
#define BTRFS_FEATURE_INCOMPAT_EXTENT_TREE_V2 (1ULL << 13)
#define BTRFS_FEATURE_INCOMPAT_RAID_STRIPE_TREE (1ULL << 14)
#define BTRFS_FEATURE_INCOMPAT_SIMPLE_QUOTA (1ULL << 16)
#define BTRFS_FEATURE_INCOMPAT_REMAP_TREE (1ULL << 17)

enum btrfs_dev_stat_values {
    /* disk I/O failure stats */
    BTRFS_DEV_STAT_WRITE_ERRS, /* EIO or EREMOTEIO from lower layers */
    BTRFS_DEV_STAT_READ_ERRS,  /* EIO or EREMOTEIO from lower layers */
    BTRFS_DEV_STAT_FLUSH_ERRS, /* EIO or EREMOTEIO from lower layers */

    /* stats for indirect indications for I/O failures */
    BTRFS_DEV_STAT_CORRUPTION_ERRS, /* checksum error, bytenr error or
                                     * contents is illegal: this is an
                                     * indication that the block was damaged
                                     * during read or write, or written to
                                     * wrong location or read from wrong
                                     * location */
    BTRFS_DEV_STAT_GENERATION_ERRS, /* an indication that blocks have not
                                     * been written */

    BTRFS_DEV_STAT_VALUES_MAX
};

#if defined(_MSC_VER) || defined(__cplusplus)
#define __DECLARE_FLEX_ARRAY(T, member) T member[0]
#else
#define __DECLARE_FLEX_ARRAY(TYPE, NAME)                                                           \
    struct {                                                                                       \
        struct {                                                                                   \
        } __empty_##NAME;                                                                          \
        TYPE NAME[];                                                                               \
    }
#endif

#ifndef EUCLEAN
#define EUCLEAN 990
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP 991
#endif
#define min_t(t, a, b) ((t)(a) < (t)(b) ? (t)(a) : (t)(b))
#define unlikely(x) (x)
#define IS_ALIGNED(x, a) (((u64)(x) & ((u64)(a) - 1)) == 0)
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

#define S_IFMT 00170000
#define S_IFSOCK 0140000
#define S_IFLNK 0120000
#define S_IFREG 0100000
#define S_IFBLK 0060000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFIFO 0010000
#define S_ISUID 0004000
#define S_ISGID 0002000
#define S_ISVTX 0001000

#define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#define check_add_overflow(a, b, p)                                                                \
    ((u64)(b) > UINT64_MAX - (u64)(a) || ((*(p) = (a) + (b)), false))
static inline bool has_single_bit_set(u64 n) { return n && !(n & (n - 1)); }
static inline bool is_power_of_2(u64 n) { return has_single_bit_set(n); }
#define ERR_PTR(e) ((void *)(intptr_t)(e))
#define PTR_ERR(p) ((int)(intptr_t)(p))
#define IS_ERR(p) ((uintptr_t)(p) >= (uintptr_t)-4095)

static inline u16 get_unaligned_le16(const void *p) {
    const u8 *b = p;
    return (u16)(b[0] | ((u16)b[1] << 8));
}
static inline u32 get_unaligned_le32(const void *p) {
    const u8 *b = p;
    return (u32)b[0] | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24);
}
static inline u64 get_unaligned_le64(const void *p) {
    const u8 *b = p;
    return get_unaligned_le32(b) | ((u64)get_unaligned_le32(b + 4) << 32);
}
static inline u8 get_unaligned_le8(const void *p) { return *(const u8 *)p; }
#endif

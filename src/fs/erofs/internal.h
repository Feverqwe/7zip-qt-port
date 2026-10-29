/* SPDX-License-Identifier: GPL-2.0-only */
/* Userspace services for the Linux EROFS read-side modules. */
#ifndef SEVENZIP_EROFS_INTERNAL_H
#define SEVENZIP_EROFS_INTERNAL_H
#include "../../include/linux/kdev_t.h"
#include "../../include/linux/string.h"
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16, __le16;
typedef uint32_t u32, __u32, __le32;
typedef uint64_t u64, __le64;
typedef int64_t s64;
typedef int64_t loff_t;
typedef uint8_t __u8;
typedef unsigned int uint;
typedef u64 erofs_nid_t, erofs_off_t, erofs_blk_t;
#ifdef _MSC_VER
#define __packed
#pragma pack(push, 1)
#else
#define __packed __attribute__((packed))
#endif
#define BIT(n) (1U << (n))
#define BIT_ULL(n) (UINT64_C(1) << (n))
#define GENMASK_ULL(h, l) ((UINT64_MAX >> (63 - (h))) & (UINT64_MAX << (l)))
#define GENMASK(h, l) ((UINT32_MAX >> (31 - (h))) & (UINT32_MAX << (l)))
#define ALIGN(x, a) (((u64)(x) + (a) - 1) & ~((u64)(a) - 1))
#define round_up(x, a) ALIGN(x, a)
#define roundup(x, a) ALIGN(x, a)
#define round_down(x, a) ((u64)(x) & ~((u64)(a) - 1))
#define rounddown(x, a) round_down(x, a)
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#define min_t(t, a, b) min((t)(a), (t)(b))
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define for_each_set_bit(bit, mask, count)                                                         \
    for ((bit) = 0; (bit) < (count); ++(bit))                                                      \
        if ((*(mask) >> (bit)) & 1UL)
#define container_of(p, type, member) ((type *)((char *)(p) - offsetof(type, member)))
#define check_add_overflow(a, b, p)                                                                \
    ((u64)(b) > UINT64_MAX - (u64)(a) || ((*(p) = (a) + (b)), false))
#define unlikely(x) (x)
#define IS_ERR(p) ((uintptr_t)(p) >= (uintptr_t)-4095)
#define PTR_ERR(p) ((int)(intptr_t)(p))
#define ERR_PTR(e) ((void *)(intptr_t)(e))
#define DBG_BUGON(x) ((void)(x))
#define WARN_ON(x) (!!(x))
#define BUILD_BUG_ON(x) ((void)sizeof(char[1 - 2 * !!(x)]))
#ifdef _MSC_VER
#define __builtin_constant_p(x) 0
#endif
#define PAGE_SHIFT 16
#define PAGE_SIZE (1U << PAGE_SHIFT)
#define PAGE_MASK (~((u64)PAGE_SIZE - 1))
#define U16_MAX UINT16_MAX
#ifndef EFSCORRUPTED
#define EFSCORRUPTED 990
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP 991
#endif
#define ERESTARTSYS 992
#define GFP_KERNEL 0
#define kmalloc_objs(type, count) calloc((count) ? (count) : 1, sizeof(type))
#define kzalloc_objs(type, count) calloc((count) ? (count) : 1, sizeof(type))
#define krealloc(ptr, size, flags) realloc(ptr, size)
#define kmalloc(n, flags) malloc(n)
#define kfree(p) free(p)
#define smp_mb() ((void)0)
#define TASK_KILLABLE 0
#define wait_on_bit_lock(p, bit, flags) 0
#define test_bit(bit, p) (!!(*(p) & (1UL << (bit))))
#define set_bit(bit, p) (*(p) |= (1UL << (bit)))
#define clear_and_wake_up_bit(bit, p) (*(p) &= ~(1UL << (bit)))
#define BITS_PER_LONG (sizeof(unsigned long) * CHAR_BIT)
#define EROFS_I_Z_INITED_BIT 1
#define EROFS_I_EA_INITED_BIT 0
#define EROFS_I_BL_XATTR_BIT (BITS_PER_LONG - 1)
#define EROFS_I_BL_Z_BIT (BITS_PER_LONG - 2)
#define CONFIG_EROFS_FS_ZIP 1
#define CONFIG_EROFS_FS_XATTR 1
#define CONFIG_EROFS_FS_SECURITY 1
#define CONFIG_EROFS_FS_POSIX_ACL 1
#define IS_ENABLED(x) (x)
#define XATTR_USER 1
#define DAX_ALWAYS 0
#define test_opt(opt, option) ((void)(opt), (option) == XATTR_USER)
#define S_DAX 0
#define S_IFMT 0170000
#define S_IFREG 0100000
#define S_IFDIR 0040000
#define S_IFLNK 0120000
#define S_IFCHR 0020000
#define S_IFBLK 0060000
#define S_IFIFO 0010000
#define S_IFSOCK 0140000
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#define S_ISLNK(mode) (((mode) & S_IFMT) == S_IFLNK)
#include "../../include/linux/fs_dirent.h"
#define fallthrough ((void)0)
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define le16_to_cpu(x) __builtin_bswap16(x)
#define le32_to_cpu(x) __builtin_bswap32(x)
#define le64_to_cpu(x) __builtin_bswap64(x)
#define cpu_to_le64(x) __builtin_bswap64(x)
#else
#define le16_to_cpu(x) (x)
#define le32_to_cpu(x) (x)
#define le64_to_cpu(x) (x)
#define cpu_to_le64(x) (x)
#endif
static inline u32 get_unaligned_le32(const void *p) {
    const u8 *b = p;
    return b[0] | (u32)b[1] << 8 | (u32)b[2] << 16 | (u32)b[3] << 24;
}
static inline unsigned int ilog2(u64 n) {
    unsigned int bits = 0;
    while (n >>= 1)
        ++bits;
    return bits;
}
#include "erofs_fs.h"
#ifdef _MSC_VER
#pragma pack(pop)
#endif
#define EROFS_SUPER_MAGIC_V1 0xE0F5E1E2

struct super_block;
struct inode {
    struct super_block *i_sb;
    struct inode *i_mapping;
    s64 i_size, mtime;
    u64 i_blocks;
    u32 i_mode, uid, gid, nlink, nsec, i_rdev, i_flags;
    unsigned int i_blkbits;
    char *i_link;
};
struct erofs_inode {
    erofs_nid_t nid;
    unsigned long flags;
    unsigned char datalayout, inode_isize;
    bool dot_omitted;
    unsigned int xattr_isize;
    unsigned int xattr_name_filter, xattr_shared_count;
    unsigned int *xattr_shared_xattrs;
    union {
        erofs_blk_t startblk;
        struct {
            unsigned short chunkformat;
            unsigned char chunkbits;
        };
        struct {
            unsigned short z_advise;
            unsigned char z_algofmt[2], z_lclusterbits;
            union {
                u64 z_tailextent_headlcn, z_extents;
            };
            erofs_off_t z_fragmentoff;
            unsigned short z_idata_size;
        };
    };
    struct inode vfs_inode;
};
struct erofs_device_info {
    u64 blocks, uniaddr;
    unsigned int id;
    u8 tag[64];
};
struct erofs_map_dev {
    struct super_block *m_sb;
    struct erofs_device_info *m_dif;
    erofs_off_t m_pa;
    unsigned int m_deviceid;
};
struct erofs_xattr_prefix_item {
    struct erofs_xattr_long_prefix *prefix;
    u8 infix_len;
};
struct erofs_sb_info {
    struct erofs_device_info dif0, *devices;
    unsigned int extra_devices;
    bool flatdev;
    struct inode *packed_inode, *metabox_inode;
    u64 total_blocks;
    u32 meta_blkaddr, xattr_blkaddr;
    u32 xattr_prefix_start;
    unsigned int xattr_prefix_count, xattr_filter_reserved, ishare_xattr_prefix_id;
    struct erofs_xattr_prefix_item *xattr_prefixes;
    u16 device_id_mask;
    unsigned char islotbits, blkszbits;
    u32 sb_size, fixed_nsec;
    s64 epoch;
    erofs_nid_t root_nid, packed_nid, metabox_nid;
    u64 inos;
    char *volume_name;
    u32 feature_compat, feature_incompat;
    u16 available_compr_algs;
    struct {
        u16 max_pclusterblks;
    } lz4;
    unsigned int lzma_dict_size;
    unsigned int zstd_dict_size;
    unsigned int opt;
};
struct super_block {
    struct erofs_sb_info *s_fs_info;
    unsigned int s_blocksize, s_blocksize_bits;
    void *context;
    u64 image_size;
    int (*read)(void *, unsigned int, u64, void *, size_t);
    int (*poll)(void *);
    int (*read_inode)(struct inode *, u64, void *, size_t);
    int (*decode)(void *, unsigned int, unsigned int, const void *, size_t, void *, size_t, int);
    int (*open_device)(void *, unsigned int, const char *, u64);
    unsigned int read_depth;
    u8 s_uuid[16];
    u8 s_uuid_len;
};
#include "../../include/linux/fs.h"
// Owned single-page folios replace the kernel page cache. Decoder pages borrow
// their data; only metadata folios are released through folio_put.
struct page {
    u8 *data;
};
struct folio {
    struct page page;
    u64 index;
    u8 data[PAGE_SIZE];
};
typedef u64 pgoff_t;
static inline struct folio *page_folio(struct page *page) {
    return container_of(page, struct folio, page);
}
static inline bool folio_contains(struct folio *folio, pgoff_t index) {
    return folio->index == index;
}
static inline struct page *folio_file_page(struct folio *folio, pgoff_t index) {
    return folio_contains(folio, index) ? &folio->page : NULL;
}
static inline void folio_put(struct folio *folio) { free(folio); }
static inline void *kmap_local_page(struct page *page) { return page->data; }
#define kunmap_local(p) ((void)(p))
struct erofs_buf {
    u8 *base;
    struct page *page;
    u64 off;
    struct super_block *sb;
    struct inode *mapping;
};
struct qstr {
    const char *name;
    unsigned int len;
};
#define QSTR(text) ((struct qstr){(text), (unsigned int)strlen(text)})
#define __EROFS_BUF_INITIALIZER ((struct erofs_buf){0})
#define EROFS_SB(sb) ((sb)->s_fs_info)
#define EROFS_I_SB(inode) EROFS_SB((inode)->i_sb)
#define EROFS_I(ptr) container_of(ptr, struct erofs_inode, vfs_inode)
#define i_blocksize(inode) ((inode)->i_sb->s_blocksize)
#define erofs_blknr(sb, pos) ((erofs_blk_t)((pos) >> (sb)->s_blocksize_bits))
#define erofs_blkoff(sb, pos) ((pos) & ((sb)->s_blocksize - 1))
#define erofs_pos(sb, blk) ((erofs_off_t)(blk) << (sb)->s_blocksize_bits)
#define erofs_iblks(i) (round_up((i)->i_size, i_blocksize(i)) >> (i)->i_blkbits)
#define EROFS_FEATURE_FUNCS(name, kind, feature)                                                   \
    static inline bool erofs_sb_has_##name(struct erofs_sb_info *sbi) {                            \
        return !!(sbi->feature_##kind & EROFS_FEATURE_##feature);                                  \
    }
EROFS_FEATURE_FUNCS(lz4_0padding, incompat, INCOMPAT_LZ4_0PADDING)
EROFS_FEATURE_FUNCS(compr_cfgs, incompat, INCOMPAT_COMPR_CFGS)
EROFS_FEATURE_FUNCS(big_pcluster, incompat, INCOMPAT_BIG_PCLUSTER)
EROFS_FEATURE_FUNCS(device_table, incompat, INCOMPAT_DEVICE_TABLE)
EROFS_FEATURE_FUNCS(fragments, incompat, INCOMPAT_FRAGMENTS)
EROFS_FEATURE_FUNCS(48bit, incompat, INCOMPAT_48BIT)
EROFS_FEATURE_FUNCS(metabox, incompat, INCOMPAT_METABOX)
EROFS_FEATURE_FUNCS(sb_chksum, compat, COMPAT_SB_CHKSUM)
EROFS_FEATURE_FUNCS(xattr_filter, compat, COMPAT_XATTR_FILTER)
EROFS_FEATURE_FUNCS(shared_ea_in_metabox, compat, COMPAT_SHARED_EA_IN_METABOX)
EROFS_FEATURE_FUNCS(plain_xattr_pfx, compat, COMPAT_PLAIN_XATTR_PFX)
EROFS_FEATURE_FUNCS(ishare_xattrs, compat, COMPAT_ISHARE_XATTRS)

static inline bool erofs_inode_in_metabox(struct inode *inode) {
    return !!(EROFS_I(inode)->nid & BIT_ULL(EROFS_DIRENT_NID_METABOX_BIT));
}
static inline erofs_off_t erofs_iloc(struct inode *inode) {
    struct erofs_sb_info *sbi = EROFS_I_SB(inode);
    erofs_nid_t nid_lo = EROFS_I(inode)->nid & EROFS_DIRENT_NID_MASK;
    if (erofs_inode_in_metabox(inode))
        return nid_lo << sbi->islotbits;
    return erofs_pos(inode->i_sb, sbi->meta_blkaddr) + (nid_lo << sbi->islotbits);
}
static inline unsigned int erofs_inode_version(unsigned int f) { return f & 1; }
static inline unsigned int erofs_inode_datalayout(unsigned int f) { return (f >> 1) & 7; }
static inline void i_uid_write(struct inode *inode, u32 uid) { inode->uid = uid; }
static inline void i_gid_write(struct inode *inode, u32 gid) { inode->gid = gid; }
static inline void set_nlink(struct inode *inode, u32 n) { inode->nlink = n; }
static inline void inode_set_mtime(struct inode *inode, s64 sec, u32 nsec) {
    inode->mtime = sec;
    inode->nsec = nsec;
}
#define inode_set_atime_to_ts(...) ((void)0)
#define inode_set_ctime_to_ts(...) ((void)0)
#define inode_get_mtime(...) 0
#define cache_no_acl(...) ((void)0)
#define erofs_inode_has_noacl(...) 0
#define inode_set_cached_link(inode, link, size) ((inode)->i_link = (link))
#define in_range32(v, start, len) ((u32)((v) - (start)) < (u32)(len))
#define erofs_nid_to_ino64(sbi, nid) (nid)
struct dir_context {
    u64 pos;
    void *context;
    bool (*emit)(void *, const char *, unsigned int, u64, unsigned char);
};
struct file {
    struct inode *inode;
};
#define file_inode(file) ((file)->inode)
#define dir_emit(ctx, name, len, nid, type) ((ctx)->emit((ctx)->context, name, len, nid, type))
static inline bool dir_emit_dot(struct file *file, struct dir_context *ctx) {
    struct inode *inode = file_inode(file);
    return dir_emit(ctx, ".", 1, erofs_nid_to_ino64(EROFS_I_SB(inode), EROFS_I(inode)->nid),
                    DT_DIR);
}
#define EROFS_MAP_MAPPED 0x0001
#define EROFS_MAP_META 0x0002
#define EROFS_MAP_PARTIAL_MAPPED 0x0004
#define EROFS_MAP_PARTIAL_REF 0x0008
#define EROFS_MAP_FRAGMENT 0x0010
#define EROFS_MAP_FULL(f) (!((f) & (EROFS_MAP_PARTIAL_MAPPED | EROFS_MAP_PARTIAL_REF)))
struct erofs_map_blocks {
    struct erofs_buf buf;
    erofs_off_t m_pa, m_la;
    u64 m_plen, m_llen;
    unsigned short m_deviceid;
    char m_algorithmformat;
    unsigned int m_flags;
};
#define EROFS_GET_BLOCKS_FIEMAP 1
#define EROFS_GET_BLOCKS_READMORE 2
#define EROFS_GET_BLOCKS_FINDTAIL 4
enum {
    Z_EROFS_COMPRESSION_SHIFTED = Z_EROFS_COMPRESSION_MAX,
    Z_EROFS_COMPRESSION_INTERLACED,
    Z_EROFS_COMPRESSION_RUNTIME_MAX
};
static inline void erofs_printk(struct super_block *sb, const char *fmt, ...) {
    (void)sb;
    (void)fmt;
}
#define erofs_err erofs_printk
#define erofs_info erofs_printk
#define trace_erofs_map_blocks_enter(inode, map, flags) ((void)(inode), (void)(map), (void)(flags))
#define trace_erofs_map_blocks_exit(inode, map, flags, err) ((void)(err))

char *kmemdup_nul(const char *, size_t, int);
char *kstrndup(const char *, size_t, int);
u32 crc32c(u32, const void *, size_t);
void erofs_put_metabuf(struct erofs_buf *);
void erofs_unmap_metabuf(struct erofs_buf *);
int erofs_init_metabuf(struct erofs_buf *, struct super_block *, bool);
void *erofs_bread(struct erofs_buf *, erofs_off_t, bool);
struct folio *erofs_read_cache_folio(struct erofs_buf *, pgoff_t);
void *erofs_read_metabuf(struct erofs_buf *, struct super_block *, erofs_off_t, bool);
void *erofs_read_metadata(struct super_block *, struct erofs_buf *, erofs_off_t *, int *);
int erofs_map_blocks(struct inode *, struct erofs_map_blocks *);
int erofs_map_dev(struct super_block *, struct erofs_map_dev *);
int z_erofs_map_blocks_iter(struct inode *, struct erofs_map_blocks *, int);
int erofs_read_inode(struct inode *);
int erofs_read_superblock(struct super_block *);
int erofs_scan_devices(struct super_block *, struct erofs_super_block *);
int z_erofs_parse_cfgs(struct super_block *, struct erofs_super_block *);
int z_erofs_load_lzma_config(struct super_block *, struct erofs_super_block *, void *, int);
int z_erofs_load_deflate_config(struct super_block *, struct erofs_super_block *, void *, int);
int z_erofs_load_zstd_config(struct super_block *, struct erofs_super_block *, void *, int);
int erofs_decode_extent(struct inode *, struct erofs_map_blocks *, const void *, void *);
int erofs_fill_dentries(struct inode *, struct dir_context *, void *, struct erofs_dirent *,
                        unsigned int, unsigned int);
int erofs_readdir(struct file *, struct dir_context *);
#endif

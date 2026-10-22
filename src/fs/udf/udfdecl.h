// SPDX-License-Identifier: GPL-2.0-only
/* Stream and owned-buffer services for the Linux allocation reader. */
#ifndef SEVENZIP_UDF_DECL_H
#define SEVENZIP_UDF_DECL_H
#include "../../udfreader.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../nls/types.h"
#include "../../include/linux/crc-itu-t.h"
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
#include "../../../third_party/linux/fs/udf/ecma_167.h"
#include "../../../third_party/linux/fs/udf/osta_udf.h"
#ifdef _MSC_VER
#pragma pack(pop)
#endif
#ifndef EFSCORRUPTED
#define EFSCORRUPTED EIO
#endif
#ifndef ESTALE
#define ESTALE EIO
#endif
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define le16_to_cpu(v) __builtin_bswap16(v)
#define le32_to_cpu(v) __builtin_bswap32(v)
#define le64_to_cpu(v) __builtin_bswap64(v)
#define cpu_to_le16(v) __builtin_bswap16(v)
#define cpu_to_le32(v) __builtin_bswap32(v)
#else
#define le16_to_cpu(v) (v)
#define le32_to_cpu(v) (v)
#define le64_to_cpu(v) (v)
#define cpu_to_le16(v) (v)
#define cpu_to_le32(v) (v)
#endif
#include "udfend.h"
#define kfree(p) free(p)
#define kzalloc_objs(object, count) calloc((count), sizeof(object))
#define noinline
#define udf_err(...) ((void)0)
#define udf_warn(...) ((void)0)
#define udf_debug(...) ((void)0)
#define pr_err(...) ((void)0)
#define pr_notice(...) ((void)0)
#define pr_warn(...) ((void)0)
#define WARN_ON_ONCE(value) ((void)(value))
#define min_t(type, a, b) ((type)(a) < (type)(b) ? (type)(a) : (type)(b))
#define hex_asc_upper_hi(value) ("0123456789ABCDEF"[((value) >> 4) & 15])
#define hex_asc_upper_lo(value) ("0123456789ABCDEF"[(value) & 15])
#define fallthrough ((void)0)
#define UDF_EXTENT_LENGTH_MASK 0x3fffffff
#define UDF_NAME_LEN 254
#define UDF_NAME_LEN_CS0 255
#define UDF_NAME_PAD 4
#define __GFP_NOFAIL 0
#define ALIGN(value, size) (((value) + (size) - 1) & ~((size) - 1))
#define DIV_ROUND_UP(value, size) (((value) + (size) - 1) / (size))
#define buffer_uptodate(bh) (1)
typedef uint64_t udf_pblk_t;
struct buffer_head { uint8_t *b_data; unsigned b_size, references; };
typedef uint32_t u32;
typedef uint64_t sector_t;
typedef uint16_t umode_t;
typedef uint16_t u16;
struct timespec64 { int64_t tv_sec; long tv_nsec; };
#define UDF_INVALID_ID UINT32_MAX
/* Linux mode/dev_t representation, independent of the host C runtime. */
#define S_IFDIR 0040000
#define S_IFREG 0100000
#define S_IFBLK 0060000
#define S_IFCHR 0020000
#define S_IFIFO 0010000
#define S_IFSOCK 0140000
#define S_IFLNK 0120000
#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000
#define S_ISCHR(mode) (((mode) & 0170000) == S_IFCHR)
#define S_ISBLK(mode) (((mode) & 0170000) == S_IFBLK)
#include "../../include/linux/kdev_t.h"
#define kmalloc(size, flags) malloc(size)
#define GFP_KERNEL 0
#define read_lock(lock) ((void)0)
#define read_unlock(lock) ((void)0)
struct super_block;
struct inode;
#include "udf_sb.h"
#define PTR_ERR(v) ((int)(intptr_t)(v))
#define sb_rdonly(sb) (1)
#define ERR_PTR(v) ((void *)(intptr_t)(v))
#define IS_ERR(v) ((uintptr_t)(v) >= (uintptr_t)-4095)
#define is_power_of_2(n) ((n) != 0 && (((n) & ((n) - 1)) == 0))
#define struct_size(p, member, n) (sizeof(*(p)) + sizeof((p)->member[0]) * (n))
struct super_block {
    struct udf_sb_info info;
    uint32_t s_blocksize;
    unsigned char s_blocksize_bits;
    void *context;
    sevenzip_udf_read read;
    sevenzip_udf_iget iget;
    sevenzip_udf_inode_block inode_block;
    uint64_t blocks;
    sevenzip_udf_open_file open_file;
    sevenzip_udf_read_file read_file;
    sevenzip_udf_close_file close_file;
    int io_error;
    uint64_t device_size;
    sevenzip_udf_descriptor descriptor;
    sevenzip_udf_reset reset;
    void *descriptor_output;
    unsigned seen_anchor;
    uint32_t anchor_block;
};
struct udf_options { uint32_t blocksize, lastblock, anchor; };
struct udf_vds_record { uint32_t block, volDescSeqNum; };
struct generic_desc { struct tag descTag; __le32 volDescSeqNum; };
#include "udf_i.h"
/* Constant-space cycle guard for one allocation traversal. */
struct sevenzip_udf_walk {
    struct kernel_lb_addr block;
    uint32_t offset;
    uint64_t power, distance;
    bool external;
};
struct udf_fileident_iter {
    struct inode *dir;
    int64_t pos;
    struct buffer_head *bh[2];
    struct kernel_lb_addr eloc;
    uint32_t elen;
    sector_t loffset;
    struct extent_position epos;
    struct fileIdentDesc fi;
    uint8_t *name, *namebuf;
    struct sevenzip_udf_walk walk;
};
static inline unsigned int udf_dir_entry_len(struct fileIdentDesc *cfi)
{
    return ALIGN(sizeof(struct fileIdentDesc) +
        le16_to_cpu(cfi->lengthOfImpUse) + cfi->lengthFileIdent, UDF_NAME_PAD);
}
struct udf_inode_info {
    struct kernel_lb_addr i_location;
    uint32_t i_lenEAttr, i_lenAlloc;
    uint64_t i_lenExtents;
    unsigned i_alloc_type, i_efe, i_use, i_strat4096;
    uint8_t *i_data;
    struct udf_ext_cache cached_extent;
    unsigned i_hidden, i_streamdir;
    uint64_t i_unique, i_lenStreams;
    uint32_t i_next_alloc_block, i_next_alloc_goal, i_extraPerms, i_checkpoint;
    struct kernel_lb_addr i_locStreamdir;
    struct timespec64 i_crtime;
};
struct inode {
    struct super_block *i_sb;
    struct udf_inode_info info;
    struct buffer_head *entry;
    uint64_t i_size;
    unsigned i_state, cached;
    void *file;
    uint32_t i_uid, i_gid, i_nlink, i_rdev;
    umode_t i_mode;
    uint64_t i_blocks, i_generation;
    struct timespec64 atime, mtime, ctime;
    unsigned char i_blkbits;
};
static inline void i_uid_write(struct inode *inode, uint32_t id) { inode->i_uid = id; }
static inline void i_gid_write(struct inode *inode, uint32_t id) { inode->i_gid = id; }
static inline void set_nlink(struct inode *inode, unsigned count) { inode->i_nlink = count; }
static inline void inc_nlink(struct inode *inode) { ++inode->i_nlink; }
static inline void init_special_inode(struct inode *inode, umode_t mode, uint32_t dev) {
    inode->i_mode = mode;
    inode->i_rdev = dev;
}
static inline void inode_set_atime_to_ts(struct inode *inode, struct timespec64 ts) { inode->atime = ts; }
static inline void inode_set_mtime_to_ts(struct inode *inode, struct timespec64 ts) { inode->mtime = ts; }
static inline void inode_set_ctime_to_ts(struct inode *inode, struct timespec64 ts) { inode->ctime = ts; }
void udf_disk_stamp_to_time(struct timespec64 *, struct timestamp);
int64_t mktime64(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
static inline uint64_t sb_bdev_nr_blocks(struct super_block *sb) { return sb->blocks; }
struct inode *udf_iget_special(struct super_block *, const struct kernel_lb_addr *);
struct buffer_head *udf_bread(struct inode *, uint32_t, int, int *);
void iput(struct inode *);
#define UDF_I(inode) (&(inode)->info)
#define UDF_SB(sb) (&(sb)->info)
#define UDF_QUERY_FLAG(sb, flag) ((UDF_SB(sb)->s_flags & (1U << (flag))) != 0)
#define UDF_SET_FLAG(sb, flag) (UDF_SB(sb)->s_flags |= (1U << (flag)))
/* Serialized archive calls replace VFS locking at this boundary. */
#define spin_lock(lock) ((void)0)
#define spin_unlock(lock) ((void)0)
static inline void get_bh(struct buffer_head *bh) { ++bh->references; }
void udf_clear_extent_cache(struct inode *);
static inline size_t udf_file_entry_alloc_offset(struct inode *inode)
{
    struct udf_inode_info *iinfo = UDF_I(inode);
    if (iinfo->i_use)
        return sizeof(struct unallocSpaceEntry);
    else if (iinfo->i_efe)
        return sizeof(struct extendedFileEntry) + iinfo->i_lenEAttr;
    else
        return sizeof(struct fileEntry) + iinfo->i_lenEAttr;
}
static inline int check_add_overflow(size_t a, uint32_t b, int *result)
{
    if (a > INT_MAX || b > INT_MAX - a)
        return 1;
    *result = (int)(a + b);
    return 0;
}
static inline int udf_add_u32_overflow(uint32_t a, uint32_t b, uint32_t *result)
{
    if (b > UINT32_MAX - a)
        return 1;
    *result = a + b;
    return 0;
}
/* This token carries a logical address to the caller's existing partition mapper. */
static inline udf_pblk_t udf_get_lb_pblock(struct super_block *sb,
                                          const struct kernel_lb_addr *loc, unsigned offset)
{
    (void)sb;
    if (offset > UINT32_MAX - loc->logicalBlockNum)
        return UINT64_MAX;
    return ((uint64_t)loc->partitionReferenceNum << 32) | (loc->logicalBlockNum + offset);
}
struct buffer_head *sb_bread(struct super_block *, udf_pblk_t);
void brelse(struct buffer_head *);
struct short_ad *udf_get_fileshortad(uint8_t *, int, uint32_t *, int);
struct long_ad *udf_get_filelongad(uint8_t *, int, uint32_t *, int);
int udf_current_aext(struct inode *, struct extent_position *, struct kernel_lb_addr *, uint32_t *, int8_t *, int);
int udf_next_aext(struct inode *, struct extent_position *, struct kernel_lb_addr *, uint32_t *, int8_t *, int);
int sevenzip_udf_next_aext(struct sevenzip_udf_walk *, struct inode *, struct extent_position *,
                           struct kernel_lb_addr *, uint32_t *, int8_t *);
u8 udf_tag_checksum(const struct tag *);
struct buffer_head *udf_read_tagged(struct super_block *, udf_pblk_t, uint32_t, uint16_t *);
struct buffer_head *udf_read_ptagged(struct super_block *, struct kernel_lb_addr *, uint32_t, uint16_t *);
struct inode *__udf_iget(struct super_block *, struct kernel_lb_addr *, bool);
struct inode *iget_locked(struct super_block *, unsigned long);
void iget_failed(struct inode *);
#define I_NEW 1U
#define UDF_INODE_LOADING 2U
static inline unsigned inode_state_read_once(struct inode *inode) { return inode->i_state; }
static inline void unlock_new_inode(struct inode *inode) { inode->i_state &= ~(I_NEW | UDF_INODE_LOADING); }
static inline unsigned long udf_inode_block(struct super_block *sb,
                                            const struct kernel_lb_addr *location)
{
    return sb->inode_block(sb->context, location->partitionReferenceNum,
                           location->logicalBlockNum);
}
struct genericFormat *udf_get_extendedattr(struct inode *, uint32_t, uint8_t);
uint32_t udf_get_pblock_spar15(struct super_block *, uint32_t, uint16_t, uint32_t);
int sevenzip_udf_load_sparable(struct super_block *, struct udf_part_map *, struct sparablePartitionMap *);
uint32_t udf_get_pblock(struct super_block *, uint32_t, uint16_t, uint32_t);
uint32_t udf_get_pblock_virt15(struct super_block *, uint32_t, uint16_t, uint32_t);
uint32_t udf_get_pblock_virt20(struct super_block *, uint32_t, uint16_t, uint32_t);
int sevenzip_udf_load_vat(struct super_block *, int, int);
struct inode *udf_find_metadata_inode_efe(struct super_block *, u32, u32);
uint32_t udf_get_pblock_meta25(struct super_block *, uint32_t, uint16_t, uint32_t);
int inode_bmap(struct inode *, sector_t, struct extent_position *, struct kernel_lb_addr *,
               uint32_t *, sector_t *, int8_t *);
int sevenzip_udf_load_metadata(struct super_block *, int, int);
int sb_set_blocksize(struct super_block *, unsigned);
udf_pblk_t udf_get_last_block(struct super_block *);
unsigned int udf_get_last_session(struct super_block *);
int sevenzip_udf_load_vrs(struct super_block *, struct udf_options *, struct kernel_lb_addr *);
int sevenzip_udf_find_fileset(struct super_block *, struct kernel_lb_addr *, struct kernel_lb_addr *);
struct logicalVolIntegrityDescImpUse *udf_sb_lvidiu(struct super_block *);
int sevenzip_udf_check_vsd(struct super_block *);
int udf_get_filename(struct super_block *, const uint8_t *, int, uint8_t *, int);
int udf_fiiter_init(struct udf_fileident_iter *, struct inode *, int64_t);
int udf_fiiter_advance(struct udf_fileident_iter *);
void udf_fiiter_release(struct udf_fileident_iter *);
#endif

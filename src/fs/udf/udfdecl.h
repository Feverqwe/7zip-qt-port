// SPDX-License-Identifier: GPL-2.0-only
/* Stream and owned-buffer services for the Linux allocation reader. */
#ifndef SEVENZIP_UDF_DECL_H
#define SEVENZIP_UDF_DECL_H
#include "../../udfreader.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
#include "../../../third_party/linux/fs/udf/ecma_167.h"
#ifdef _MSC_VER
#pragma pack(pop)
#endif
#ifndef EFSCORRUPTED
#define EFSCORRUPTED EIO
#endif
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define le16_to_cpu(v) __builtin_bswap16(v)
#define le32_to_cpu(v) __builtin_bswap32(v)
#else
#define le16_to_cpu(v) (v)
#define le32_to_cpu(v) (v)
#endif
#include "udfend.h"
#define udf_err(...) ((void)0)
#define udf_debug(...) ((void)0)
#define pr_err(...) ((void)0)
#define UDF_EXTENT_LENGTH_MASK 0x3fffffff
typedef uint64_t udf_pblk_t;
struct buffer_head { uint8_t *b_data; unsigned b_size; };
struct udf_sb_info { uint32_t s_partitions; };
struct super_block {
    struct udf_sb_info info;
    uint32_t s_blocksize;
    void *context;
    sevenzip_udf_read read;
};
struct udf_inode_info {
    struct kernel_lb_addr i_location;
    uint32_t i_lenEAttr, i_lenAlloc;
    unsigned i_alloc_type, i_efe, i_use;
    uint8_t *i_data;
};
struct inode {
    struct super_block *i_sb;
    struct udf_inode_info info;
};
#include "udf_i.h"
#define UDF_I(inode) (&(inode)->info)
#define UDF_SB(sb) (&(sb)->info)
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
/* This token carries a logical address to the caller's existing partition mapper. */
static inline udf_pblk_t udf_get_lb_pblock(struct super_block *sb,
                                          const struct kernel_lb_addr *loc, unsigned offset)
{
    (void)sb;
    return ((uint64_t)loc->partitionReferenceNum << 32) | (loc->logicalBlockNum + offset);
}
struct buffer_head *sb_bread(struct super_block *, udf_pblk_t);
void brelse(struct buffer_head *);
struct short_ad *udf_get_fileshortad(uint8_t *, int, uint32_t *, int);
struct long_ad *udf_get_filelongad(uint8_t *, int, uint32_t *, int);
int udf_current_aext(struct inode *, struct extent_position *, struct kernel_lb_addr *, uint32_t *, int8_t *, int);
int udf_next_aext(struct inode *, struct extent_position *, struct kernel_lb_addr *, uint32_t *, int8_t *, int);
#endif

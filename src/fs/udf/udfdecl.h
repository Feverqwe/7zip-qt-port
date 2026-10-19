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
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define le16_to_cpu(v) __builtin_bswap16(v)
#define le32_to_cpu(v) __builtin_bswap32(v)
#define cpu_to_le16(v) __builtin_bswap16(v)
#else
#define le16_to_cpu(v) (v)
#define le32_to_cpu(v) (v)
#define cpu_to_le16(v) (v)
#endif
#include "udfend.h"
#define udf_err(...) ((void)0)
#define udf_debug(...) ((void)0)
#define pr_err(...) ((void)0)
#define pr_notice(...) ((void)0)
#define UDF_EXTENT_LENGTH_MASK 0x3fffffff
typedef uint64_t udf_pblk_t;
struct buffer_head { uint8_t *b_data; unsigned b_size; };
typedef uint32_t u32;
typedef uint64_t sector_t;
struct super_block;
struct inode;
struct udf_virtual_data { uint32_t s_num_entries, s_start_offset; };
struct udf_sparing_data {
    uint16_t s_packet_len;
    struct buffer_head *s_spar_map[4];
};
struct udf_part_map {
    uint32_t s_partition_len, s_partition_root;
    unsigned s_partition_type;
    union { struct udf_sparing_data s_sparing; struct udf_virtual_data s_virtual; } s_type_specific;
    uint32_t (*s_partition_func)(struct super_block *, uint32_t, uint16_t, uint32_t);
};
#define UDF_SPARABLE_MAP15 0x1522U
#define UDF_VIRTUAL_MAP15 0x1512U
#define UDF_VIRTUAL_MAP20 0x2012U
#define ERR_PTR(v) ((void *)(intptr_t)(v))
#define IS_ERR(v) ((uintptr_t)(v) >= (uintptr_t)-4095)
#define is_power_of_2(n) ((n) != 0 && (((n) & ((n) - 1)) == 0))
#define struct_size(p, member, n) (sizeof(*(p)) + sizeof((p)->member[0]) * (n))
struct udf_sb_info {
    uint32_t s_partitions;
    struct udf_part_map *s_partmaps;
    struct inode *s_vat_inode;
    uint64_t s_last_block;
};
struct super_block {
    struct udf_sb_info info;
    uint32_t s_blocksize;
    void *context;
    sevenzip_udf_read read;
    uint64_t blocks;
    sevenzip_udf_open_file open_file;
    sevenzip_udf_read_file read_file;
    sevenzip_udf_close_file close_file;
    int io_error;
};
struct udf_inode_info {
    struct kernel_lb_addr i_location;
    uint32_t i_lenEAttr, i_lenAlloc;
    unsigned i_alloc_type, i_efe, i_use, i_strat4096;
    uint8_t *i_data;
};
struct inode {
    struct super_block *i_sb;
    struct udf_inode_info info;
    struct buffer_head *entry;
    uint64_t i_size;
    void *file;
};
static inline uint64_t sb_bdev_nr_blocks(struct super_block *sb) { return sb->blocks; }
struct inode *udf_iget_special(struct super_block *, const struct kernel_lb_addr *);
struct buffer_head *udf_bread(struct inode *, uint32_t, int, int *);
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
u8 udf_tag_checksum(const struct tag *);
struct buffer_head *udf_read_tagged(struct super_block *, udf_pblk_t, uint32_t, uint16_t *);
struct buffer_head *udf_read_ptagged(struct super_block *, struct kernel_lb_addr *, uint32_t, uint16_t *);
int sevenzip_udf_read_inode(struct inode *);
uint32_t udf_get_pblock_spar15(struct super_block *, uint32_t, uint16_t, uint32_t);
int sevenzip_udf_load_sparable(struct super_block *, struct udf_part_map *, struct sparablePartitionMap *);
uint32_t udf_get_pblock(struct super_block *, uint32_t, uint16_t, uint32_t);
uint32_t udf_get_pblock_virt15(struct super_block *, uint32_t, uint16_t, uint32_t);
uint32_t udf_get_pblock_virt20(struct super_block *, uint32_t, uint16_t, uint32_t);
int sevenzip_udf_load_vat(struct super_block *, int, int);
#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Read-side projection of Linux fs/udf/udf_sb.h. */
#ifndef _UDF_SB_H
#define _UDF_SB_H
#define MF_DUPLICATE_MD 0x01
#define MF_MIRROR_FE_LOADED 0x02
#define UDF_MAX_READ_VERSION 0x0260
#define UDF_FLAG_NOVRS 8
#define UDF_FLAG_RW_INCOMPAT 19
#define UDF_FLAG_UID_SET 13
#define UDF_FLAG_GID_SET 14
#define UDF_INVALID_MODE ((umode_t) - 1)
#define UDF_PART_FLAG_UNALLOC_BITMAP 0x0001
#define UDF_PART_FLAG_UNALLOC_TABLE 0x0002
#define UDF_PART_FLAG_READ_ONLY 0x0010
#define UDF_PART_FLAG_WRITE_ONCE 0x0020
#define UDF_PART_FLAG_REWRITABLE 0x0040
#define UDF_PART_FLAG_OVERWRITABLE 0x0080

struct udf_meta_data {
    uint32_t s_meta_file_loc;
    uint32_t s_mirror_file_loc;
    uint32_t s_bitmap_file_loc;
    uint32_t s_alloc_unit_size;
    uint16_t s_align_unit_size;
    /* Partition reference of the associated physical / sparable partition. */
    uint16_t s_phys_partition_ref;
    int s_flags;
    struct inode *s_metadata_fe;
    struct inode *s_mirror_fe;
    struct inode *s_bitmap_fe;
};
struct udf_sparing_data {
    uint16_t s_packet_len;
    struct buffer_head *s_spar_map[4];
};
struct udf_virtual_data {
    uint32_t s_num_entries;
    uint16_t s_start_offset;
};
struct udf_bitmap {
    uint32_t s_extPosition;
    int s_nr_groups;
    struct buffer_head *s_block_bitmap[];
};
struct udf_part_map {
    union {
        struct udf_bitmap *s_bitmap;
        struct inode *s_table;
    } s_uspace;
    unsigned s_partition_flags;
    uint16_t s_volumeseqnum, s_partition_num;
    const uint8_t *s_raw;
    unsigned s_raw_len;
    uint32_t s_partition_len, s_partition_root;
    unsigned s_partition_type;
    union {
        struct udf_sparing_data s_sparing;
        struct udf_virtual_data s_virtual;
        struct udf_meta_data s_metadata;
    } s_type_specific;
    uint32_t (*s_partition_func)(struct super_block *, uint32_t, uint16_t, uint32_t);
};
struct udf_sb_info {
    uint32_t s_uid, s_gid;
    umode_t s_umask, s_fmode, s_dmode;
    struct nls_table *s_nls_map;
    uint32_t s_partitions;
    struct udf_part_map *s_partmaps;
    struct inode *s_vat_inode;
    uint64_t s_last_block;
    uint32_t s_anchor, s_session, s_flags;
    uint16_t s_partition, s_serial_number;
    struct buffer_head *s_lvid_bh;
};
#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Read-side projection of Linux fs/udf/udf_sb.h. */
#ifndef _UDF_SB_H
#define _UDF_SB_H
#define UDF_VIRTUAL_MAP15 0x1512U
#define UDF_VIRTUAL_MAP20 0x2012U
#define UDF_SPARABLE_MAP15 0x1522U
#define UDF_METADATA_MAP25 0x2511U
#define MF_DUPLICATE_MD 0x01
#define MF_MIRROR_FE_LOADED 0x02

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
struct udf_part_map {
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
	uint32_t s_partitions;
	struct udf_part_map *s_partmaps;
	struct inode *s_vat_inode;
	uint64_t s_last_block;
};
#endif

// SPDX-License-Identifier: GPL-2.0-only
/* Owned services for the Linux read-only metadata partition mapper. */
#include "formats/udf/udfdecl.h"

struct sevenzip_udf_metadata {
    struct super_block sb;
    uint16_t partition;
    bool mapping;
    sevenzip_udf_status status;
};

void sevenzip_udf_metadata_free(struct sevenzip_udf_metadata *state)
{
    if (state) {
        struct udf_meta_data *data = &state->sb.info.s_partmaps[state->partition].s_type_specific.s_metadata;
        iput(data->s_metadata_fe);
        iput(data->s_mirror_fe);
        iput(data->s_bitmap_fe);
        free(state->sb.info.s_partmaps);
        free(state);
    }
}

int sevenzip_udf_metadata_create(void *context, sevenzip_udf_open_file open_file,
    sevenzip_udf_read read, sevenzip_udf_close_file close_file, sevenzip_udf_status status,
    uint32_t block_size,
    uint32_t partitions, uint16_t partition, uint16_t backing, const void *raw,
    struct sevenzip_udf_metadata **output)
{
    struct sevenzip_udf_metadata *state = calloc(1, sizeof(*state));
    struct metadataPartitionMap map;
    struct udf_meta_data *data;
    int error;
    *output = NULL;
    if (!state)
        return -ENOMEM;
    state->partition = partition;
    state->status = status;
    state->sb.info.s_partitions = partitions;
    state->sb.info.s_partmaps = calloc(partitions, sizeof(struct udf_part_map));
    if (!state->sb.info.s_partmaps) {
        free(state);
        return -ENOMEM;
    }
    state->sb.s_blocksize = block_size;
    while ((1U << state->sb.s_blocksize_bits) < block_size)
        ++state->sb.s_blocksize_bits;
    state->sb.context = context;
    state->sb.open_file = open_file;
    state->sb.read = read;
    state->sb.close_file = close_file;
    memcpy(&map, raw, sizeof(map));
    data = &state->sb.info.s_partmaps[partition].s_type_specific.s_metadata;
    state->sb.info.s_partmaps[partition].s_partition_type = UDF_METADATA_MAP25;
    state->sb.info.s_partmaps[partition].s_partition_func = udf_get_pblock_meta25;
    data->s_meta_file_loc = le32_to_cpu(map.metadataFileLoc);
    data->s_mirror_file_loc = le32_to_cpu(map.metadataMirrorFileLoc);
    data->s_bitmap_file_loc = le32_to_cpu(map.metadataBitmapFileLoc);
    data->s_alloc_unit_size = le32_to_cpu(map.allocUnitSize);
    data->s_align_unit_size = le16_to_cpu(map.alignUnitSize);
    if (map.flags & 0x01)
        data->s_flags |= MF_DUPLICATE_MD;
    error = sevenzip_udf_load_metadata(&state->sb, partition, backing);
    if (error) {
        if (state->sb.io_error < -4095)
            error = state->sb.io_error;
        sevenzip_udf_metadata_free(state);
        return error;
    }
    *output = state;
    return 0;
}

int sevenzip_udf_metadata_map(struct sevenzip_udf_metadata *state, uint32_t block,
                            uint32_t offset, uint16_t *backing, uint32_t *mapped)
{
    if (state->mapping)
        return 1;
    state->mapping = true;
    state->sb.io_error = 0;
    *mapped = udf_get_pblock(&state->sb, block, state->partition, offset);
    state->mapping = false;
    if (state->status(state->sb.context))
        return state->status(state->sb.context);
    if (*mapped == UINT32_MAX)
        return state->sb.io_error ? state->sb.io_error : 1;
    *backing = state->sb.info.s_partmaps[state->partition].s_type_specific.s_metadata.s_phys_partition_ref;
    return 0;
}

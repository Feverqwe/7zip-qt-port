// SPDX-License-Identifier: GPL-2.0-only
/* Owned stream services for the Linux VAT loader and block mapper. */
#include "udfdecl.h"

struct sevenzip_udf_virtual {
    struct super_block sb;
    uint16_t partition;
};

void sevenzip_udf_virtual_free(struct sevenzip_udf_virtual *state)
{
    if (state) {
        struct inode *inode = state->sb.info.s_vat_inode;
        if (inode) {
            state->sb.close_file(inode->file);
            free(inode);
        }
        free(state->sb.info.s_partmaps);
        free(state);
    }
}

int sevenzip_udf_virtual_create(void *context, sevenzip_udf_open_file open_file,
    sevenzip_udf_read_file read_file, sevenzip_udf_close_file close_file,
    uint32_t block_size, uint32_t partitions, uint64_t blocks, uint32_t last, uint32_t root,
    uint16_t partition, uint16_t backing, int version20, struct sevenzip_udf_virtual **out)
{
    struct sevenzip_udf_virtual *state = calloc(1, sizeof(*state));
    struct udf_part_map *map;
    int error;
    *out = NULL;
    if (!state)
        return -ENOMEM;
    state->partition = partition;
    state->sb.info.s_partitions = partitions;
    state->sb.info.s_partmaps = calloc(state->sb.info.s_partitions, sizeof(*map));
    if (!state->sb.info.s_partmaps) {
        sevenzip_udf_virtual_free(state);
        return -ENOMEM;
    }
    state->sb.s_blocksize = block_size;
    state->sb.blocks = blocks;
    state->sb.info.s_last_block = last;
    state->sb.context = context;
    state->sb.open_file = open_file;
    state->sb.read_file = read_file;
    state->sb.close_file = close_file;
    map = &state->sb.info.s_partmaps[partition];
    map->s_partition_root = root;
    map->s_partition_type = version20 ? UDF_VIRTUAL_MAP20 : UDF_VIRTUAL_MAP15;
    map->s_partition_func = version20 ? udf_get_pblock_virt20 : udf_get_pblock_virt15;
    error = sevenzip_udf_load_vat(&state->sb, partition, backing);
    if (error) {
        if (state->sb.io_error < -4095)
            error = state->sb.io_error;
        sevenzip_udf_virtual_free(state);
        return error;
    }
    state->sb.context = NULL;
    state->sb.open_file = NULL;
    *out = state;
    return 0;
}

int sevenzip_udf_virtual_map(struct sevenzip_udf_virtual *state, uint32_t block,
                            uint32_t offset, uint16_t *backing, uint32_t *mapped)
{
    state->sb.io_error = 0;
    *mapped = udf_get_pblock(&state->sb, block, state->partition, offset);
    if (*mapped == UINT32_MAX)
        return state->sb.io_error ? state->sb.io_error : 1;
    *backing = state->sb.info.s_vat_inode->info.i_location.partitionReferenceNum;
    return 0;
}

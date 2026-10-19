// SPDX-License-Identifier: GPL-2.0-only
#include "udfdecl.h"

struct buffer_head *sb_bread(struct super_block *sb, udf_pblk_t address)
{
    struct buffer_head *bh = malloc(sizeof(*bh));
    if (!bh)
        return NULL;
    bh->b_size = sb->s_blocksize;
    bh->b_data = malloc(bh->b_size);
    if (!bh->b_data || sb->read(sb->context, (uint16_t)(address >> 32),
                               (uint32_t)address, bh->b_data, bh->b_size)) {
        free(bh->b_data);
        free(bh);
        return NULL;
    }
    return bh;
}

void brelse(struct buffer_head *bh)
{
    if (bh) {
        free(bh->b_data);
        free(bh);
    }
}

int sevenzip_udf_icb(void *context, sevenzip_udf_read read, uint32_t block_size,
                     const uint32_t *lengths, uint32_t partitions,
                     uint16_t *partition, uint32_t *block, void *data)
{
    struct super_block sb = {0};
    struct inode inode = {0};
    int result;
    uint32_t i;
    sb.info.s_partmaps = calloc(partitions, sizeof(*sb.info.s_partmaps));
    if (!sb.info.s_partmaps)
        return -ENOMEM;
    for (i = 0; i < partitions; ++i)
        sb.info.s_partmaps[i].s_partition_len = lengths[i];
    sb.info.s_partitions = partitions;
    sb.s_blocksize = block_size;
    sb.context = context;
    sb.read = read;
    inode.i_sb = &sb;
    inode.info.i_location.logicalBlockNum = *block;
    inode.info.i_location.partitionReferenceNum = *partition;
    result = sevenzip_udf_read_inode(&inode);
    if (!result) {
        memcpy(data, inode.entry->b_data, block_size);
        *block = inode.info.i_location.logicalBlockNum;
        *partition = inode.info.i_location.partitionReferenceNum;
    }
    brelse(inode.entry);
    free(sb.info.s_partmaps);
    return result;
}

int sevenzip_udf_extents(void *context, sevenzip_udf_read read,
                         sevenzip_udf_extent emit, uint32_t block_size,
                         uint32_t partitions, uint16_t partition, unsigned type,
                         int extended, const void *data, uint32_t size,
                         uint64_t file_size, uint32_t max_extents)
{
    struct super_block sb = {0};
    struct inode inode = {0};
    struct extent_position pos = {0};
    uint32_t count = 0;
    int result = 0;
    sb.info.s_partitions = partitions;
    sb.s_blocksize = block_size;
    sb.context = context;
    sb.read = read;
    inode.i_sb = &sb;
    inode.info.i_data = (uint8_t *)data;
    inode.info.i_lenAlloc = size;
    inode.info.i_alloc_type = type;
    inode.info.i_efe = extended != 0;
    inode.info.i_location.partitionReferenceNum = partition;
    while (file_size) {
        struct kernel_lb_addr loc;
        uint32_t length;
        int8_t extent_type;
        result = udf_next_aext(&inode, &pos, &loc, &length, &extent_type, 1);
        if (result <= 0)
            break;
        if (count++ >= max_extents || emit(context, (unsigned)extent_type, length,
                                          loc.logicalBlockNum, loc.partitionReferenceNum)) {
            result = -EIO;
            break;
        }
        file_size -= length < file_size ? length : file_size;
    }
    brelse(pos.bh);
    return result < 0 ? result : 0;
}

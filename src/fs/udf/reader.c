// SPDX-License-Identifier: GPL-2.0-only
#include "udfdecl.h"

struct sevenzip_udf_sparing {
    struct super_block sb;
    struct udf_part_map map;
};

void sevenzip_udf_sparing_free(struct sevenzip_udf_sparing *state)
{
    unsigned i;
    if (!state)
        return;
    for (i = 0; i < 4; ++i)
        brelse(state->map.s_type_specific.s_sparing.s_spar_map[i]);
    free(state);
}

int sevenzip_udf_sparing_create(void *context, sevenzip_udf_read read,
                                uint32_t block_size, uint32_t root, const void *raw,
                                struct sevenzip_udf_sparing **output)
{
    struct sevenzip_udf_sparing *state = calloc(1, sizeof(*state));
    struct sparablePartitionMap map;
    int result;
    *output = NULL;
    if (!state)
        return -ENOMEM;
    memcpy(&map, raw, sizeof(map));
    state->sb.info.s_partitions = 1;
    state->sb.info.s_partmaps = &state->map;
    state->sb.s_blocksize = block_size;
    state->sb.context = context;
    state->sb.read = read;
    state->map.s_partition_root = root;
    result = sevenzip_udf_load_sparable(&state->sb, &state->map, &map);
    if (result) {
        sevenzip_udf_sparing_free(state);
        return result;
    }
    state->sb.context = NULL;
    state->sb.read = NULL;
    *output = state;
    return 0;
}

int sevenzip_udf_sparing_span(const struct sevenzip_udf_sparing *state, uint32_t block,
                              uint32_t *physical, uint32_t *blocks)
{
    struct super_block sb = state->sb;
    *physical = udf_get_pblock_spar15(&sb, block, 0, 0);
    if (*physical == UINT32_MAX)
        return -EIO;
    *blocks = state->map.s_type_specific.s_sparing.s_packet_len -
        (block & (state->map.s_type_specific.s_sparing.s_packet_len - 1));
    return 0;
}

struct buffer_head *sb_bread(struct super_block *sb, udf_pblk_t address)
{
    struct buffer_head *bh = malloc(sizeof(*bh));
    if (!bh)
        return NULL;
    bh->b_size = sb->s_blocksize;
    bh->references = 1;
    bh->b_data = malloc(bh->b_size);
    if (!bh->b_data) {
        free(bh);
        sb->io_error = -ENOMEM;
        return NULL;
    }
    sb->io_error = sb->read(sb->context, (uint16_t)(address >> 32),
                            (uint32_t)address, bh->b_data, bh->b_size);
    if (sb->io_error) {
        free(bh->b_data);
        free(bh);
        return NULL;
    }
    return bh;
}

void brelse(struct buffer_head *bh)
{
    if (bh && --bh->references == 0) {
        free(bh->b_data);
        free(bh);
    }
}

int sevenzip_udf_icb(void *context, sevenzip_udf_read read, uint32_t block_size,
                     const uint32_t *lengths, uint32_t partitions,
                     uint16_t *partition, uint32_t *block, void *data,
                     int hidden, struct sevenzip_udf_inode *attributes)
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
    sb.info.s_uid = sb.info.s_gid = 65534;
    sb.info.s_fmode = sb.info.s_dmode = UDF_INVALID_MODE;
    sb.s_blocksize = block_size;
    while ((1U << sb.s_blocksize_bits) < block_size)
        ++sb.s_blocksize_bits;
    sb.context = context;
    sb.read = read;
    inode.i_sb = &sb;
    inode.info.i_location.logicalBlockNum = *block;
    inode.info.i_location.partitionReferenceNum = *partition;
    result = sevenzip_udf_read_inode(&inode, hidden != 0);
    if (!result) {
        memcpy(data, inode.entry->b_data, block_size);
        *block = inode.info.i_location.logicalBlockNum;
        *partition = inode.info.i_location.partitionReferenceNum;
        attributes->mode = inode.i_mode;
        attributes->uid = inode.i_uid;
        attributes->gid = inode.i_gid;
        attributes->links = inode.i_nlink;
        attributes->device_major = MAJOR(inode.i_rdev);
        attributes->device_minor = MINOR(inode.i_rdev);
        attributes->hidden = inode.info.i_hidden;
    }
    brelse(inode.entry);
    free(inode.info.i_data);
    free(sb.info.s_partmaps);
    return result;
}

int sevenzip_udf_next_aext(struct sevenzip_udf_walk *walk, struct inode *inode,
                          struct extent_position *pos, struct kernel_lb_addr *location,
                          uint32_t *length, int8_t *type)
{
    int result = udf_next_aext(inode, pos, location, length, type, 1);
    bool external;
    if (result <= 0)
        return result;
    external = pos->bh != NULL;
    if (walk->power) {
        if (walk->external == external && walk->offset == pos->offset &&
            walk->block.logicalBlockNum == pos->block.logicalBlockNum &&
            walk->block.partitionReferenceNum == pos->block.partitionReferenceNum)
            return -EFSCORRUPTED;
        if (++walk->distance != walk->power)
            return result;
        if (walk->power > UINT64_MAX / 2)
            return -EFSCORRUPTED;
        walk->power *= 2;
    } else {
        walk->power = 1;
    }
    walk->distance = 0;
    walk->block = pos->block;
    walk->offset = pos->offset;
    walk->external = external;
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
    struct sevenzip_udf_walk walk = {0};
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
        result = sevenzip_udf_next_aext(&walk, &inode, &pos, &loc, &length, &extent_type);
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

struct inode *udf_iget_special(struct super_block *sb, const struct kernel_lb_addr *loc)
{
    struct sevenzip_udf_file file = {0};
    struct inode *inode = calloc(1, sizeof(*inode));
    int error;
    if (!inode)
        return ERR_PTR(-ENOMEM);
    error = sb->open_file(sb->context, loc->partitionReferenceNum, loc->logicalBlockNum, &file);
    if (error) {
        sb->io_error = error;
        free(inode);
        return ERR_PTR(-EIO);
    }
    inode->i_sb = sb;
    inode->i_size = file.size;
    inode->file = file.handle;
    inode->info.i_location = *loc;
    inode->info.cached_extent.lstart = -1;
    inode->info.i_location.logicalBlockNum = file.block;
    inode->info.i_location.partitionReferenceNum = file.partition;
    inode->info.i_alloc_type = file.allocation_type;
    inode->info.i_efe = file.extended;
    inode->info.i_lenAlloc = file.allocation_size;
    inode->info.i_data = (uint8_t *)(file.inline_data ? file.data : file.allocation_data);
    return inode;
}

void iput(struct inode *inode)
{
    if (inode) {
        udf_clear_extent_cache(inode);
        inode->i_sb->close_file(inode->file);
        free(inode);
    }
}

struct buffer_head *udf_bread(struct inode *inode, uint32_t block, int create, int *error)
{
    struct super_block *sb = inode->i_sb;
    struct buffer_head *bh = calloc(1, sizeof(*bh));
    (void)create;
    *error = -ENOMEM;
    if (!bh)
        return NULL;
    bh->b_size = sb->s_blocksize;
    bh->references = 1;
    bh->b_data = malloc(bh->b_size);
    if (!bh->b_data) {
        brelse(bh);
        return NULL;
    }
    sb->io_error = sb->read_file(inode->file, block, bh->b_data, bh->b_size);
    if (sb->io_error) {
        *error = -EIO;
        brelse(bh);
        return NULL;
    }
    *error = 0;
    return bh;
}

// SPDX-License-Identifier: GPL-2.0-only
/* Stream services for Linux volume recognition and descriptor selection. */
#include "udfdecl.h"

int sb_set_blocksize(struct super_block *sb, unsigned size)
{
    if (size < 512 || size > 4096 || !is_power_of_2(size))
        return 0;
    sb->s_blocksize = size;
    sb->s_blocksize_bits = 0;
    while ((1U << sb->s_blocksize_bits) < size)
        ++sb->s_blocksize_bits;
    sb->blocks = sb->device_size / size;
    return (int)size;
}

int sevenzip_udf_discover(void *context, sevenzip_udf_read read,
    sevenzip_udf_descriptor descriptor, sevenzip_udf_reset reset, sevenzip_udf_status status,
    uint64_t size, struct sevenzip_udf_discovery *output)
{
    struct super_block sb = {0};
    struct udf_options options = {0};
    int result = -EAGAIN;
    memset(output, 0, sizeof(*output));
    sb.context = context;
    sb.read = read;
    sb.descriptor = descriptor;
    sb.reset = reset;
    sb.device_size = size;
    sb.info.s_session = udf_get_last_session(&sb);
    /* The stream has the regular-file branch's 512-byte logical device sector. */
    for (options.blocksize = 512; options.blocksize <= 4096; options.blocksize <<= 1) {
        struct kernel_lb_addr fileset = {UINT32_MAX, UINT16_MAX};
        brelse(sb.info.s_lvid_bh);
        sb.info.s_lvid_bh = NULL;
        reset(context);
        result = sevenzip_udf_load_vrs(&sb, &options, &fileset);
        if (status(context))
            break;
        if (!result || result == -EACCES || result == -ENOMEM)
            break;
    }
    if (!result && sb.info.s_lvid_bh) {
        struct logicalVolIntegrityDescImpUse *use = udf_sb_lvidiu(&sb);
        if (!use || le16_to_cpu(use->minUDFReadRev) > UDF_MAX_READ_VERSION)
            result = -EINVAL;
    }
    output->seen_anchor = sb.seen_anchor;
    output->block_size = sb.s_blocksize;
    output->last_block = (uint32_t)sb.info.s_last_block;
    output->anchor_block = sb.anchor_block;
    brelse(sb.info.s_lvid_bh);
    return result;
}

struct memory_volume { const uint8_t *data; size_t size; bool incomplete; };
static int read_memory(void *opaque, uint16_t partition, uint32_t block, void *data, size_t size)
{
    struct memory_volume *volume = opaque;
    uint64_t offset = (uint64_t)block * size;
    if (partition || offset > volume->size || size > volume->size - offset) {
        volume->incomplete = true;
        return 1;
    }
    memcpy(data, volume->data + offset, size);
    return 0;
}

int sevenzip_udf_probe(const void *data, size_t size)
{
    struct memory_volume volume = {data, size, false};
    struct super_block sb = {0};
    unsigned block_size;
    sb.context = &volume;
    sb.read = read_memory;
    sb.device_size = size;
    for (block_size = 2048; block_size <= 4096; block_size <<= 1) {
        sb_set_blocksize(&sb, block_size);
        if (sevenzip_udf_check_vsd(&sb) == 1)
            return 1;
    }
    return volume.incomplete ? -EAGAIN : 0;
}

int sevenzip_udf_fileset(void *context, sevenzip_udf_read read, uint32_t block_size,
                        uint16_t partition, uint32_t block, void *data)
{
    struct super_block sb = {0};
    struct kernel_lb_addr fileset = {block, partition}, root;
    sb.context = context;
    sb.read = read;
    sb.s_blocksize = block_size;
    sb.descriptor_output = data;
    return sevenzip_udf_find_fileset(&sb, &fileset, &root);
}

// SPDX-License-Identifier: GPL-2.0-or-later
// Archive memory/I/O services around the original SquashFS decompressor units.
#include "squashfsdecoder.h"
#include "squashfsio.h"
#include "fs/squashfs/squashfscompat.h"
#include "fs/squashfs/squashfs_fs.h"
#include "fs/squashfs/squashfs_fs_sb.h"
#include "fs/squashfs/decompressor.h"
#include "fs/squashfs/page_actor.h"
#include "fs/squashfs/squashfs.h"

extern const struct squashfs_decompressor_thread_ops squashfs_decompressor_single;

struct sevenzip_squashfs_decoder {
    struct squashfs_sb_info msblk;
    struct sevenzip_squashfs_io io;
};

// The module supplies bounded zlib/XZ/Zstd decoders. The Linux
// framework still parses their options; its stream wrapper owns no extra state.
void *sevenzip_squashfs_external_init(struct squashfs_sb_info *msblk, void *opts)
{
    (void)msblk; (void)opts;
    return NULL;
}
void sevenzip_squashfs_external_free(void *stream) { (void)stream; }
const struct squashfs_decompressor squashfs_zlib_comp_ops = {
    .init = sevenzip_squashfs_external_init, .free = sevenzip_squashfs_external_free,
    .id = ZLIB_COMPRESSION, .name = "zlib", .supported = 1
};
const struct squashfs_decompressor squashfs_zstd_comp_ops = {
    .init = sevenzip_squashfs_external_init, .free = sevenzip_squashfs_external_free,
    .id = ZSTD_COMPRESSION, .name = "zstd", .supported = 1
};

int sevenzip_squashfs_decoder_create(unsigned method, unsigned block_size, unsigned flags,
    uint64_t bytes_used, sevenzip_squashfs_read read, void *opaque,
    struct sevenzip_squashfs_decoder **result)
{
    struct sevenzip_squashfs_decoder *decoder;
    struct super_block sb;
    int error;
    *result = NULL;
    if (!read || !block_size || block_size > SQUASHFS_FILE_MAX_SIZE || bytes_used > INT64_MAX)
        return -EINVAL;
    decoder = calloc(1, sizeof(*decoder));
    if (!decoder) return -ENOMEM;
    decoder->io.read = read;
    decoder->io.opaque = opaque;
    decoder->msblk.block_size = block_size;
    decoder->msblk.bytes_used = (long long)bytes_used;
    decoder->msblk.read_context = &decoder->io;
    decoder->msblk.thread_ops = &squashfs_decompressor_single;
    decoder->msblk.decompressor = squashfs_lookup_decompressor((int)method);
    if (!decoder->msblk.decompressor->supported) {
        free(decoder);
        return -95;
    }
    sb.s_fs_info = &decoder->msblk;
    decoder->msblk.stream = squashfs_decompressor_setup(&sb, (unsigned short)flags);
    if (IS_ERR(decoder->msblk.stream)) {
        error = PTR_ERR(decoder->msblk.stream);
        free(decoder);
        return error;
    }
    // Source callbacks are borrowed only during setup, never by a returned stream.
    decoder->io.read = NULL;
    decoder->io.opaque = NULL;
    *result = decoder;
    return 0;
}

void sevenzip_squashfs_decoder_destroy(struct sevenzip_squashfs_decoder *decoder)
{
    if (!decoder) return;
    decoder->msblk.thread_ops->destroy(&decoder->msblk);
    free(decoder);
}

int sevenzip_squashfs_decode(struct sevenzip_squashfs_decoder *decoder, const void *source,
    unsigned source_size, void *destination, unsigned destination_size)
{
    struct bio bio;
    struct squashfs_page_actor *actor;
    void *pages[SQUASHFS_FILE_MAX_SIZE / PAGE_SIZE];
    unsigned i, count;
    int result;
    if (!decoder || !source || !destination || !source_size || !destination_size ||
        !decoder->msblk.decompressor->decompress ||
        source_size > max_t(unsigned, decoder->msblk.block_size, SQUASHFS_METADATA_SIZE) ||
        destination_size > max_t(unsigned, decoder->msblk.block_size, SQUASHFS_METADATA_SIZE))
        return -EIO;
    count = (destination_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (i = 0; i < count; ++i) pages[i] = (char *)destination + (size_t)i * PAGE_SIZE;
    actor = squashfs_page_actor_init(pages, (int)count, (int)destination_size);
    if (!actor) return -ENOMEM;
    bio.segment.data = source;
    bio.segment.bv_len = source_size;
    result = decoder->msblk.thread_ops->decompress(&decoder->msblk, &bio, 0, (int)source_size, actor);
    free(actor);
    return result;
}

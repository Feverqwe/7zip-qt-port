// SPDX-License-Identifier: GPL-2.0-only
// Private inode ownership around the original NTFS3 reparse/link readers.
#include "formats/ntfs/ntfslinkcompat.h"

struct link_context {
    struct ntfs_inode ni;
    struct ntfs_sb_info sbi;
    struct super_block sb;
    struct ntfs_mount_options options;
    struct ATTRIB attr;
};
static int link_context_read(void *opaque, uint64_t offset, void *data, size_t size)
{
    const struct link_context *ctx = opaque;
    const uint64_t bytes = le64_to_cpu(ctx->attr.nres.data_size);
    if (offset > bytes || size > bytes - offset)
        return -EINVAL;
    memcpy(data, (const u8 *)ctx->attr.data + (size_t)offset, size);
    return 0;
}
static void link_context_init(struct link_context *ctx, const void *data, size_t size, uint64_t limit)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->attr.data = data;
    ctx->attr.res.data_size = cpu_to_le32((u32)size);
    if (size > UINT32_MAX) {
        ctx->attr.non_res = 1;
        ctx->attr.nres.data_size = cpu_to_le64(size);
        ctx->sbi.read = link_context_read;
        ctx->sbi.opaque = ctx;
    }
    ctx->ni.base = &ctx->ni;
    ctx->ni.mi.sbi = &ctx->sbi;
    ctx->ni.vfs_inode.i_sb = &ctx->sb;
    ctx->ni.vfs_inode.ntfs = &ctx->ni;
    ctx->ni.reparse = &ctx->attr;
    ctx->sbi.options = &ctx->options;
    ctx->sbi.sb = &ctx->sb;
    ctx->sbi.reparse.max_size = limit;
    ctx->sb.s_fs_info = &ctx->sbi;
}
int sevenzip_ntfs_classify_reparse(const void *data, size_t size, unsigned *frame_bits)
{
    struct link_context ctx;
    struct REPARSE_DATA_BUFFER header;
    int type;
    if (frame_bits) *frame_bits = 0;
    if (!data) return 0;
    link_context_init(&ctx, data, size, size);
    type = ni_parse_reparse(&ctx.ni, &ctx.attr, &header);
    if (frame_bits && type == REPARSE_COMPRESSED)
        *frame_bits = ni_ext_compress_bits(&ctx.ni);
    return type;
}
int sevenzip_ntfs_readlink(const void *data, size_t size, const char *path,
                          char *output, size_t capacity, uint64_t limit)
{
    struct link_context ctx;
    struct dentry de;
    if (!data || !path || !output || size > limit || capacity < PATH_MAX)
        return -EINVAL;
    link_context_init(&ctx, data, size, limit);
    de.path = path;
    /* NTFS3 readlink writes a final NUL, with a fixed page-sized output. */
    return ntfs_archive_readlink(&de, &ctx.ni.vfs_inode, output, PATH_MAX);
}

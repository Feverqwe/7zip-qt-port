// SPDX-License-Identifier: GPL-2.0-only
// Private inode ownership around the original NTFS3 reparse/link readers.
#include "fs/ntfs3/ntfslinkcompat.h"

struct link_context {
    struct ntfs_inode ni;
    struct ntfs_sb_info sbi;
    struct super_block sb;
    struct ntfs_mount_options options;
    struct ATTRIB attr;
};
static void link_context_init(struct link_context *ctx, const void *data, size_t size)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->attr.data = data;
    ctx->attr.res.data_size = cpu_to_le32((u32)size);
    ctx->ni.base = &ctx->ni;
    ctx->ni.mi.sbi = &ctx->sbi;
    ctx->ni.vfs_inode.i_sb = &ctx->sb;
    ctx->ni.vfs_inode.ntfs = &ctx->ni;
    ctx->ni.reparse = &ctx->attr;
    ctx->sbi.options = &ctx->options;
    ctx->sbi.sb = &ctx->sb;
    ctx->sbi.reparse.max_size = 16384;
    ctx->sb.s_fs_info = &ctx->sbi;
}
int sevenzip_ntfs_classify_reparse(const void *data, size_t size)
{
    struct link_context ctx;
    struct REPARSE_DATA_BUFFER header;
    if (!data || size > 16384) return 0;
    link_context_init(&ctx, data, size);
    return ni_parse_reparse(&ctx.ni, &ctx.attr, &header);
}
int sevenzip_ntfs_readlink(const void *data, size_t size, const char *path,
                          char *output, size_t capacity)
{
    struct link_context ctx;
    struct dentry de;
    if (!data || !path || !output || size > 16384 || capacity < PATH_MAX)
        return -EINVAL;
    link_context_init(&ctx, data, size);
    de.path = path;
    /* NTFS3 readlink writes a final NUL, with a fixed page-sized output. */
    return ntfs_archive_readlink(&de, &ctx.ni.vfs_inode, output, PATH_MAX);
}

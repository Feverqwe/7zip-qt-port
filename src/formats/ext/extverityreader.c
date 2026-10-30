/* SPDX-License-Identifier: GPL-2.0-only */
/* Archive-owned contexts and page/extent services for Linux fs-verity. */
#include "formats/ext/extverityreader.h"
#include "crypto/verity/fsverity_private.h"

struct ext_verity_reader {
    struct inode inode;
    struct super_block sb;
    struct fsverity_info *info;
    ext_verity_read read;
    void *context;
    u32 allocated_end;
    struct page *pages[4];
    u64 age;
};

struct page *read_mapping_folio(struct inode *inode, pgoff_t index, void *unused)
{
    struct ext_verity_reader *reader = inode->verity_context;
    u64 limit = (u64)reader->allocated_end << inode->i_blkbits;
    u64 offset;
    size_t amount;
    struct page *page;
    unsigned int slot = 4;
    if (index > (UINT64_MAX >> PAGE_SHIFT)) return ERR_PTR(-EIO);
    offset = index << PAGE_SHIFT;
    if (offset >= limit) return ERR_PTR(-EIO);
    for (unsigned int i = 0; i < 4; ++i) {
        struct page *cached = reader->pages[i];
        if (cached && cached->index == index) {
            ++cached->references;
            cached->age = ++reader->age;
            return cached;
        }
        if (!cached || (cached->references == 1 &&
            (slot == 4 || (reader->pages[slot] && cached->age < reader->pages[slot]->age))))
            slot = i;
    }
    amount = (size_t)min_t(u64, PAGE_SIZE, limit - offset);
    page = calloc(1, sizeof(*page) + PAGE_SIZE);
    if (!page) return ERR_PTR(-ENOMEM);
    page->data = (u8 *)(page + 1);
    page->index = index;
    page->references = 1;
    if (reader->read(reader->context, offset, page->data, amount)) {
        free(page);
        return ERR_PTR(-EIO);
    }
    if (slot != 4) {
        if (reader->pages[slot]) verity_put_page(reader->pages[slot]);
        reader->pages[slot] = page;
        ++page->references;
        page->age = ++reader->age;
    }
    return page;
}

struct ext4_ext_path *ext4_find_extent(struct inode *inode, u32 block, void *unused, int flags)
{
    struct ext_verity_reader *reader = inode->verity_context;
    struct ext4_ext_path *path;
    struct ext4_extent *extent;
    /* The archive adapter has already traversed and validated the complete map.
     * Only the final allocated endpoint is queried by ext4/verity.c. */
    if (block != EXT_MAX_BLOCKS - 1 || !reader->allocated_end) return ERR_PTR(-EIO);
    path = calloc(1, sizeof(*path) + sizeof(*extent));
    if (!path) return ERR_PTR(-ENOMEM);
    extent = (struct ext4_extent *)(path + 1);
    extent->ee_block = cpu_to_le32(reader->allocated_end - 1);
    extent->ee_len = cpu_to_le16(1);
    path->p_ext = extent;
    return path;
}

int ext_verity_open(ext_verity_read read, void *context, uint64_t file_size,
                    unsigned int block_bits, uint32_t allocated_end,
                    const struct ext_verity_signature_options *options,
                    struct ext_verity_reader **result, unsigned int *verity_block_bits)
{
    struct ext_verity_reader *reader;
    struct fsverity_descriptor *descriptor = NULL;
    struct fsverity_info *info;
    int error;
    *result = NULL;
    if (block_bits < 10 || block_bits > PAGE_SHIFT || file_size > INT64_MAX || !allocated_end)
        return -EINVAL;
    reader = calloc(1, sizeof(*reader));
    if (!reader) return -ENOMEM;
    reader->read = read;
    reader->context = context;
    reader->allocated_end = allocated_end;
    reader->sb.s_blocksize = 1U << block_bits;
    reader->sb.s_blocksize_bits = block_bits;
    reader->sb.s_vop = &ext4_read_verityops;
    reader->inode.i_sb = &reader->sb;
    reader->inode.i_mapping = &reader->inode;
    reader->inode.i_blkbits = block_bits;
    reader->inode.i_size = file_size;
    reader->inode.flags = (1U << EXT4_INODE_EXTENTS);
    reader->inode.verity_context = reader;
    if (options) {
        reader->inode.verity_trust = options->trust;
        reader->inode.verity_require_signatures = options->require_signatures != 0;
    }
    error = fsverity_get_descriptor(&reader->inode, &descriptor);
    if (error) goto fail;
    info = fsverity_create_info(&reader->inode, descriptor);
    free(descriptor);
    if (IS_ERR(info)) {
        error = PTR_ERR(info);
        goto fail;
    }
    reader->info = info;
    *verity_block_bits = info->tree_params.log_blocksize;
    *result = reader;
    return 0;
fail:
    ext_verity_close(reader);
    return error;
}

void ext_verity_close(struct ext_verity_reader *reader)
{
    if (reader) {
        if (reader->info) fsverity_free_info(reader->info);
        for (unsigned int i = 0; i < 4; ++i)
            if (reader->pages[i]) verity_put_page(reader->pages[i]);
        free(reader);
    }
}

int ext_verity_verify(struct ext_verity_reader *reader, uint64_t position, const void *block)
{
    struct page page = { .data = (u8 *)block, .index = position >> PAGE_SHIFT,
                         .base_offset = position & (PAGE_SIZE - 1) };
    const struct merkle_tree_params *params = &reader->info->tree_params;
    if (position >= reader->inode.i_size || !IS_ALIGNED(position, params->block_size)) return -EINVAL;
    return fsverity_verify_blocks(reader->info, &page, params->block_size, page.base_offset) ? 0 : -EIO;
}

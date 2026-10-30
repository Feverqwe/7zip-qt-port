// SPDX-License-Identifier: GPL-2.0-only
// Borrowed NTFS3 index readers use private immutable archive metadata.
#include "formats/ntfs/ntfsdirectorycompat.h"

int sevenzip_ntfs_directory_read(const struct sevenzip_ntfs_directory_input *input)
{
    struct ntfs_sb_info sbi = {0};
    struct ntfs_inode ni = {0};
    const struct INDEX_ROOT *root;
    u32 block;
    int err;
    if (!input || !input->read || !input->emit || !input->root ||
        input->root_size < sizeof(struct INDEX_ROOT) || input->root_size > UINT32_MAX ||
        input->bitmap_size > SIZE_MAX / 8 || input->bitmap_valid > input->bitmap_size ||
        input->cluster_bits < 9 || input->cluster_bits > 21)
        return -EINVAL;
    root = input->root;
    block = le32_to_cpu(root->index_block_size);
    if (root->type != ATTR_NAME || root->rule != NTFS_COLLATION_TYPE_FILENAME ||
        block < 512 || block > 4096 || (block & (block - 1)) ||
        !root->index_block_clst || (root->index_block_clst & (root->index_block_clst - 1)))
        return -EINVAL;
    sbi.cluster_bits = input->cluster_bits;
    sbi.cluster_size = 1u << input->cluster_bits;
    sbi.input.read = input->read;
    sbi.input.opaque = input->opaque;
    sbi.input.allocation_size = input->allocation_size;
    ni.mi.sbi = &sbi;
    ni.vfs_inode.owner = &ni;
    ni.vfs_inode.size = input->allocation_size;
    ni.directory_input = input;
    ni.root.data = input->root;
    ni.root.res.data_size = cpu_to_le32((u32)input->root_size);
    ni.dir.alloc_run.source = 1;
    ni.directory_unverified = (input->allocation_size & (block - 1)) != 0;
    err = indx_init(&ni.dir, &sbi, &ni.root, INDEX_MUTEX_I30);
    if (!err)
        err = ntfs_directory_count(&ni);
    return err ? err : sbi.fixup_corrected ? -EINVAL : 0;
}

// SPDX-License-Identifier: GPL-2.0-only
// Archive ownership and bounded logical-stream services around NTFS3 readers.
#include "formats/ntfs/ntfssecuritycompat.h"

void sevenzip_ntfs_security_destroy(struct sevenzip_ntfs_security *reader)
{
    if (reader) {
        free(reader->root);
        free(reader);
    }
}

int sevenzip_ntfs_security_create(const struct sevenzip_ntfs_security_input *input,
                                 struct sevenzip_ntfs_security **result)
{
    struct sevenzip_ntfs_security *reader;
    const struct INDEX_ROOT *root;
    u32 block;
    int err;
    *result = NULL;
    if (!input || !input->read || !input->root ||
        input->root_size < sizeof(struct INDEX_ROOT) || input->root_size > UINT32_MAX ||
        input->cluster_bits < 9 || input->cluster_bits > 21)
        return -EINVAL;
    root = input->root;
    block = le32_to_cpu(root->index_block_size);
    /* NTFS3 super.c validates the supported power-of-two 512..4096 geometry. */
    if (root->type != ATTR_ZERO || root->rule != NTFS_COLLATION_TYPE_UINT ||
        block < 512 || block > 4096 || (block & (block - 1)) ||
        !root->index_block_clst || (root->index_block_clst & (root->index_block_clst - 1)))
        return -EINVAL;
    reader = calloc(1, sizeof(*reader));
    if (!reader)
        return -ENOMEM;
    reader->root = malloc(input->root_size);
    if (!reader->root) {
        sevenzip_ntfs_security_destroy(reader);
        return -ENOMEM;
    }
    memcpy(reader->root, root, input->root_size);
    reader->sbi.input = *input;
    reader->sbi.cluster_bits = input->cluster_bits;
    reader->sbi.cluster_size = 1u << input->cluster_bits;
    reader->sbi.security.ni = &reader->ni;
    reader->sbi.security.index_sii.alloc_run.source = 1;
    reader->ni.mi.sbi = &reader->sbi;
    reader->ni.root.data = reader->root;
    reader->ni.root.res.data_size = cpu_to_le32((u32)input->root_size);
    err = indx_init(&reader->sbi.security.index_sii, &reader->sbi,
                    &reader->ni.root, INDEX_MUTEX_SII);
    if (err) {
        sevenzip_ntfs_security_destroy(reader);
        return err;
    }
    *result = reader;
    return 0;
}

int sevenzip_ntfs_security_read(struct sevenzip_ntfs_security *reader, u32 id,
                               void *output, size_t capacity, size_t *length)
{
    struct SECURITY_DESCRIPTOR_RELATIVE *descriptor = NULL;
    size_t bytes = 0;
    int err;
    *length = 0;
    if (id < 0x100)
        return 0; // xattr.c::ntfs_getxattr excludes reserved security IDs.
    reader->sbi.fixup_corrected = false;
    err = ntfs_get_security_by_id(&reader->sbi, cpu_to_le32(id), &descriptor, &bytes);
    if (err || !descriptor)
        goto out;
    if (reader->sbi.fixup_corrected) {
        err = -E_NTFS_FIXUP;
        goto out;
    }
    /* Keep all original SD validation, with non-wrapping offset preconditions. */
    if (bytes < sizeof(*descriptor) || le32_to_cpu(descriptor->Owner) > bytes ||
        le32_to_cpu(descriptor->Group) > bytes || le32_to_cpu(descriptor->Sacl) > bytes ||
        le32_to_cpu(descriptor->Dacl) > bytes || !is_sd_valid(descriptor, (u32)bytes)) {
        err = -EINVAL;
        goto out;
    }
    if (!output || capacity < bytes) {
        err = -ERANGE;
        goto out;
    }
    memcpy(output, descriptor, bytes);
    *length = bytes;
out:
    free(descriptor);
    return err;
}

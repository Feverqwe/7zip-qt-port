// SPDX-License-Identifier: GPL-2.0-only
// Immutable archive inode/stream boundary for NTFS3 directory enumeration.
#ifndef SEVENZIP_NTFS_DIRECTORY_COMPAT_H
#define SEVENZIP_NTFS_DIRECTORY_COMPAT_H
#include "ntfssecuritycompat.h"
#include "../../ntfsdirectory.h"
#define MINUS_ONE_T SIZE_MAX
#define ntfs_i(inode) ((inode)->owner)
#define i_size_read(inode) ((inode)->size)
static inline void ntfs_directory_check_child(struct ntfs_inode *ni, const struct NTFS_DE *e,
                                               const struct indx_node *node) {
    if (de_has_vcn_ex(e)) {
        const u64 vbn = de_get_vbn(e);
        const u64 size = ni->directory_input->allocation_size;
        if (vbn > (UINT64_MAX >> ni->dir.vbn2vbo_bits) ||
            (vbn << ni->dir.vbn2vbo_bits) > size ||
            (UINT64_C(1) << ni->dir.index_bits) > size - (vbn << ni->dir.vbn2vbo_bits) ||
            (node && vbn == le64_to_cpu(node->index->vbn)))
            ni->directory_unverified = true;
    }
}
static inline int ntfs_directory_emit(struct ntfs_inode *ni, const struct NTFS_DE *e,
                                      const struct ATTR_FILE_NAME *fname) {
    const struct sevenzip_ntfs_directory_input *input = ni->directory_input;
    if (fname_full_size(fname) > le16_to_cpu(e->key_size))
        return -EINVAL;
    if (get_unaligned_le64(&fname->home) != input->parent_ref ||
        (get_unaligned_le64(&e->ref) & UINT64_C(0xffffffffffff)) == 5)
        return 0; // ntfs_dir_emit: skip a foreign parent or the root's own entry.
    return input->emit(input->opaque, get_unaligned_le64(&e->ref), fname, fname_full_size(fname));
}
#endif

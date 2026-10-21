// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEVENZIP_SQUASHFS_METADATA_H
#define SEVENZIP_SQUASHFS_METADATA_H
#include "squashfsdecoder.h"
#ifdef __cplusplus
extern "C" {
#endif
struct sevenzip_squashfs_metadata;
// The caller supplies a validated v4 superblock size and retains callback state
// until destroy. Metadata callbacks are serialized through this reader.
// Decode one metadata block: size is capacity on input and actual size on output;
// next is the absolute next block position. Return zero or a negative error.
typedef int (*sevenzip_squashfs_read_meta)(void *, uint64_t, void *, unsigned *, uint64_t *);
int sevenzip_squashfs_metadata_create(uint64_t bytes_used, sevenzip_squashfs_read,
                                      sevenzip_squashfs_read_meta, void *,
                                      struct sevenzip_squashfs_metadata **,
                                      uint64_t *directory_end);
void sevenzip_squashfs_metadata_destroy(struct sevenzip_squashfs_metadata *);
int sevenzip_squashfs_id(struct sevenzip_squashfs_metadata *, unsigned, unsigned *);
int sevenzip_squashfs_fragment(struct sevenzip_squashfs_metadata *, unsigned, uint64_t *,
                               unsigned *);
int sevenzip_squashfs_inode(struct sevenzip_squashfs_metadata *, unsigned, uint64_t *);
int sevenzip_squashfs_validate_xattrs(struct sevenzip_squashfs_metadata *, unsigned);
typedef int (*sevenzip_squashfs_xattr_emit)(void *, const char *, const void *, size_t);
// Names/values are borrowed during emit; nonzero stops and is returned unchanged.
// Unreadable values are omitted, with a negative final result for the incomplete
// set. Allocation failure and cancellation stop immediately.
int sevenzip_squashfs_visit_xattrs(struct sevenzip_squashfs_metadata *, unsigned,
                                   sevenzip_squashfs_xattr_emit, void *);
int sevenzip_squashfs_xattrs(struct sevenzip_squashfs_metadata *, unsigned, char *, size_t);
int sevenzip_squashfs_xattr(struct sevenzip_squashfs_metadata *, unsigned, const char *, void *,
                            size_t);
#ifdef __cplusplus
}
#endif
#endif

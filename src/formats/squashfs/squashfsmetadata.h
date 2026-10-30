// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEVENZIP_SQUASHFS_METADATA_H
#define SEVENZIP_SQUASHFS_METADATA_H
#include "formats/squashfs/squashfsdecoder.h"
#ifdef __cplusplus
extern "C" {
#endif
struct sevenzip_squashfs_metadata;
struct sevenzip_squashfs_file;
uint32_t sevenzip_squashfs_decode_device(uint32_t device);
enum {
    SEVENZIP_SQUASHFS_XATTR_UNAVAILABLE = 1,
    SEVENZIP_SQUASHFS_IDS_UNAVAILABLE = 2,
    SEVENZIP_SQUASHFS_EXPORT_UNAVAILABLE = 4,
    SEVENZIP_SQUASHFS_FRAGMENTS_UNAVAILABLE = 8
};
// The caller supplies a validated v4 superblock size and retains callback state
// until destroy. Metadata callbacks are serialized through this reader.
// Decode one metadata block: size is capacity on input and actual size on output;
// next is the absolute next block position. Return zero or a negative error.
typedef int (*sevenzip_squashfs_read_meta)(void *, uint64_t, void *, unsigned *, uint64_t *);
// Independent bounded blocklist reader. No ancillary tables are loaded.
int sevenzip_squashfs_file_create(uint64_t bytes_used, uint64_t inode_table,
                                  uint64_t directory_table, unsigned block_size, uint64_t reference,
                                  unsigned header_size, uint64_t file_size, uint64_t block_count,
                                  uint64_t data_start, unsigned inode_number,
                                  sevenzip_squashfs_read_meta, void *,
                                  struct sevenzip_squashfs_file **);
void sevenzip_squashfs_file_destroy(struct sevenzip_squashfs_file *);
int sevenzip_squashfs_file_block(struct sevenzip_squashfs_file *, uint64_t index, uint64_t *start,
                                 unsigned *size);
int sevenzip_squashfs_metadata_create(uint64_t bytes_used, sevenzip_squashfs_read,
                                      sevenzip_squashfs_read_meta, void *,
                                      struct sevenzip_squashfs_metadata **,
                                      uint64_t *directory_end);
void sevenzip_squashfs_metadata_destroy(struct sevenzip_squashfs_metadata *);
// Read exactly size bytes through the native metadata cursor, within directory
// table bounds. Partial field output on failure must be discarded by the caller.
int sevenzip_squashfs_directory_read(struct sevenzip_squashfs_metadata *, uint64_t *block,
                                     unsigned *offset, void *, unsigned size);
int sevenzip_squashfs_inode_read(struct sevenzip_squashfs_metadata *, uint64_t *block,
                                 unsigned *offset, void *, unsigned size);
// Copy a complete single-block inode table only if it is already cached.
unsigned sevenzip_squashfs_cached_inodes(struct sevenzip_squashfs_metadata *, void *, unsigned);
unsigned sevenzip_squashfs_unavailable_tables(const struct sevenzip_squashfs_metadata *);
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

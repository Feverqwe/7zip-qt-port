// SPDX-License-Identifier: MIT
// Read-only archive boundary for Linux ext4 metadata services.
#ifndef SEVENZIP_EXT_READER_H
#define SEVENZIP_EXT_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct ext_volume;
struct ext_node;
typedef int (*ext_dir_read_block)(void *, uint32_t, void *, size_t);
typedef int (*ext_dir_parse_block)(void *, uint32_t, const void *, size_t);
int ext_linux_index_directory(struct ext_volume *, uint32_t ino, uint32_t seed,
                              uint32_t flags, uint64_t size, void *context,
                              ext_dir_read_block, ext_dir_parse_block);
struct sevenzip_fscrypt_geometry;
struct sevenzip_fscrypt_inode;
struct ext_link_services {
    void *context;
    int (*read_block)(void *, void *, size_t);
};
int ext_linux_readlink(struct ext_node *, const struct ext_link_services *,
                        void *, size_t, size_t *);
uint16_t crc16(uint16_t, const unsigned char *, size_t);
struct ext_time { int64_t seconds; uint32_t nsec; int defined; };
struct ext_node_info {
    uint64_t size;
    uint32_t checksum_seed, inline_size, has_xattrs, fast_symlink;
    int read_error, special_read_error, ea_read_error;
    struct ext_time atime, mtime, change_time, birth_time;
};
/* All buffers remain owned by the caller except the returned opaque objects. */
int ext_linux_open(const void *superblock, struct ext_volume **);
int ext_linux_geometry_check(struct ext_volume *, uint64_t input_size);
void ext_linux_close(struct ext_volume *);
uint64_t ext_linux_descriptor_block(struct ext_volume *, unsigned int number);
int ext_linux_group_check(struct ext_volume *, uint32_t group, const void *descriptor);
int ext_linux_bitmap_check(struct ext_volume *, uint32_t group, const void *descriptor,
                           const void *bitmap, int inode_bitmap);
/* Setup borrows callbacks only during the call and copies descriptors. */
struct ext_block_services {
    void *context;
    int (*inode)(void *, uint32_t ino, void *, size_t);
    int (*map)(void *, uint32_t ino, uint32_t logical, uint32_t *count, uint64_t *physical);
};
int ext_linux_system_zone(struct ext_volume *, const void *descriptors, uint32_t groups,
                          const struct ext_block_services *);
int ext_linux_block_valid(struct ext_volume *, uint32_t ino, uint64_t start, uint32_t count);
int ext_linux_blockrefs(struct ext_volume *, uint32_t ino, const void *refs, uint32_t count);
struct ext_indirect_services {
    void *context;
    int (*read)(void *, uint64_t block, void *, size_t);
};
struct ext_indirect;
int ext_linux_indirect_open(struct ext_volume *, uint32_t ino, const void *refs,
                            const struct ext_indirect_services *, struct ext_indirect **);
void ext_linux_indirect_close(struct ext_indirect *);
int ext_linux_indirect_lookup(struct ext_indirect *, uint32_t logical, uint32_t count,
                              uint64_t *, uint32_t *);
int ext_linux_indirect_map(struct ext_volume *, uint32_t ino, const void *refs,
                           uint32_t logical, uint32_t count,
                           const struct ext_indirect_services *, uint64_t *, uint32_t *);
int ext_linux_inode_open(struct ext_volume *, uint32_t ino, const void *raw,
                         struct ext_node **, struct ext_node_info *);
void ext_linux_inode_close(struct ext_node *);
int ext_linux_inline_read(struct ext_node *, void *output, unsigned int size);
typedef int (*ext_dir_emit)(void *, uint32_t ino, unsigned int type, const char *, unsigned int);
int ext_linux_inline_directory(struct ext_node *, ext_dir_emit, void *context);
int ext_linux_xattr_check(struct ext_node *, uint64_t block, const void *data);
/* Private eager-Open services. ino==0 reads physical filesystem bytes; other
 * values read an EA inode without accepting holes. Callbacks return -errno. */
typedef int (*ext_xattr_read_fn)(void *, uint32_t ino, uint64_t offset, void *, size_t);
typedef int (*ext_xattr_inode_fn)(void *, uint32_t ino, void *, size_t);
struct ext_xattr_services {
    void *context;
    ext_xattr_read_fn read;
    ext_xattr_inode_fn inode;
    /* Optional export only: return zero to skip a failed value/container.
     * Getters used for decoding filesystem data remain strict. */
    int (*handle_error)(void *, int);
};
typedef int (*ext_xattr_emit)(void *, const char *name, const void *value, size_t size);
int ext_linux_xattr_visit(struct ext_node *, const struct ext_xattr_services *, ext_xattr_emit, void *);
int ext_linux_fscrypt_context(struct ext_node *, const struct ext_xattr_services *, void *, size_t);
void ext_linux_fscrypt_geometry(struct ext_volume *, struct sevenzip_fscrypt_geometry *);
/* root_hash_version is -1 for an unindexed directory; otherwise dx_root's version. */
int ext_linux_directory_hash(struct ext_volume *, uint32_t flags, uint64_t directory_size,
                              const struct sevenzip_fscrypt_inode *, int root_hash_version, int linear,
                              const void *record, size_t record_size,
                              uint32_t *hash, uint32_t *minor_hash);
int ext_linux_extent_check(struct ext_volume *, uint32_t ino, uint32_t seed,
                           uint32_t root_depth, const void *block, size_t size,
                           uint32_t depth, uint64_t physical, uint32_t logical);
struct ext_extent_services {
    void *context;
    int (*read)(void *, uint64_t physical, void *, size_t);
    int (*emit)(void *, uint32_t logical, uint64_t physical, uint32_t count, int written);
};
int ext_linux_extent_walk(struct ext_volume *, uint32_t ino, uint32_t seed,
                          const void *root, size_t, const struct ext_extent_services *);
int ext_linux_directory_check(struct ext_volume *, uint32_t seed, uint32_t flags,
                              uint32_t logical, const void *block);
int ext_linux_directory_entry_check(struct ext_volume *, uint32_t flags,
                                    const void *block, size_t size, unsigned int offset);
uint8_t ext_linux_directory_type(struct ext_volume *, uint8_t filetype);
int verify_dirent_name(const char *, int);
#ifdef __cplusplus
}
#endif
#endif

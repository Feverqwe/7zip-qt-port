// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_UDF_READER_H
#define SEVENZIP_UDF_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef int (*sevenzip_udf_read)(void *, uint16_t, uint32_t, void *, size_t);
typedef int (*sevenzip_udf_extent)(void *, unsigned, uint32_t, uint32_t, uint16_t);
typedef int (*sevenzip_udf_dirent)(void *, uint8_t, const void *, const void *, uint8_t);
struct sevenzip_udf_sparing;
struct sevenzip_udf_virtual;
struct sevenzip_udf_metadata;
struct sevenzip_udf_cached_inode;
typedef struct sevenzip_udf_cached_inode *(*sevenzip_udf_iget)(void *, uint32_t);
typedef uint32_t (*sevenzip_udf_inode_block)(void *, uint16_t, uint32_t);
struct sevenzip_udf_cached_inode *sevenzip_udf_inode_create(void);
void sevenzip_udf_inode_free(struct sevenzip_udf_cached_inode *);
#define UDF_TYPE1_MAP15 0x1511U
#define UDF_VIRTUAL_MAP15 0x1512U
#define UDF_VIRTUAL_MAP20 0x2012U
#define UDF_SPARABLE_MAP15 0x1522U
#define UDF_METADATA_MAP25 0x2511U
struct sevenzip_udf_map {
    uint32_t kind, metadata_file;
    uint16_t volume_sequence, partition_number;
    /* bytes is the available raw prefix, independent of the on-disk map length. */
    uint8_t type, bytes;
    uint8_t data[64];
};
int sevenzip_udf_maps(const void *, uint32_t, struct sevenzip_udf_map **, uint32_t *, uint32_t *);
void sevenzip_udf_maps_free(struct sevenzip_udf_map *);
void sevenzip_udf_map_indices(const struct sevenzip_udf_map *, uint32_t, uint16_t,
                              uint32_t *, uint32_t *);
struct sevenzip_udf_inode {
    uint32_t mode, uid, gid, links, device_major, device_minor, hidden;
};
struct sevenzip_udf_time { int64_t seconds; uint32_t nanoseconds; };
void sevenzip_udf_stamp(const void *, struct sevenzip_udf_time *);
struct sevenzip_udf_file {
    uint64_t size;
    uint16_t partition;
    unsigned inline_data;
    const void *data;
    void *handle;
    uint32_t block, allocation_size;
    unsigned allocation_type, extended, unallocated;
    const void *allocation_data;
};
int sevenzip_udf_directory(void *, sevenzip_udf_read, sevenzip_udf_dirent,
                          uint32_t, uint32_t, const struct sevenzip_udf_file *);
typedef int (*sevenzip_udf_open_file)(void *, uint16_t, uint32_t, struct sevenzip_udf_file *);
typedef int (*sevenzip_udf_read_file)(void *, uint32_t, void *, size_t);
typedef void (*sevenzip_udf_close_file)(void *);
typedef int (*sevenzip_udf_status)(void *);
typedef int (*sevenzip_udf_descriptor)(void *, uint16_t, uint32_t, const void *, size_t,
                                       uint32_t *, uint32_t);
typedef void (*sevenzip_udf_reset)(void *);
struct sevenzip_udf_discovery {
    uint32_t block_size, last_block, anchor_block, flags;
    unsigned seen_anchor;
};
int sevenzip_udf_discover(void *, sevenzip_udf_read, sevenzip_udf_descriptor,
    sevenzip_udf_reset, sevenzip_udf_status, uint64_t, struct sevenzip_udf_discovery *);
int sevenzip_udf_probe(const void *, size_t);
int sevenzip_udf_fileset(void *, sevenzip_udf_read, uint32_t, uint16_t, uint32_t, void *);
int sevenzip_udf_filename(const void *, int, void *, int);
int sevenzip_udf_dstring(const void *, int, void *, int);
int sevenzip_udf_symlink(void *, int, void *, int);
int sevenzip_udf_metadata_create(void *, sevenzip_udf_open_file, sevenzip_udf_read,
    sevenzip_udf_close_file, sevenzip_udf_status, uint32_t, uint32_t, uint16_t, uint16_t, const void *,
    struct sevenzip_udf_metadata **);
void sevenzip_udf_metadata_free(struct sevenzip_udf_metadata *);
int sevenzip_udf_metadata_map(struct sevenzip_udf_metadata *, uint32_t, uint32_t,
                            uint16_t *, uint32_t *);
int sevenzip_udf_virtual_create(void *, sevenzip_udf_open_file, sevenzip_udf_read_file,
    sevenzip_udf_close_file, uint32_t, uint32_t, uint64_t, uint32_t, uint32_t, uint16_t, uint16_t,
    int, struct sevenzip_udf_virtual **);
void sevenzip_udf_virtual_free(struct sevenzip_udf_virtual *);
int sevenzip_udf_virtual_map(struct sevenzip_udf_virtual *, uint32_t, uint32_t,
                            uint16_t *, uint32_t *);
int sevenzip_udf_sparing_create(void *, sevenzip_udf_read, uint32_t, uint32_t,
                                const void *, struct sevenzip_udf_sparing **);
void sevenzip_udf_sparing_free(struct sevenzip_udf_sparing *);
void sevenzip_udf_sparing_root(struct sevenzip_udf_sparing *, uint32_t);
int sevenzip_udf_partition(void *, sevenzip_udf_open_file, sevenzip_udf_close_file,
                           uint32_t, uint32_t, uint32_t, uint32_t, const void *, uint32_t *);
int sevenzip_udf_sparing_span(const struct sevenzip_udf_sparing *, uint32_t,
                              uint32_t *, uint32_t *);
int sevenzip_udf_icb(void *, sevenzip_udf_read, sevenzip_udf_iget,
                     sevenzip_udf_inode_block, uint32_t, const uint32_t *,
                     uint32_t, uint16_t *, uint32_t *, void *, int, struct sevenzip_udf_inode *);
int sevenzip_udf_extents(void *, sevenzip_udf_read, sevenzip_udf_extent,
                         uint32_t, uint32_t, uint16_t, unsigned, int,
                         const void *, uint32_t, uint64_t, uint32_t);
#ifdef __cplusplus
}
#endif
#endif

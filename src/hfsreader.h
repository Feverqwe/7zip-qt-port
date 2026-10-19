// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_HFS_READER_H
#define SEVENZIP_HFS_READER_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int sevenzip_hfsplus_name(const void *big_endian, unsigned length, int xattr,
                         uint16_t *output, unsigned *output_length);
int sevenzip_hfsplus_compare(const void *name_a, const void *name_b, int binary);
int sevenzip_hfsplus_private_name(const void *name, int binary);
struct sevenzip_hfsplus_permissions {
    uint32_t owner, group;
    uint16_t mode;
    uint8_t rootflags, userflags;
};
int hfsplus_get_perms(const void *permissions, int directory, uint32_t uid, uint32_t gid,
                      uint16_t umask, struct sevenzip_hfsplus_permissions *result);
int hfsplus_brec_read_cat(const void *record, size_t length);
int hfsplus_getxattr_finder_info(const void *record, size_t length, void *value, size_t size);
struct sevenzip_hfsplus_record {
    uint32_t id, parent, creation, special, file_type, creator;
    int is_directory;
};
typedef int (*sevenzip_hfsplus_lookup_fn)(void *, uint32_t, const char *,
                                         struct sevenzip_hfsplus_record *, unsigned *);
int hfsplus_lookup(struct sevenzip_hfsplus_record entry, unsigned source,
                   uint32_t hidden_id, uint32_t hidden_creation, uint32_t root_creation,
                   sevenzip_hfsplus_lookup_fn lookup, void *context, unsigned *resolved);
#ifdef __cplusplus
}
#endif
#endif

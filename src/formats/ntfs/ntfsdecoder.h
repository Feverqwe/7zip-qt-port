// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_NTFS_DECODER_H
#define SEVENZIP_NTFS_DECODER_H
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#ifndef EOPNOTSUPP
#define EOPNOTSUPP 95
#endif
#ifdef __cplusplus
extern "C" {
#endif
int sevenzip_ntfs_lznt(const void *, size_t, void *, size_t);
int sevenzip_ntfs_mft_fixup(void *, size_t);
typedef int (*sevenzip_ntfs_read_fn)(void *, uint64_t, void *, size_t);
struct sevenzip_ntfs_wof;
struct sevenzip_ntfs_wof *sevenzip_ntfs_wof_create(uint64_t file_size, uint64_t ads_size,
                                                   unsigned frame_bits, sevenzip_ntfs_read_fn,
                                                   void *);
void sevenzip_ntfs_wof_destroy(struct sevenzip_ntfs_wof *);
int sevenzip_ntfs_wof_read_frame(struct sevenzip_ntfs_wof *, uint64_t frame, void *output);
struct sevenzip_ntfs_list_entry {
    uint64_t ref, vcn;
    uint32_t type;
    uint16_t id;
    uint8_t name_len;
    const void *name;
};
int sevenzip_ntfs_list_next(const void *, size_t, size_t *cursor,
                            struct sevenzip_ntfs_list_entry *);
int sevenzip_ntfs_list_find_unnamed(const void *, size_t, uint32_t,
                                    struct sevenzip_ntfs_list_entry *);
#ifdef __cplusplus
}
#endif
#endif

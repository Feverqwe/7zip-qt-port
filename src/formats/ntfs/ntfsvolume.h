// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_NTFS_VOLUME_H
#define SEVENZIP_NTFS_VOLUME_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sevenzip_ntfs_volume_info {
    uint8_t major, minor;
    int dirty;
};
int sevenzip_ntfs_read_volume_info(const void *, size_t, int, uint16_t,
                                   struct sevenzip_ntfs_volume_info *);
#ifdef __cplusplus
}
#endif
#endif

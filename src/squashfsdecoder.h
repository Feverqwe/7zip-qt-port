// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef SEVENZIP_SQUASHFS_DECODER_H
#define SEVENZIP_SQUASHFS_DECODER_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sevenzip_squashfs_decoder;
typedef int (*sevenzip_squashfs_read)(void *, uint64_t, void *, size_t);
int sevenzip_squashfs_decoder_create(unsigned method, unsigned block_size, unsigned flags,
    uint64_t bytes_used, sevenzip_squashfs_read read, void *opaque,
    struct sevenzip_squashfs_decoder **result);
void sevenzip_squashfs_decoder_destroy(struct sevenzip_squashfs_decoder *);
int sevenzip_squashfs_decode(struct sevenzip_squashfs_decoder *, const void *source,
    unsigned source_size, void *destination, unsigned destination_size);
#ifdef __cplusplus
}
#endif
#endif

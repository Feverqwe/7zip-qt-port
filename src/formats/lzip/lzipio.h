// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_LZIP_IO_H
#define SEVENZIP_LZIP_IO_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sevenzip_lzip_io {
    void *opaque;
    int (*read)(void *, void *, size_t *);
    int (*write)(void *, const void *, size_t);
    int (*progress)(void *, uint64_t, uint64_t);
};
enum sevenzip_lzip_result {
    SEVENZIP_LZIP_OK,
    SEVENZIP_LZIP_DATA_ERROR,
    SEVENZIP_LZIP_UNEXPECTED_END,
    SEVENZIP_LZIP_MEMORY_ERROR,
    SEVENZIP_LZIP_TRAILING_DATA,
    SEVENZIP_LZIP_IO_ERROR
};
int SevenzipLzipDecompress(struct sevenzip_lzip_io *io, uint64_t offset, uint64_t source_size,
                           uint64_t *consumed, uint64_t *produced);
#ifdef __cplusplus
}
#endif
#endif

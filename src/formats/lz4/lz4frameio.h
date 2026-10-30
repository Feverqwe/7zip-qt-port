// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_LZ4_FRAME_IO_H
#define SEVENZIP_LZ4_FRAME_IO_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct sevenzip_lz4_frame_io {
    void *opaque;
    int (*read)(void *, void *, size_t *);
    int (*write)(void *, const void *, size_t);
    int (*seek)(void *, uint64_t);
    int (*progress)(void *, uint64_t, uint64_t);
};
enum sevenzip_lz4_result {
    SEVENZIP_LZ4_OK,
    SEVENZIP_LZ4_DATA_ERROR,
    SEVENZIP_LZ4_UNEXPECTED_END,
    SEVENZIP_LZ4_MEMORY_ERROR,
    SEVENZIP_LZ4_IO_ERROR
};
int LZ4IO_decompressLZ4F(struct sevenzip_lz4_frame_io *io, uint64_t offset, uint64_t source_size,
                         uint64_t *consumed, uint64_t *produced);
#ifdef __cplusplus
}
#endif
#endif

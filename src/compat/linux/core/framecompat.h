// SPDX-License-Identifier: GPL-2.0-only
// Explicit per-operation services for the Linux boot decompressor wrappers.
#ifndef SEVENZIP_FRAME_COMPAT_H
#define SEVENZIP_FRAME_COMPAT_H
#include "formats/initramfs/linuxframe.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#define STATIC static
#define INIT
#define unlikely(x) (x)
static inline u16 get_unaligned_be16(const void *p) {
    const u8 *b = p;
    return (u16)((u16)b[0] << 8 | b[1]);
}
static inline u32 get_unaligned_be32(const void *p) {
    const u8 *b = p;
    return (u32)b[0] << 24 | (u32)b[1] << 16 | (u32)b[2] << 8 | b[3];
}
static inline u32 get_unaligned_le32(const void *p) {
    const u8 *b = p;
    return (u32)b[3] << 24 | (u32)b[2] << 16 | (u32)b[1] << 8 | b[0];
}
uint32_t crc32_le(uint32_t crc, const unsigned char *p, size_t size);
static inline u32 sevenzip_frame_checksum(int crc, const void *data, size_t size) {
    return crc ? crc32_le(UINT32_MAX, data, size) ^ UINT32_MAX : (u32)adler32_z(1, data, size);
}
static inline void *sevenzip_frame_alloc(struct sevenzip_linux_frame_io *io, size_t size) {
    void *p = malloc(size);
    if (!p)
        io->allocation_failed = 1;
    return p;
}
#define malloc(n) sevenzip_frame_alloc(io, n)
#define large_malloc(n) sevenzip_frame_alloc(io, n)
#define large_free(p) free(p)
#define fill(p, n) io->fill(io->opaque, p, n)
#define flush(p, n) io->flush(io->opaque, p, n)
#define error(s) io->error(io->opaque, s)
#endif

// SPDX-License-Identifier: MIT
// Portable memory/endian services; the LZO decoder itself remains Linux C.
#ifndef SEVENZIP_LZO_COMPAT_H
#define SEVENZIP_LZO_COMPAT_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define likely(x) (x)
#define unlikely(x) (x)
#define EXPORT_SYMBOL_GPL(x)
#define MODULE_LICENSE(x)
#define MODULE_DESCRIPTION(x)
static inline uint16_t get_unaligned_le16(const void *p) {
    const uint8_t *b = p;
    return (uint16_t)(b[0] | (uint16_t)b[1] << 8);
}
// The portable scalar branches avoid architecture-specific unaligned accesses.
#endif

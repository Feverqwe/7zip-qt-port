// SPDX-License-Identifier: MIT
// Allocation-free userspace services for the Linux LZ4 decoder.
#ifndef SEVENZIP_LZ4_COMPAT_H
#define SEVENZIP_LZ4_COMPAT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#if UINTPTR_MAX == UINT64_MAX
#define CONFIG_64BIT 1
#endif
#if !defined(__BYTE_ORDER__) || __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define SEVENZIP_LZ4_LITTLE_ENDIAN 1
#else
#define SEVENZIP_LZ4_LITTLE_ENDIAN 0
#endif
#ifdef _MSC_VER
#define SEVENZIP_LZ4_INLINE __forceinline
#define __builtin_memcpy memcpy
#define __builtin_memmove memmove
#else
#define SEVENZIP_LZ4_INLINE inline __attribute__((always_inline))
#endif
#define likely(x) (x)
#define unlikely(x) (x)
#define min(a, b) ((a) < (b) ? (a) : (b))
#define BUILD_BUG_ON(x) ((void)sizeof(char[1 - 2 * !!(x)]))
#define BITS_PER_LONG (sizeof(size_t) * CHAR_BIT)
#define EXPORT_SYMBOL(x)
#define MODULE_LICENSE(x)
#define MODULE_DESCRIPTION(x)
static inline uint64_t sevenzip_lz4_read(const void *p, size_t size) {
    if (size == 2) { uint16_t v; memcpy(&v, p, 2); return v; }
    if (size == 4) { uint32_t v; memcpy(&v, p, 4); return v; }
    uint64_t v; memcpy(&v, p, 8); return v;
}
static inline void sevenzip_lz4_write(uint64_t value, void *p, size_t size) {
    if (size == 2) { uint16_t v = (uint16_t)value; memcpy(p, &v, 2); }
    else if (size == 4) { uint32_t v = (uint32_t)value; memcpy(p, &v, 4); }
    else memcpy(p, &value, 8);
}
#define get_unaligned(p) sevenzip_lz4_read(p, sizeof(*(p)))
#define put_unaligned(v, p) sevenzip_lz4_write(v, p, sizeof(*(p)))
static inline uint16_t get_unaligned_le16(const void *p) {
    const uint8_t *b = p;
    return (uint16_t)(b[0] | (uint16_t)b[1] << 8);
}
static inline void put_unaligned_le16(uint16_t value, void *p) {
    uint8_t *b = p; b[0] = (uint8_t)value; b[1] = (uint8_t)(value >> 8);
}
static inline unsigned int __ffs(size_t value) {
    unsigned int bit = 0;
    while (!(value & 1)) { ++bit; value >>= 1; }
    return bit;
}
static inline unsigned int __fls(size_t value) {
    unsigned int bit = 0;
    while (value >>= 1) ++bit;
    return bit;
}
#endif

// SPDX-License-Identifier: MIT
// Portable types/memory services for the original NTFS3 read decoder.
#ifndef SEVENZIP_NTFS_COMPAT_H
#define SEVENZIP_NTFS_COMPAT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int32_t s32;
typedef uint16_t __le16;
typedef uint32_t __le32;
typedef uint64_t __le64;
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define cpu_to_le16(v) __builtin_bswap16(v)
#define cpu_to_le32(v) __builtin_bswap32(v)
#else
#define cpu_to_le16(v) (v)
#define cpu_to_le32(v) (v)
#endif
#define ALIGN(v, a) (((v) + (a) - 1) & ~((a) - 1))
#ifndef __always_inline
#define __always_inline inline
#endif
#define unlikely(v) (v)
#define GFP_NOFS 0
#define GFP_KERNEL 0
#define kmalloc_obj(type, flags) ((type *)malloc(sizeof(type)))
#define kfree(p) free(p)
#define swap(a, b) do { u32 ntfs_swap_value = (a); (a) = (b); (b) = ntfs_swap_value; } while (0)
#if defined(__GNUC__)
#define fallthrough __attribute__((fallthrough))
#else
#define fallthrough ((void)0)
#endif
static inline size_t ntfs_get_word(const void *p) { size_t v; memcpy(&v, p, sizeof(v)); return v; }
static inline void ntfs_put_word(size_t v, void *p) { memcpy(p, &v, sizeof(v)); }
#define get_unaligned(p) ntfs_get_word(p)
#define put_unaligned(v, p) ntfs_put_word(v, p)
static inline u16 get_unaligned_le16(const void *p) {
    const u8 *b = p; return (u16)(b[0] | (u16)b[1] << 8);
}
static inline u32 get_unaligned_le32(const void *p) {
    const u8 *b = p; return (u32)b[0] | (u32)b[1] << 8 | (u32)b[2] << 16 | (u32)b[3] << 24;
}
static inline u64 get_unaligned_le64(const void *p) {
    const u8 *b = p; return get_unaligned_le32(b) | (u64)get_unaligned_le32(b + 4) << 32;
}
static inline void put_unaligned_le32(u32 v, void *p) {
    u8 *b = p; b[0] = (u8)v; b[1] = (u8)(v >> 8); b[2] = (u8)(v >> 16); b[3] = (u8)(v >> 24);
}
static inline void put_unaligned_le64(u64 v, void *p) {
    put_unaligned_le32((u32)v, p); put_unaligned_le32((u32)(v >> 32), (u8 *)p + 4);
}
#ifdef _MSC_VER
typedef intptr_t ssize_t;
#else
#include <sys/types.h>
#endif
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define PtrOffset(base, p) ((size_t)((const u8 *)(p) - (const u8 *)(base)))
ssize_t decompress_lznt(const void *, size_t, void *, size_t);
#endif

/* SPDX-License-Identifier: GPL-2.0-only */
/* C11 host services for Linux's generic cryptographic library routines. */
#ifndef SEVENZIP_CRYPTO_USERSPACE_H
#define SEVENZIP_CRYPTO_USERSPACE_H
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#ifndef __attribute_const__
#define __attribute_const__
#endif
#if ULONG_MAX == 0xffffffffUL
#define BITS_PER_LONG 32
#else
#define BITS_PER_LONG 64
#endif
#ifndef EKEYREJECTED
#define EKEYREJECTED 129
#endif
#ifdef _MSC_VER
#include <intrin.h>
#define SEVENZIP_CRYPTO_NOINLINE __declspec(noinline)
#define ____cacheline_aligned __declspec(align(64))
#define barrier() _ReadWriteBarrier()
#define OPTIMIZER_HIDE_VAR(var) ((void)*(volatile unsigned long *)&(var))
#else
#define SEVENZIP_CRYPTO_NOINLINE __attribute__((noinline))
#define ____cacheline_aligned __attribute__((aligned(64)))
#define barrier() __asm__ __volatile__("" : : : "memory")
#define OPTIMIZER_HIDE_VAR(var) __asm__ __volatile__("" : "+r"(var))
#endif
#ifndef __always_inline
#define __always_inline inline
#endif
#define __maybe_unused
#define __must_check
#define at_least
#define likely(value) (value)
#define unlikely(value) (value)
static inline bool crypto_warn_on(bool value) { return value; }
#define WARN_ON_ONCE(value) crypto_warn_on(value)
#define EXPORT_SYMBOL(name)
#define EXPORT_SYMBOL_GPL(name)
#define MODULE_DESCRIPTION(value)
#define MODULE_LICENSE(value)
#define L1_CACHE_BYTES 64
#define round_down(n, a) ((n) & ~((size_t)(a) - 1))
#define round_up(n, a) (((n) + (a) - 1) & ~((size_t)(a) - 1))
#define static_assert(e) _Static_assert((e), #e)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define REPEAT_BYTE(v) ((~0UL / 0xff) * (v))
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#define fallthrough ((void)0)
#define fips_enabled 0
#define IS_ENABLED(option) IS_ENABLED_##option
#define IS_ENABLED_CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS 0
#define IS_ENABLED_CONFIG_CRYPTO_LIB_MLDSA_KUNIT_TEST 0
#if UINTPTR_MAX > UINT32_MAX
#define IS_ENABLED_CONFIG_64BIT 1
#else
#define IS_ENABLED_CONFIG_64BIT 0
#endif
typedef uint8_t u8;
typedef uint8_t __u8;
typedef uint16_t u16;
typedef uint16_t __le16;
typedef uint32_t u32;
typedef uint32_t __u32;
typedef uint32_t __le32;
typedef uint32_t __be32;
typedef uint64_t u64, __le64, __be64;
typedef uint64_t __u64;
typedef int64_t s64;
typedef int8_t s8;
typedef int32_t s32;
static inline void memzero_explicit(void *data, size_t size)
{
	volatile u8 *p = data;
	while (size--)
		*p++ = 0;
}
static inline u16 crypto_load_u16(const void *p) { u16 v; memcpy(&v, p, sizeof(v)); return v; }
static inline u32 crypto_load_u32(const void *p) { u32 v; memcpy(&v, p, sizeof(v)); return v; }
static inline u64 crypto_load_u64(const void *p) { u64 v; memcpy(&v, p, sizeof(v)); return v; }
static inline unsigned long crypto_load_ulong(const void *p) { unsigned long v; memcpy(&v, p, sizeof(v)); return v; }
static inline void crypto_store_u16(void *p, u16 v) { memcpy(p, &v, sizeof(v)); }
static inline void crypto_store_u32(void *p, u32 v) { memcpy(p, &v, sizeof(v)); }
static inline void crypto_store_u64(void *p, u64 v) { memcpy(p, &v, sizeof(v)); }
static inline void crypto_store_ulong(void *p, unsigned long v) { memcpy(p, &v, sizeof(v)); }
#define get_unaligned(p) _Generic((p), u16 *: crypto_load_u16, const u16 *: crypto_load_u16, \
	u32 *: crypto_load_u32, const u32 *: crypto_load_u32, u64 *: crypto_load_u64, const u64 *: crypto_load_u64, default: crypto_load_ulong)(p)
#define put_unaligned(v, p) _Generic((p), u16 *: crypto_store_u16, u32 *: crypto_store_u32, u64 *: crypto_store_u64, default: crypto_store_ulong)(p, v)
static inline u32 get_unaligned_le32(const void *p)
{
	const u8 *b = p;
	return (u32)b[0] | (u32)b[1] << 8 | (u32)b[2] << 16 | (u32)b[3] << 24;
}
static inline u16 get_unaligned_le16(const void *p)
{
	const u8 *b = p;
	return (u16)b[0] | (u16)b[1] << 8;
}
static inline __le16 cpu_to_le16(u16 v)
{
	const u8 bytes[2] = {(u8)v, (u8)(v >> 8)};
	__le16 result;
	memcpy(&result, bytes, sizeof(result));
	return result;
}
static inline u64 get_unaligned_le64(const void *p)
{
	return (u64)get_unaligned_le32(p) | (u64)get_unaligned_le32((const u8 *)p + 4) << 32;
}
#define le64_to_cpup(p) get_unaligned_le64(p)
#define le32_to_cpup(p) get_unaligned_le32(p)
#define le16_to_cpup(p) get_unaligned_le16(p)
static inline void put_unaligned_le32(u32 v, void *p)
{
	u8 *b = p;
	for (unsigned i = 0; i < 4; ++i) b[i] = (u8)(v >> (i * 8));
}
static inline __le32 cpu_to_le32(u32 v)
{
	__le32 result;
	put_unaligned_le32(v, &result);
	return result;
}
static inline u64 le64_to_cpu(__le64 v)
{
	const u8 *p = (const u8 *)&v;
	return (u64)get_unaligned_le32(p) | (u64)get_unaligned_le32(p + 4) << 32;
}
static inline __le64 cpu_to_le64(u64 v)
{
	__le64 result;
	u8 *p = (u8 *)&result;
	put_unaligned_le32((u32)v, p);
	put_unaligned_le32((u32)(v >> 32), p + 4);
	return result;
}
static inline void le64_add_cpu(__le64 *p, u64 v) { *p = cpu_to_le64(le64_to_cpu(*p) + v); }
static inline void le64_to_cpus(u64 *p) { *p = le64_to_cpu(*p); }
static inline u32 rol32(u32 v, unsigned n) { return (v << n) | (v >> (32 - n)); }
static inline u32 ror32(u32 v, unsigned n) { return (v >> n) | (v << (32 - n)); }
static inline u64 ror64(u64 v, unsigned n) { return (v >> n) | (v << (64 - n)); }
static inline u64 rol64(u64 v, unsigned n) { return (v << n) | (v >> (64 - n)); }
static inline u32 get_unaligned_be32(const void *p)
{
	const u8 *b = p;
	return (u32)b[3] | (u32)b[2] << 8 | (u32)b[1] << 16 | (u32)b[0] << 24;
}
static inline void put_unaligned_be32(u32 v, void *p)
{
	u8 *b = p;
	for (unsigned i = 0; i < 4; ++i) b[3 - i] = (u8)(v >> (i * 8));
}
static inline u64 get_unaligned_be64(const void *p)
{
	const u8 *b = p;
	u64 result = 0;
	for (unsigned i = 0; i < 8; ++i) result = result << 8 | b[i];
	return result;
}
static inline void put_unaligned_be64(u64 v, void *p)
{
	u8 *b = p;
	for (unsigned i = 0; i < 8; ++i) b[7 - i] = (u8)(v >> (i * 8));
}
static inline __be64 cpu_to_be64(u64 v)
{
	__be64 result;
	put_unaligned_be64(v, &result);
	return result;
}
static inline __be32 cpu_to_be32(u32 v)
{
	__be32 result;
	u8 *b = (u8 *)&result;
	for (unsigned i = 0; i < 4; ++i) b[3 - i] = (u8)(v >> (i * 8));
	return result;
}
static inline bool check_add_overflow(u64 a, u64 b, u64 *result)
{
	*result = a + b;
	return *result < a;
}
static inline unsigned __ffs(unsigned long v)
{
	unsigned n = 0;
	while (!(v & 1)) { v >>= 1; ++n; }
	return n;
}
#endif

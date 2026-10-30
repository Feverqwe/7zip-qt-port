// SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause)
/* Copyright (C) 2016-2022 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
 *
 * SipHash: a fast short-input PRF
 * https://131002.net/siphash/
 *
 * This implementation is specifically for SipHash2-4 for a secure PRF
 * and HalfSipHash1-3/SipHash1-3 for an insecure PRF only suitable for
 * hashtables.
 */

#include <linux/siphash.h>

#if defined(CONFIG_DCACHE_WORD_ACCESS) && BITS_PER_LONG == 64
#include <linux/dcache.h>
#include <asm/word-at-a-time.h>
#endif

#define SIPROUND SIPHASH_PERMUTATION(v0, v1, v2, v3)

#define PREAMBLE(len) \
	u64 v0 = SIPHASH_CONST_0; \
	u64 v1 = SIPHASH_CONST_1; \
	u64 v2 = SIPHASH_CONST_2; \
	u64 v3 = SIPHASH_CONST_3; \
	u64 b = ((u64)(len)) << 56; \
	v3 ^= key->key[1]; \
	v2 ^= key->key[0]; \
	v1 ^= key->key[1]; \
	v0 ^= key->key[0];

#define POSTAMBLE \
	v3 ^= b; \
	SIPROUND; \
	SIPROUND; \
	v0 ^= b; \
	v2 ^= 0xff; \
	SIPROUND; \
	SIPROUND; \
	SIPROUND; \
	SIPROUND; \
	return (v0 ^ v1) ^ (v2 ^ v3);

#ifndef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
u64 __siphash_aligned(const void *data, size_t len, const siphash_key_t *key)
{
	const u8 *end = (const u8 *)data + len - (len % sizeof(u64));
	const u8 left = len & (sizeof(u64) - 1);
	u64 m;
	PREAMBLE(len)
	for (; data != end; data = (const u8 *)data + sizeof(u64)) {
		m = le64_to_cpup(data);
		v3 ^= m;
		SIPROUND;
		SIPROUND;
		v0 ^= m;
	}
#if defined(CONFIG_DCACHE_WORD_ACCESS) && BITS_PER_LONG == 64
	if (left)
		b |= le64_to_cpu((__force __le64)(load_unaligned_zeropad(data) &
						  bytemask_from_count(left)));
#else
	switch (left) {
	case 7: b |= ((u64)end[6]) << 48; fallthrough;
	case 6: b |= ((u64)end[5]) << 40; fallthrough;
	case 5: b |= ((u64)end[4]) << 32; fallthrough;
	case 4: b |= le32_to_cpup(data); break;
	case 3: b |= ((u64)end[2]) << 16; fallthrough;
	case 2: b |= le16_to_cpup(data); break;
	case 1: b |= end[0];
	}
#endif
	POSTAMBLE
}
EXPORT_SYMBOL(__siphash_aligned);
#endif

u64 __siphash_unaligned(const void *data, size_t len, const siphash_key_t *key)
{
	const u8 *end = (const u8 *)data + len - (len % sizeof(u64));
	const u8 left = len & (sizeof(u64) - 1);
	u64 m;
	PREAMBLE(len)
	for (; data != end; data = (const u8 *)data + sizeof(u64)) {
		m = get_unaligned_le64(data);
		v3 ^= m;
		SIPROUND;
		SIPROUND;
		v0 ^= m;
	}
#if defined(CONFIG_DCACHE_WORD_ACCESS) && BITS_PER_LONG == 64
	if (left)
		b |= le64_to_cpu((__force __le64)(load_unaligned_zeropad(data) &
						  bytemask_from_count(left)));
#else
	switch (left) {
	case 7: b |= ((u64)end[6]) << 48; fallthrough;
	case 6: b |= ((u64)end[5]) << 40; fallthrough;
	case 5: b |= ((u64)end[4]) << 32; fallthrough;
	case 4: b |= get_unaligned_le32(end); break;
	case 3: b |= ((u64)end[2]) << 16; fallthrough;
	case 2: b |= get_unaligned_le16(end); break;
	case 1: b |= end[0];
	}
#endif
	POSTAMBLE
}
EXPORT_SYMBOL(__siphash_unaligned);

/**
 * siphash_1u64 - compute 64-bit siphash PRF value of a u64
 * @first: first u64
 * @key: the siphash key
 */
u64 siphash_1u64(const u64 first, const siphash_key_t *key)
{
	PREAMBLE(8)
	v3 ^= first;
	SIPROUND;
	SIPROUND;
	v0 ^= first;
	POSTAMBLE
}
EXPORT_SYMBOL(siphash_1u64);

/**
 * siphash_2u64 - compute 64-bit siphash PRF value of 2 u64
 * @first: first u64
 * @second: second u64
 * @key: the siphash key
 */
u64 siphash_2u64(const u64 first, const u64 second, const siphash_key_t *key)
{
	PREAMBLE(16)
	v3 ^= first;
	SIPROUND;
	SIPROUND;
	v0 ^= first;
	v3 ^= second;
	SIPROUND;
	SIPROUND;
	v0 ^= second;
	POSTAMBLE
}
EXPORT_SYMBOL(siphash_2u64);

/**
 * siphash_3u64 - compute 64-bit siphash PRF value of 3 u64
 * @first: first u64
 * @second: second u64
 * @third: third u64
 * @key: the siphash key
 */
u64 siphash_3u64(const u64 first, const u64 second, const u64 third,
		 const siphash_key_t *key)
{
	PREAMBLE(24)
	v3 ^= first;
	SIPROUND;
	SIPROUND;
	v0 ^= first;
	v3 ^= second;
	SIPROUND;
	SIPROUND;
	v0 ^= second;
	v3 ^= third;
	SIPROUND;
	SIPROUND;
	v0 ^= third;
	POSTAMBLE
}
EXPORT_SYMBOL(siphash_3u64);

/**
 * siphash_4u64 - compute 64-bit siphash PRF value of 4 u64
 * @first: first u64
 * @second: second u64
 * @third: third u64
 * @forth: forth u64
 * @key: the siphash key
 */
u64 siphash_4u64(const u64 first, const u64 second, const u64 third,
		 const u64 forth, const siphash_key_t *key)
{
	PREAMBLE(32)
	v3 ^= first;
	SIPROUND;
	SIPROUND;
	v0 ^= first;
	v3 ^= second;
	SIPROUND;
	SIPROUND;
	v0 ^= second;
	v3 ^= third;
	SIPROUND;
	SIPROUND;
	v0 ^= third;
	v3 ^= forth;
	SIPROUND;
	SIPROUND;
	v0 ^= forth;
	POSTAMBLE
}
EXPORT_SYMBOL(siphash_4u64);

u64 siphash_1u32(const u32 first, const siphash_key_t *key)
{
	PREAMBLE(4)
	b |= first;
	POSTAMBLE
}
EXPORT_SYMBOL(siphash_1u32);

u64 siphash_3u32(const u32 first, const u32 second, const u32 third,
		 const siphash_key_t *key)
{
	u64 combined = (u64)second << 32 | first;
	PREAMBLE(12)
	v3 ^= combined;
	SIPROUND;
	SIPROUND;
	v0 ^= combined;
	b |= third;
	POSTAMBLE
}
EXPORT_SYMBOL(siphash_3u32);

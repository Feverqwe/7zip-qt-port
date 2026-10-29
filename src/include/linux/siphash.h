/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/* Copyright (C) 2016-2022 Jason A. Donenfeld <Jason@zx2c4.com>. All Rights Reserved.
 *
 * SipHash: a fast short-input PRF
 * https://131002.net/siphash/
 *
 * This implementation is specifically for SipHash2-4 for a secure PRF
 * and HalfSipHash1-3/SipHash1-3 for an insecure PRF only suitable for
 * hashtables.
 */

#ifndef _LINUX_SIPHASH_H
#define _LINUX_SIPHASH_H

#include "../../lib/crypto/userspace.h"

#define SIPHASH_ALIGNMENT _Alignof(u64)
typedef struct {
    u64 key[2];
} siphash_key_t;

static inline bool siphash_key_is_zero(const siphash_key_t *key) {
    return !(key->key[0] | key->key[1]);
}

u64 __siphash_aligned(const void *data, size_t len, const siphash_key_t *key);
u64 __siphash_unaligned(const void *data, size_t len, const siphash_key_t *key);

u64 siphash_1u64(const u64 a, const siphash_key_t *key);
u64 siphash_2u64(const u64 a, const u64 b, const siphash_key_t *key);
u64 siphash_3u64(const u64 a, const u64 b, const u64 c, const siphash_key_t *key);
u64 siphash_4u64(const u64 a, const u64 b, const u64 c, const u64 d, const siphash_key_t *key);
u64 siphash_1u32(const u32 a, const siphash_key_t *key);
u64 siphash_3u32(const u32 a, const u32 b, const u32 c, const siphash_key_t *key);

static inline u64 siphash_2u32(const u32 a, const u32 b, const siphash_key_t *key) {
    return siphash_1u64((u64)b << 32 | a, key);
}
static inline u64 siphash_4u32(const u32 a, const u32 b, const u32 c, const u32 d,
                               const siphash_key_t *key) {
    return siphash_2u64((u64)b << 32 | a, (u64)d << 32 | c, key);
}

/* Archive buffers use the original alignment-independent path. */
static inline u64 siphash(const void *data, size_t len, const siphash_key_t *key) {
    return __siphash_unaligned(data, len, key);
}

#define SIPHASH_PERMUTATION(a, b, c, d)                                                            \
    ((a) += (b), (b) = rol64((b), 13), (b) ^= (a), (a) = rol64((a), 32), (c) += (d),               \
     (d) = rol64((d), 16), (d) ^= (c), (a) += (d), (d) = rol64((d), 21), (d) ^= (a), (c) += (b),   \
     (b) = rol64((b), 17), (b) ^= (c), (c) = rol64((c), 32))

#define SIPHASH_CONST_0 0x736f6d6570736575ULL
#define SIPHASH_CONST_1 0x646f72616e646f6dULL
#define SIPHASH_CONST_2 0x6c7967656e657261ULL
#define SIPHASH_CONST_3 0x7465646279746573ULL

#endif /* _LINUX_SIPHASH_H */

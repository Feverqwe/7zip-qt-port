/* SPDX-License-Identifier: GPL-2.0-only */
/* Owned workspace and scalar services for Linux's ML-DSA verification body. */
#ifndef SEVENZIP_MLDSA_READ_H
#define SEVENZIP_MLDSA_READ_H
#include "crypto/modes/skciphercompat.h"
#ifndef EKEYREJECTED
#define EKEYREJECTED 129
#endif
#define kmalloc(size, flags) sevenzip_crypto_zalloc(size)
void *memchr_inv(const void *, int, size_t);
static inline bool mem_is_zero(const void *s, size_t n) { return !memchr_inv(s, 0, n); }
#endif

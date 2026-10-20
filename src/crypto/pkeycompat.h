/* SPDX-License-Identifier: GPL-2.0-only */
/* Owned, synchronous Crypto API services for original public-key providers. */
#ifndef SEVENZIP_PKEY_COMPAT_H
#define SEVENZIP_PKEY_COMPAT_H
#include "skciphercompat.h"
#include <crypto/utils.h>
#include <linux/asn1_decoder.h>
#include <linux/asn1.h>
#include <stdio.h>
#ifndef ENOPKG
#define ENOPKG 65
#endif
#ifndef ENOTSUPP
#define ENOTSUPP 524
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP 95
#endif
#define CRYPTO_MAX_ALG_NAME 128
#define BITS_PER_BYTE 8
#define PAGE_SIZE SEVENZIP_CRYPTO_PAGE_SIZE
#define THIS_MODULE NULL
#ifndef kmalloc
#define kmalloc(size, flags) sevenzip_crypto_zalloc(size)
#endif
#ifndef kfree
#define kfree(ptr) sevenzip_crypto_free_sensitive(ptr)
#endif
#define pr_devel(...) ((void)0)
#define pr_debug(...) ((void)0)
#define pr_err(...) ((void)0)
#define BUG_ON(value) do { if (value) abort(); } while (0)
#define WARN_ON_ONCE(value) crypto_warn_on(value)

/* Scalar algorithm metadata; module discovery and hardware dispatch are
 * replaced by the fixed process-owned provider registry. */
struct crypto_alg {
	char cra_name[CRYPTO_MAX_ALG_NAME];
	char cra_driver_name[CRYPTO_MAX_ALG_NAME];
	unsigned cra_priority;
	size_t cra_ctxsize;
	u32 cra_flags;
	const void *cra_module;
};
#endif

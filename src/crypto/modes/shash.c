// SPDX-License-Identifier: GPL-2.0-or-later
/* Synchronous hash read path from Linux crypto/shash.c.
 * Copyright (c) 2008 Herbert Xu <herbert@gondor.apana.org.au>
 */
#include <crypto/hash.h>

static inline bool crypto_shash_block_only(struct crypto_shash *tfm)
{
	return crypto_shash_alg(tfm)->base.cra_flags &
	       CRYPTO_AHASH_ALG_BLOCK_ONLY;
}

static inline bool crypto_shash_final_nonzero(struct crypto_shash *tfm)
{
	return crypto_shash_alg(tfm)->base.cra_flags &
	       CRYPTO_AHASH_ALG_FINAL_NONZERO;
}

static inline bool crypto_shash_finup_max(struct crypto_shash *tfm)
{
	return crypto_shash_alg(tfm)->base.cra_flags &
	       CRYPTO_AHASH_ALG_FINUP_MAX;
}

static int __crypto_shash_init(struct shash_desc *desc)
{
	struct crypto_shash *tfm = desc->tfm;

	if (crypto_shash_block_only(tfm)) {
		u8 *buf = shash_desc_ctx(desc);

		buf += crypto_shash_descsize(tfm) - 1;
		*buf = 0;
	}

	return crypto_shash_alg(tfm)->init(desc);
}

int crypto_shash_init(struct shash_desc *desc)
{
	if (crypto_shash_get_flags(desc->tfm) & CRYPTO_TFM_NEED_KEY)
		return -ENOKEY;
	return __crypto_shash_init(desc);
}
EXPORT_SYMBOL_GPL(crypto_shash_init);

int shash_default_finup(struct shash_desc *desc, const u8 *data,
			       unsigned int len, u8 *out)
{
	struct shash_alg *shash = crypto_shash_alg(desc->tfm);

	int err = shash->update(desc, data, len);
	return err ? err : shash->final(desc, out);
}

static int crypto_shash_op_and_zero(
	int (*op)(struct shash_desc *desc, const u8 *data,
		  unsigned int len, u8 *out),
	struct shash_desc *desc, const u8 *data, unsigned int len, u8 *out)
{
	int err;

	err = op(desc, data, len, out);
	memset(shash_desc_ctx(desc), 0, crypto_shash_descsize(desc->tfm));
	return err;
}

int crypto_shash_finup(struct shash_desc *restrict desc, const u8 *data,
		       unsigned int len, u8 *restrict out)
{
	struct crypto_shash *tfm = desc->tfm;
	u8 *blenp = shash_desc_ctx(desc);
	bool finup_max, nonzero;
	unsigned int bs;
	int err;
	u8 *buf;

	if (!crypto_shash_block_only(tfm)) {
		if (out)
			goto finup;
		return crypto_shash_alg(tfm)->update(desc, data, len);
	}

	finup_max = out && crypto_shash_finup_max(tfm);

	/* Retain extra block for final nonzero algorithms. */
	nonzero = crypto_shash_final_nonzero(tfm);

	/*
	 * The partial block buffer follows the algorithm desc context.
	 * The byte following that contains the length.
	 */
	blenp += crypto_shash_descsize(tfm) - 1;
	bs = crypto_shash_blocksize(tfm);
	buf = blenp - bs;

	if (likely(!*blenp && finup_max))
		goto finup;

	while ((*blenp + len) >= bs + nonzero) {
		unsigned int nbytes = len - nonzero;
		const u8 *src = data;

		if (*blenp) {
			memcpy(buf + *blenp, data, bs - *blenp);
			nbytes = bs;
			src = buf;
		}

		err = crypto_shash_alg(tfm)->update(desc, src, nbytes);
		if (err < 0)
			return err;

		data += nbytes - err - *blenp;
		len -= nbytes - err - *blenp;
		*blenp = 0;
	}

	if (*blenp || !out) {
		if (len)
			memcpy(buf + *blenp, data, len);
		*blenp += len;
		if (!out)
			return 0;
		data = buf;
		len = *blenp;
	}

finup:
	return crypto_shash_op_and_zero(crypto_shash_alg(tfm)->finup, desc,
					data, len, out);
}
EXPORT_SYMBOL_GPL(crypto_shash_finup);

int shash_default_digest(struct shash_desc *desc, const u8 *data,
				unsigned int len, u8 *out)
{
	int err = __crypto_shash_init(desc);
	return err ? err : crypto_shash_finup(desc, data, len, out);
}

int crypto_shash_digest(struct shash_desc *desc, const u8 *data,
			unsigned int len, u8 *out)
{
	struct crypto_shash *tfm = desc->tfm;

	if (crypto_shash_get_flags(tfm) & CRYPTO_TFM_NEED_KEY)
		return -ENOKEY;

	return crypto_shash_op_and_zero(crypto_shash_alg(tfm)->digest, desc,
					data, len, out);
}
EXPORT_SYMBOL_GPL(crypto_shash_digest);

int crypto_shash_tfm_digest(struct crypto_shash *tfm, const u8 *data,
			    unsigned int len, u8 *out)
{
	SHASH_DESC_ON_STACK(desc, tfm);

	desc->tfm = tfm;
	return crypto_shash_digest(desc, data, len, out);
}
EXPORT_SYMBOL_GPL(crypto_shash_tfm_digest);

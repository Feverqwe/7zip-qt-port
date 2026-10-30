/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Public Key Signature Algorithm: default callbacks and owned context services.
 * Copyright (c) 2023 Herbert Xu <herbert@gondor.apana.org.au> */
#include <crypto/internal/sig.h>
#define DIV_ROUND_UP_POW2(n, d) DIV_ROUND_UP(n, d)
struct sig_alg *crypto_sig_alg(struct crypto_sig *tfm) { return &tfm->instance->alg; }

static int sig_default_sign(struct crypto_sig *tfm,
			    const void *src, unsigned int slen,
			    void *dst, unsigned int dlen)
{
	return -ENOSYS;
}

static int sig_default_verify(struct crypto_sig *tfm,
			      const void *src, unsigned int slen,
			      const void *dst, unsigned int dlen)
{
	return -ENOSYS;
}

static int sig_default_set_key(struct crypto_sig *tfm,
			       const void *key, unsigned int keylen)
{
	return -ENOSYS;
}

static unsigned int sig_default_size(struct crypto_sig *tfm)
{
	return DIV_ROUND_UP_POW2(crypto_sig_keysize(tfm), BITS_PER_BYTE);
}

struct crypto_sig *sevenzip_sig_alloc(const struct sig_alg *algorithm, size_t instance_size)
{
	struct crypto_sig *tfm = sevenzip_crypto_zalloc(sizeof(*tfm));
	if (!tfm) return ERR_PTR(-ENOMEM);
	tfm->instance = sevenzip_crypto_zalloc(sizeof(*tfm->instance));
	if (!tfm->instance) { sevenzip_crypto_free_sensitive(tfm); return ERR_PTR(-ENOMEM); }
	tfm->instance->alg = *algorithm;
	tfm->instance->ctx = sevenzip_crypto_zalloc(instance_size);
	tfm->ctx = sevenzip_crypto_zalloc(algorithm->base.cra_ctxsize);
	if (!tfm->instance->ctx || !tfm->ctx) {
		crypto_free_sig(tfm);
		return ERR_PTR(-ENOMEM);
	}
	/* Selected sig_prepare_alg defaults, on this owned immutable instance. */
	struct sig_alg *alg = crypto_sig_alg(tfm);
	if (!alg->sign) alg->sign = sig_default_sign;
	if (!alg->verify) alg->verify = sig_default_verify;
	if (!alg->set_priv_key) alg->set_priv_key = sig_default_set_key;
	if (!alg->max_size) alg->max_size = sig_default_size;
	if (!alg->digest_size) alg->digest_size = sig_default_size;
	return tfm;
}
int sevenzip_sig_finish_init(struct crypto_sig *tfm)
{
	int err = crypto_sig_alg(tfm)->init ? crypto_sig_alg(tfm)->init(tfm) : 0;
	if (!err) tfm->initialized = true;
	return err;
}
void crypto_free_sig(struct crypto_sig *tfm)
{
	if (!tfm || IS_ERR(tfm)) return;
	if (tfm->initialized && crypto_sig_alg(tfm)->exit) crypto_sig_alg(tfm)->exit(tfm);
	sevenzip_crypto_free_sensitive(tfm->ctx);
	sevenzip_crypto_free_sensitive(tfm->instance->ctx);
	sevenzip_crypto_free_sensitive(tfm->instance);
	sevenzip_crypto_free_sensitive(tfm);
}

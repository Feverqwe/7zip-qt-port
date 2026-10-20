// SPDX-License-Identifier: GPL-2.0
/*
 * ECDSA P1363 signature encoding
 *
 * Copyright (c) 2024 Intel Corporation
 */

#include <crypto/internal/ecc.h>
#include <crypto/internal/sig.h>

struct ecdsa_p1363_ctx {
	struct crypto_sig *child;
};

static int ecdsa_p1363_verify(struct crypto_sig *tfm,
			      const void *src, unsigned int slen,
			      const void *digest, unsigned int dlen)
{
	struct ecdsa_p1363_ctx *ctx = crypto_sig_ctx(tfm);
	unsigned int keylen = DIV_ROUND_UP_POW2(crypto_sig_keysize(ctx->child),
						BITS_PER_BYTE);
	unsigned int ndigits = DIV_ROUND_UP_POW2(keylen, sizeof(u64));
	struct ecdsa_raw_sig sig;

	if (slen != 2 * keylen)
		return -EINVAL;

	ecc_digits_from_bytes(src, keylen, sig.r, ndigits);
	ecc_digits_from_bytes((const u8 *)src + keylen, keylen, sig.s, ndigits);

	return crypto_sig_verify(ctx->child, &sig, sizeof(sig), digest, dlen);
}

static unsigned int ecdsa_p1363_key_size(struct crypto_sig *tfm)
{
	struct ecdsa_p1363_ctx *ctx = crypto_sig_ctx(tfm);

	return crypto_sig_keysize(ctx->child);
}

static unsigned int ecdsa_p1363_max_size(struct crypto_sig *tfm)
{
	struct ecdsa_p1363_ctx *ctx = crypto_sig_ctx(tfm);

	return 2 * DIV_ROUND_UP_POW2(crypto_sig_keysize(ctx->child),
				     BITS_PER_BYTE);
}

static unsigned int ecdsa_p1363_digest_size(struct crypto_sig *tfm)
{
	struct ecdsa_p1363_ctx *ctx = crypto_sig_ctx(tfm);

	return crypto_sig_digestsize(ctx->child);
}

static int ecdsa_p1363_set_pub_key(struct crypto_sig *tfm,
				   const void *key, unsigned int keylen)
{
	struct ecdsa_p1363_ctx *ctx = crypto_sig_ctx(tfm);

	return crypto_sig_set_pubkey(ctx->child, key, keylen);
}

static int ecdsa_p1363_init_tfm(struct crypto_sig *tfm)
{
	struct sig_instance *inst = sig_alg_instance(tfm);
	const char *child_name = sig_instance_ctx(inst);
	struct ecdsa_p1363_ctx *ctx = crypto_sig_ctx(tfm);
	struct crypto_sig *child_tfm;

	child_tfm = crypto_alloc_sig(child_name, 0, 0);
	if (IS_ERR(child_tfm))
		return PTR_ERR(child_tfm);

	ctx->child = child_tfm;

	return 0;
}

static void ecdsa_p1363_exit_tfm(struct crypto_sig *tfm)
{
	struct ecdsa_p1363_ctx *ctx = crypto_sig_ctx(tfm);

	crypto_free_sig(ctx->child);
}

/* Fixed owned construction replaces template/module registration. */
struct crypto_sig *sevenzip_ecdsa_p1363_alloc(const char *child_name)
{
	if (strncmp(child_name, "ecdsa-nist-", 11)) return ERR_PTR(-EINVAL);
	struct sig_alg algorithm = {
		.verify = ecdsa_p1363_verify,
		.set_pub_key = ecdsa_p1363_set_pub_key,
		.key_size = ecdsa_p1363_key_size,
		.max_size = ecdsa_p1363_max_size,
		.digest_size = ecdsa_p1363_digest_size,
		.init = ecdsa_p1363_init_tfm,
		.exit = ecdsa_p1363_exit_tfm,
		.base = { .cra_ctxsize = sizeof(struct ecdsa_p1363_ctx) },
	};
	size_t size = strlen(child_name) + 1;
	struct crypto_sig *tfm = sevenzip_sig_alloc(&algorithm, size);
	if (IS_ERR(tfm)) return tfm;
	memcpy(sig_instance_ctx(sig_alg_instance(tfm)), child_name, size);
	int err = sevenzip_sig_finish_init(tfm);
	if (err) { crypto_free_sig(tfm); return ERR_PTR(err); }
	return tfm;
}

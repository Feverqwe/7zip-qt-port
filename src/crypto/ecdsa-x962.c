// SPDX-License-Identifier: GPL-2.0+
/*
 * ECDSA X9.62 signature encoding
 *
 * Copyright (c) 2021 IBM Corporation
 * Copyright (c) 2024 Intel Corporation
 */

#include <crypto/internal/ecc.h>
#include <crypto/internal/sig.h>
#include "ecdsasignature.asn1.h"

struct ecdsa_x962_ctx {
	struct crypto_sig *child;
};

struct ecdsa_x962_signature_ctx {
	struct ecdsa_raw_sig sig;
	unsigned int ndigits;
};

/* Get the r and s components of a signature from the X.509 certificate. */
static int ecdsa_get_signature_rs(u64 *dest, size_t hdrlen, unsigned char tag,
				  const void *value, size_t vlen,
				  unsigned int ndigits)
{
	size_t bufsize = ndigits * sizeof(u64);
	const u8 *d = value;

	if (!value || !vlen || vlen > bufsize + 1)
		return -EINVAL;

	/*
	 * vlen may be 1 byte larger than bufsize due to a leading zero byte
	 * (necessary if the most significant bit of the integer is set).
	 */
	if (vlen > bufsize) {
		/* skip over leading zeros that make 'value' a positive int */
		if (*d == 0) {
			vlen -= 1;
			d++;
		} else {
			return -EINVAL;
		}
	}

	ecc_digits_from_bytes(d, vlen, dest, ndigits);

	return 0;
}

int ecdsa_get_signature_r(void *context, size_t hdrlen, unsigned char tag,
			  const void *value, size_t vlen)
{
	struct ecdsa_x962_signature_ctx *sig_ctx = context;

	return ecdsa_get_signature_rs(sig_ctx->sig.r, hdrlen, tag, value, vlen,
				      sig_ctx->ndigits);
}

int ecdsa_get_signature_s(void *context, size_t hdrlen, unsigned char tag,
			  const void *value, size_t vlen)
{
	struct ecdsa_x962_signature_ctx *sig_ctx = context;

	return ecdsa_get_signature_rs(sig_ctx->sig.s, hdrlen, tag, value, vlen,
				      sig_ctx->ndigits);
}

static int ecdsa_x962_verify(struct crypto_sig *tfm,
			     const void *src, unsigned int slen,
			     const void *digest, unsigned int dlen)
{
	struct ecdsa_x962_ctx *ctx = crypto_sig_ctx(tfm);
	struct ecdsa_x962_signature_ctx sig_ctx;
	int err;

	sig_ctx.ndigits = DIV_ROUND_UP_POW2(crypto_sig_keysize(ctx->child),
					    sizeof(u64) * BITS_PER_BYTE);

	err = asn1_ber_decoder(&ecdsasignature_decoder, &sig_ctx, src, slen);
	if (err < 0)
		return err;

	return crypto_sig_verify(ctx->child, &sig_ctx.sig, sizeof(sig_ctx.sig),
				 digest, dlen);
}

static unsigned int ecdsa_x962_key_size(struct crypto_sig *tfm)
{
	struct ecdsa_x962_ctx *ctx = crypto_sig_ctx(tfm);

	return crypto_sig_keysize(ctx->child);
}

static unsigned int ecdsa_x962_max_size(struct crypto_sig *tfm)
{
	struct ecdsa_x962_ctx *ctx = crypto_sig_ctx(tfm);
	struct sig_alg *alg = crypto_sig_alg(ctx->child);
	int slen = DIV_ROUND_UP_POW2(crypto_sig_keysize(ctx->child),
				     BITS_PER_BYTE);

	/*
	 * Verify takes ECDSA-Sig-Value (described in RFC 5480) as input,
	 * which is actually 2 'key_size'-bit integers encoded in ASN.1.
	 * Account for the ASN.1 encoding overhead here.
	 *
	 * NIST P192/256/384 may prepend a '0' to a coordinate to indicate
	 * a positive integer. NIST P521 never needs it.
	 */
	if (strcmp(alg->base.cra_name, "ecdsa-nist-p521") != 0)
		slen += 1;

	/* Length of encoding the x & y coordinates */
	slen = 2 * (slen + 2);

	/*
	 * If coordinate encoding takes at least 128 bytes then an
	 * additional byte for length encoding is needed.
	 */
	return 1 + (slen >= 128) + 1 + slen;
}

static unsigned int ecdsa_x962_digest_size(struct crypto_sig *tfm)
{
	struct ecdsa_x962_ctx *ctx = crypto_sig_ctx(tfm);

	return crypto_sig_digestsize(ctx->child);
}

static int ecdsa_x962_set_pub_key(struct crypto_sig *tfm,
				  const void *key, unsigned int keylen)
{
	struct ecdsa_x962_ctx *ctx = crypto_sig_ctx(tfm);

	return crypto_sig_set_pubkey(ctx->child, key, keylen);
}

static int ecdsa_x962_init_tfm(struct crypto_sig *tfm)
{
	struct sig_instance *inst = sig_alg_instance(tfm);
	const char *child_name = sig_instance_ctx(inst);
	struct ecdsa_x962_ctx *ctx = crypto_sig_ctx(tfm);
	struct crypto_sig *child_tfm;

	child_tfm = crypto_alloc_sig(child_name, 0, 0);
	if (IS_ERR(child_tfm))
		return PTR_ERR(child_tfm);

	ctx->child = child_tfm;

	return 0;
}

static void ecdsa_x962_exit_tfm(struct crypto_sig *tfm)
{
	struct ecdsa_x962_ctx *ctx = crypto_sig_ctx(tfm);

	crypto_free_sig(ctx->child);
}

/* Fixed owned construction replaces template/module registration. */
struct crypto_sig *sevenzip_ecdsa_x962_alloc(const char *child_name)
{
	if (strncmp(child_name, "ecdsa-nist-", 11)) return ERR_PTR(-EINVAL);
	struct sig_alg algorithm = {
		.verify = ecdsa_x962_verify,
		.set_pub_key = ecdsa_x962_set_pub_key,
		.key_size = ecdsa_x962_key_size,
		.max_size = ecdsa_x962_max_size,
		.digest_size = ecdsa_x962_digest_size,
		.init = ecdsa_x962_init_tfm,
		.exit = ecdsa_x962_exit_tfm,
		.base = { .cra_ctxsize = sizeof(struct ecdsa_x962_ctx) },
	};
	size_t size = strlen(child_name) + 1;
	struct crypto_sig *tfm = sevenzip_sig_alloc(&algorithm, size);
	if (IS_ERR(tfm)) return tfm;
	memcpy(sig_instance_ctx(sig_alg_instance(tfm)), child_name, size);
	int err = sevenzip_sig_finish_init(tfm);
	if (err) { crypto_free_sig(tfm); return ERR_PTR(err); }
	return tfm;
}

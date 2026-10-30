// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * CBC: Cipher Block Chaining mode
 *
 * Copyright (c) 2006-2016 Herbert Xu <herbert@gondor.apana.org.au>
 */

#include "crypto/modes/skciphercompat.h"

static int crypto_cbc_encrypt_segment(struct crypto_lskcipher *tfm,
				      const u8 *src, u8 *dst, unsigned nbytes,
				      u8 *iv)
{
	unsigned int bsize = crypto_lskcipher_blocksize(tfm);

	for (; nbytes >= bsize; src += bsize, dst += bsize, nbytes -= bsize) {
		crypto_xor(iv, src, bsize);
		crypto_lskcipher_encrypt(tfm, iv, dst, bsize, NULL);
		memcpy(iv, dst, bsize);
	}

	return nbytes;
}

static int crypto_cbc_encrypt_inplace(struct crypto_lskcipher *tfm,
				      u8 *src, unsigned nbytes, u8 *oiv)
{
	unsigned int bsize = crypto_lskcipher_blocksize(tfm);
	u8 *iv = oiv;

	if (nbytes < bsize)
		goto out;

	do {
		crypto_xor(src, iv, bsize);
		crypto_lskcipher_encrypt(tfm, src, src, bsize, NULL);
		iv = src;

		src += bsize;
	} while ((nbytes -= bsize) >= bsize);

	memcpy(oiv, iv, bsize);

out:
	return nbytes;
}

static int crypto_cbc_encrypt(struct crypto_lskcipher *tfm, const u8 *src,
			      u8 *dst, unsigned len, u8 *iv, u32 flags)
{
	struct crypto_lskcipher **ctx = crypto_lskcipher_ctx(tfm);
	bool final = flags & CRYPTO_LSKCIPHER_FLAG_FINAL;
	struct crypto_lskcipher *cipher = *ctx;
	int rem;

	if (src == dst)
		rem = crypto_cbc_encrypt_inplace(cipher, dst, len, iv);
	else
		rem = crypto_cbc_encrypt_segment(cipher, src, dst, len, iv);

	return rem && final ? -EINVAL : rem;
}

static int crypto_cbc_decrypt_segment(struct crypto_lskcipher *tfm,
				      const u8 *src, u8 *dst, unsigned nbytes,
				      u8 *oiv)
{
	unsigned int bsize = crypto_lskcipher_blocksize(tfm);
	const u8 *iv = oiv;

	if (nbytes < bsize)
		goto out;

	do {
		crypto_lskcipher_decrypt(tfm, src, dst, bsize, NULL);
		crypto_xor(dst, iv, bsize);
		iv = src;

		src += bsize;
		dst += bsize;
	} while ((nbytes -= bsize) >= bsize);

	memcpy(oiv, iv, bsize);

out:
	return nbytes;
}

static int crypto_cbc_decrypt_inplace(struct crypto_lskcipher *tfm,
				      u8 *src, unsigned nbytes, u8 *iv)
{
	unsigned int bsize = crypto_lskcipher_blocksize(tfm);
	u8 last_iv[MAX_CIPHER_BLOCKSIZE];

	if (nbytes < bsize)
		goto out;

	/* Start of the last block. */
	src += nbytes - (nbytes & (bsize - 1)) - bsize;
	memcpy(last_iv, src, bsize);

	for (;;) {
		crypto_lskcipher_decrypt(tfm, src, src, bsize, NULL);
		if ((nbytes -= bsize) < bsize)
			break;
		crypto_xor(src, src - bsize, bsize);
		src -= bsize;
	}

	crypto_xor(src, iv, bsize);
	memcpy(iv, last_iv, bsize);

out:
	return nbytes;
}

static int crypto_cbc_decrypt(struct crypto_lskcipher *tfm, const u8 *src,
			      u8 *dst, unsigned len, u8 *iv, u32 flags)
{
	struct crypto_lskcipher **ctx = crypto_lskcipher_ctx(tfm);
	bool final = flags & CRYPTO_LSKCIPHER_FLAG_FINAL;
	struct crypto_lskcipher *cipher = *ctx;
	int rem;

	if (src == dst)
		rem = crypto_cbc_decrypt_inplace(cipher, dst, len, iv);
	else
		rem = crypto_cbc_decrypt_segment(cipher, src, dst, len, iv);

	return rem && final ? -EINVAL : rem;
}

/* Host request/ownership boundary for the original linear CBC functions. */
struct cbc_host_ctx {
	struct crypto_lskcipher cbc;
	struct crypto_lskcipher raw;
};
static int cbc_host_setkey(struct crypto_skcipher *tfm, const u8 *key, unsigned len)
{
	struct cbc_host_ctx *ctx = tfm->ctx;
	return crypto_cipher_setkey(ctx->raw.block, key, len);
}
static int cbc_host_encrypt(struct skcipher_request *req)
{
	struct cbc_host_ctx *ctx = req->tfm->ctx;
	return crypto_cbc_encrypt(&ctx->cbc, req->src->data, req->dst->data,
				  req->cryptlen, req->iv, CRYPTO_LSKCIPHER_FLAG_FINAL);
}
static int cbc_host_decrypt(struct skcipher_request *req)
{
	struct cbc_host_ctx *ctx = req->tfm->ctx;
	return crypto_cbc_decrypt(&ctx->cbc, req->src->data, req->dst->data,
				  req->cryptlen, req->iv, CRYPTO_LSKCIPHER_FLAG_FINAL);
}
static void cbc_host_exit(struct crypto_skcipher *tfm)
{
	struct cbc_host_ctx *ctx = tfm->ctx;
	crypto_free_cipher(ctx->raw.block);
}
struct crypto_skcipher *sevenzip_cbc_alloc(const char *ciphername)
{
	struct crypto_skcipher *tfm = sevenzip_skcipher_create(sizeof(struct cbc_host_ctx));
	if (IS_ERR(tfm)) return tfm;
	struct cbc_host_ctx *ctx = tfm->ctx;
	ctx->raw.block = crypto_alloc_cipher(ciphername, 0, 0);
	if (IS_ERR(ctx->raw.block)) {
		int err = PTR_ERR(ctx->raw.block);
		crypto_free_skcipher(tfm);
		return ERR_PTR(err);
	}
	ctx->cbc.child = &ctx->raw;
	tfm->setkey = cbc_host_setkey;
	tfm->encrypt = cbc_host_encrypt;
	tfm->decrypt = cbc_host_decrypt;
	tfm->exit = cbc_host_exit;
	return tfm;
}

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ECB: Electronic CodeBook mode
 *
 * Copyright (c) 2006 Herbert Xu <herbert@gondor.apana.org.au>
 */

#include "crypto/modes/skciphercompat.h"

static int crypto_ecb_crypt(struct crypto_cipher *cipher, const u8 *src,
			    u8 *dst, unsigned nbytes, bool final,
			    void (*fn)(struct crypto_tfm *, u8 *, const u8 *))
{
	const unsigned int bsize = crypto_cipher_blocksize(cipher);

	while (nbytes >= bsize) {
		fn(crypto_cipher_tfm(cipher), dst, src);

		src += bsize;
		dst += bsize;

		nbytes -= bsize;
	}

	return nbytes && final ? -EINVAL : nbytes;
}

/* Fixed-buffer request boundary; the block iteration above is original. */
struct ecb_host_ctx { struct crypto_cipher *cipher; };
static int ecb_host_setkey(struct crypto_skcipher *tfm, const u8 *key, unsigned len)
{
	struct ecb_host_ctx *ctx = tfm->ctx;
	crypto_cipher_clear_flags(ctx->cipher, CRYPTO_TFM_REQ_MASK);
	crypto_cipher_set_flags(ctx->cipher, tfm->flags & CRYPTO_TFM_REQ_MASK);
	return crypto_cipher_setkey(ctx->cipher, key, len);
}
static int ecb_host_encrypt(struct skcipher_request *req)
{
	struct ecb_host_ctx *ctx = req->tfm->ctx;
	return crypto_ecb_crypt(ctx->cipher, req->src->data, req->dst->data,
				 req->cryptlen, true, ctx->cipher->encrypt);
}
static int ecb_host_decrypt(struct skcipher_request *req)
{
	struct ecb_host_ctx *ctx = req->tfm->ctx;
	return crypto_ecb_crypt(ctx->cipher, req->src->data, req->dst->data,
				 req->cryptlen, true, ctx->cipher->decrypt);
}
static void ecb_host_exit(struct crypto_skcipher *tfm)
{
	struct ecb_host_ctx *ctx = tfm->ctx;
	crypto_free_cipher(ctx->cipher);
}
struct crypto_skcipher *sevenzip_ecb_alloc(const char *ciphername)
{
	struct crypto_skcipher *tfm = sevenzip_skcipher_create(sizeof(struct ecb_host_ctx));
	if (IS_ERR(tfm)) return tfm;
	struct ecb_host_ctx *ctx = tfm->ctx;
	ctx->cipher = crypto_alloc_cipher(ciphername, 0, 0);
	if (IS_ERR(ctx->cipher)) {
		int err = PTR_ERR(ctx->cipher);
		crypto_free_skcipher(tfm);
		return ERR_PTR(err);
	}
	tfm->ivsize = 0;
	tfm->setkey = ecb_host_setkey;
	tfm->encrypt = ecb_host_encrypt;
	tfm->decrypt = ecb_host_decrypt;
	tfm->exit = ecb_host_exit;
	return tfm;
}

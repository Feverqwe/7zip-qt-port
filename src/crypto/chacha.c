// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Crypto API wrappers for the ChaCha20, XChaCha20, and XChaCha12 stream ciphers
 *
 * Copyright (C) 2015 Martin Willi
 * Copyright (C) 2018 Google LLC
 */

#include "skciphercompat.h"
#include <crypto/chacha.h>

struct chacha_ctx {
	u32 key[8];
	int nrounds;
};

static int chacha_setkey(struct crypto_skcipher *tfm,
			 const u8 *key, unsigned int keysize, int nrounds)
{
	struct chacha_ctx *ctx = crypto_skcipher_ctx(tfm);
	int i;

	if (keysize != CHACHA_KEY_SIZE)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(ctx->key); i++)
		ctx->key[i] = get_unaligned_le32(key + i * sizeof(u32));

	ctx->nrounds = nrounds;
	return 0;
}

static int chacha20_setkey(struct crypto_skcipher *tfm,
			   const u8 *key, unsigned int keysize)
{
	return chacha_setkey(tfm, key, keysize, 20);
}

static int chacha12_setkey(struct crypto_skcipher *tfm,
			   const u8 *key, unsigned int keysize)
{
	return chacha_setkey(tfm, key, keysize, 12);
}

static int chacha_stream_xor(struct skcipher_request *req,
			     const struct chacha_ctx *ctx,
			     const u8 iv[CHACHA_IV_SIZE])
{
	struct skcipher_walk walk;
	struct chacha_state state;
	int err;

	err = skcipher_walk_virt(&walk, req, false);

	chacha_init(&state, ctx->key, iv);

	while (walk.nbytes > 0) {
		unsigned int nbytes = walk.nbytes;

		if (nbytes < walk.total)
			nbytes = round_down(nbytes, CHACHA_BLOCK_SIZE);

		chacha_crypt(&state, walk.dst.virt.addr, walk.src.virt.addr,
			     nbytes, ctx->nrounds);
		err = skcipher_walk_done(&walk, walk.nbytes - nbytes);
	}

	return err;
}

static int crypto_chacha_crypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	const struct chacha_ctx *ctx = crypto_skcipher_ctx(tfm);

	return chacha_stream_xor(req, ctx, req->iv);
}

static int crypto_xchacha_crypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	const struct chacha_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct chacha_ctx subctx;
	struct chacha_state state;
	u8 real_iv[16];

	/* Compute the subkey given the original key and first 128 nonce bits */
	chacha_init(&state, ctx->key, req->iv);
	hchacha_block(&state, subctx.key, ctx->nrounds);
	subctx.nrounds = ctx->nrounds;

	/* Build the real IV */
	memcpy(&real_iv[0], req->iv + 24, 8); /* stream position */
	memcpy(&real_iv[8], req->iv + 16, 8); /* remaining 64 nonce bits */

	/* Generate the stream and XOR it with the data */
	return chacha_stream_xor(req, &subctx, real_iv);
}

struct crypto_skcipher *sevenzip_chacha_alloc(const char *name)
{
	bool extended = strcmp(name, "chacha20") != 0;
	bool reduced = strcmp(name, "xchacha12") == 0;
	if (!reduced && extended && strcmp(name, "xchacha20")) return ERR_PTR(-ENOENT);
	struct crypto_skcipher *tfm = sevenzip_skcipher_create(sizeof(struct chacha_ctx));
	if (IS_ERR(tfm)) return tfm;
	tfm->ivsize = extended ? XCHACHA_IV_SIZE : CHACHA_IV_SIZE;
	tfm->blocksize = 1;
	tfm->setkey = reduced ? chacha12_setkey : chacha20_setkey;
	tfm->encrypt = extended ? crypto_xchacha_crypt : crypto_chacha_crypt;
	tfm->decrypt = tfm->encrypt;
	return tfm;
}

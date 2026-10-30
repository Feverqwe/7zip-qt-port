// SPDX-License-Identifier: GPL-2.0
/*
 * ESSIV skcipher and aead template for block encryption
 *
 * This template encapsulates the ESSIV IV generation algorithm used by
 * dm-crypt and fscrypt, which converts the initial vector for the skcipher
 * used for block encryption, by encrypting it using the hash of the
 * skcipher key as encryption key. Usually, the input IV is a 64-bit sector
 * number in LE representation zero-padded to the size of the IV, but this
 * is not assumed by this driver.
 *
 * The typical use of this template is to instantiate the skcipher
 * 'essiv(cbc(aes),sha256)', which is the only instantiation used by
 * fscrypt, and the most relevant one for dm-crypt. However, dm-crypt
 * also permits ESSIV to be used in combination with the authenc template,
 * e.g., 'essiv(authenc(hmac(sha256),cbc(aes)),sha256)', in which case
 * we need to instantiate an aead that accepts the same special key format
 * as the authenc template, and deals with the way the encrypted IV is
 * embedded into the AAD area of the aead request. This means the AEAD
 * flavor produced by this template is tightly coupled to the way dm-crypt
 * happens to use it.
 *
 * Copyright (c) 2019 Linaro, Ltd. <ard.biesheuvel@linaro.org>
 *
 * Heavily based on:
 * adiantum length-preserving encryption mode
 *
 * Copyright 2018 Google LLC
 */

#include "crypto/modes/skciphercompat.h"

struct essiv_instance_ctx {
	const char *essiv_cipher_name;
	const char *shash_driver_name;
};

struct essiv_tfm_ctx {
	union {
		struct crypto_skcipher	*skcipher;
		struct crypto_aead	*aead;
	} u;
	struct crypto_cipher		*essiv_cipher;
	struct crypto_shash		*hash;
	int				ivoffset;
};

static int essiv_skcipher_setkey(struct crypto_skcipher *tfm,
				 const u8 *key, unsigned int keylen)
{
	struct essiv_tfm_ctx *tctx = crypto_skcipher_ctx(tfm);
	u8 salt[HASH_MAX_DIGESTSIZE];
	int err;

	crypto_skcipher_clear_flags(tctx->u.skcipher, CRYPTO_TFM_REQ_MASK);
	crypto_skcipher_set_flags(tctx->u.skcipher,
				  crypto_skcipher_get_flags(tfm) &
				  CRYPTO_TFM_REQ_MASK);
	err = crypto_skcipher_setkey(tctx->u.skcipher, key, keylen);
	if (err)
		return err;

	err = crypto_shash_tfm_digest(tctx->hash, key, keylen, salt);
	if (err)
		return err;

	crypto_cipher_clear_flags(tctx->essiv_cipher, CRYPTO_TFM_REQ_MASK);
	crypto_cipher_set_flags(tctx->essiv_cipher,
				crypto_skcipher_get_flags(tfm) &
				CRYPTO_TFM_REQ_MASK);
	return crypto_cipher_setkey(tctx->essiv_cipher, salt,
				    crypto_shash_digestsize(tctx->hash));
}

static void essiv_skcipher_done(void *data, int err)
{
	struct skcipher_request *req = data;

	skcipher_request_complete(req, err);
}

static int essiv_skcipher_crypt(struct skcipher_request *req, bool enc)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	const struct essiv_tfm_ctx *tctx = crypto_skcipher_ctx(tfm);
	struct skcipher_request *subreq = skcipher_request_ctx(req);

	crypto_cipher_encrypt_one(tctx->essiv_cipher, req->iv, req->iv);

	skcipher_request_set_tfm(subreq, tctx->u.skcipher);
	skcipher_request_set_crypt(subreq, req->src, req->dst, req->cryptlen,
				   req->iv);
	skcipher_request_set_callback(subreq, skcipher_request_flags(req),
				      essiv_skcipher_done, req);

	return enc ? crypto_skcipher_encrypt(subreq) :
		     crypto_skcipher_decrypt(subreq);
}

static int essiv_skcipher_encrypt(struct skcipher_request *req)
{
	return essiv_skcipher_crypt(req, true);
}

static int essiv_skcipher_decrypt(struct skcipher_request *req)
{
	return essiv_skcipher_crypt(req, false);
}

static int essiv_init_tfm(struct essiv_instance_ctx *ictx,
			  struct essiv_tfm_ctx *tctx)
{
	struct crypto_cipher *essiv_cipher;
	struct crypto_shash *hash;
	int err;

	essiv_cipher = crypto_alloc_cipher(ictx->essiv_cipher_name, 0, 0);
	if (IS_ERR(essiv_cipher))
		return PTR_ERR(essiv_cipher);

	hash = crypto_alloc_shash(ictx->shash_driver_name, 0, 0);
	if (IS_ERR(hash)) {
		err = PTR_ERR(hash);
		goto err_free_essiv_cipher;
	}

	tctx->essiv_cipher = essiv_cipher;
	tctx->hash = hash;

	return 0;

err_free_essiv_cipher:
	crypto_free_cipher(essiv_cipher);
	return err;
}

static int essiv_skcipher_init_tfm(struct crypto_skcipher *tfm)
{
	struct essiv_instance_ctx config = { "aes", "sha256" };
	struct essiv_instance_ctx *ictx = &config;
	struct essiv_tfm_ctx *tctx = crypto_skcipher_ctx(tfm);
	struct crypto_skcipher *skcipher;
	int err;

	skcipher = crypto_alloc_skcipher("cbc(aes)", 0, 0);
	if (IS_ERR(skcipher))
		return PTR_ERR(skcipher);

	crypto_skcipher_set_reqsize(tfm, sizeof(struct skcipher_request) +
				         crypto_skcipher_reqsize(skcipher));

	err = essiv_init_tfm(ictx, tctx);
	if (err) {
		crypto_free_skcipher(skcipher);
		return err;
	}

	tctx->u.skcipher = skcipher;
	return 0;
}

static void essiv_skcipher_exit_tfm(struct crypto_skcipher *tfm)
{
	struct essiv_tfm_ctx *tctx = crypto_skcipher_ctx(tfm);

	crypto_free_skcipher(tctx->u.skcipher);
	crypto_free_cipher(tctx->essiv_cipher);
	crypto_free_shash(tctx->hash);
}

struct crypto_skcipher *sevenzip_essiv_alloc(void)
{
	struct crypto_skcipher *tfm = sevenzip_skcipher_create(sizeof(struct essiv_tfm_ctx));
	if (IS_ERR(tfm)) return tfm;
	int err = essiv_skcipher_init_tfm(tfm);
	if (err) { crypto_free_skcipher(tfm); return ERR_PTR(err); }
	tfm->setkey = essiv_skcipher_setkey;
	tfm->encrypt = essiv_skcipher_encrypt;
	tfm->decrypt = essiv_skcipher_decrypt;
	tfm->exit = essiv_skcipher_exit_tfm;
	return tfm;
}

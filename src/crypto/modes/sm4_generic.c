// SPDX-License-Identifier: GPL-2.0

/*
 * SM4 Cipher Algorithm.
 *
 * Copyright (C) 2018 ARM Limited or its affiliates.
 * All rights reserved.
 */

#include "crypto/modes/skciphercompat.h"
#include <crypto/sm4.h>

/**
 * sm4_setkey - Set the SM4 key.
 * @tfm:	The %crypto_tfm that is used in the context.
 * @in_key:	The input key.
 * @key_len:	The size of the key.
 *
 * This function uses sm4_expandkey() to expand the key.
 * &sm4_ctx _must_ be the private data embedded in @tfm which is
 * retrieved with crypto_tfm_ctx().
 *
 * Return: 0 on success; -EINVAL on failure (only happens for bad key lengths)
 */
static int sm4_setkey(struct crypto_tfm *tfm, const u8 *in_key,
		       unsigned int key_len)
{
	struct sm4_ctx *ctx = crypto_tfm_ctx(tfm);

	return sm4_expandkey(ctx, in_key, key_len);
}

/* encrypt a block of text */

static void sm4_encrypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	const struct sm4_ctx *ctx = crypto_tfm_ctx(tfm);

	sm4_crypt_block(ctx->rkey_enc, out, in);
}

/* decrypt a block of text */

static void sm4_decrypt(struct crypto_tfm *tfm, u8 *out, const u8 *in)
{
	const struct sm4_ctx *ctx = crypto_tfm_ctx(tfm);

	sm4_crypt_block(ctx->rkey_dec, out, in);
}

struct crypto_cipher *sevenzip_sm4_alloc(void)
{
	struct crypto_cipher *tfm = sevenzip_cipher_create(sizeof(struct sm4_ctx));
	if (IS_ERR(tfm)) return tfm;
	tfm->setkey = sm4_setkey;
	tfm->encrypt = sm4_encrypt;
	tfm->decrypt = sm4_decrypt;
	return tfm;
}

/* SPDX-License-Identifier: GPL-2.0-only */
/* Crypto API dispatch to the original Linux portable AES mode library. */
#include "skciphercompat.h"
#include <crypto/aes-cbc.h>
#include <crypto/aes-ctr.h>
#include <crypto/aes-ecb.h>
#include <crypto/aes-xts.h>

enum aes_provider_mode { AES_ECB, AES_CBC, AES_XTS, AES_CTR, AES_XCTR };
struct aes_provider_ctx {
	enum aes_provider_mode mode;
	union { struct aes_key aes; struct aes_xts_key xts; } key;
};

static int library_aes_setkey(struct crypto_skcipher *tfm, const u8 *key, unsigned len)
{
	struct aes_provider_ctx *ctx = tfm->ctx;
	if (ctx->mode == AES_XTS)
		return aes_xts_preparekey(&ctx->key.xts, key, len,
			tfm->flags & CRYPTO_TFM_REQ_FORBID_WEAK_KEYS ? XTS_FORBID_WEAK_KEYS : 0);
	if (ctx->mode == AES_CTR || ctx->mode == AES_XCTR)
		return aes_prepareenckey(&ctx->key.aes.enc, key, len);
	return aes_preparekey(&ctx->key.aes, key, len);
}

static int library_aes_crypt(struct skcipher_request *req, bool decrypt)
{
	struct aes_provider_ctx *ctx = req->tfm->ctx;
	u8 *dst = req->dst->data;
	const u8 *src = req->src->data;
	size_t len = req->cryptlen;
	if ((ctx->mode == AES_ECB || ctx->mode == AES_CBC) && len % AES_BLOCK_SIZE)
		return -EINVAL;
	if (ctx->mode == AES_XTS && len < AES_BLOCK_SIZE)
		return -EINVAL;
	switch (ctx->mode) {
	case AES_ECB:
		if (decrypt) aes_ecb_decrypt(dst, src, len, &ctx->key.aes);
		else aes_ecb_encrypt(dst, src, len, &ctx->key.aes);
		break;
	case AES_CBC:
		if (decrypt) aes_cbc_decrypt(dst, src, len, req->iv, &ctx->key.aes);
		else aes_cbc_encrypt(dst, src, len, req->iv, &ctx->key.aes);
		break;
	case AES_XTS: {
		u8 tweak[AES_BLOCK_SIZE];
		memcpy(tweak, req->iv, sizeof(tweak));
		if (decrypt) aes_xts_decrypt(dst, src, len, tweak, &ctx->key.xts, false);
		else aes_xts_encrypt(dst, src, len, tweak, &ctx->key.xts, false);
		memzero_explicit(tweak, sizeof(tweak));
		break;
	}
	case AES_CTR:
		aes_ctr(dst, src, len, req->iv, &ctx->key.aes.enc);
		break;
	case AES_XCTR: {
		u64 counter = 1;
		aes_xctr(dst, src, len, &counter, req->iv, &ctx->key.aes.enc);
		break;
	}
	}
	return 0;
}
static int library_aes_encrypt(struct skcipher_request *req) { return library_aes_crypt(req, false); }
static int library_aes_decrypt(struct skcipher_request *req) { return library_aes_crypt(req, true); }

struct crypto_skcipher *sevenzip_aes_alloc(const char *name)
{
	static const char *const names[] = {
		"ecb(aes)", "cbc(aes)", "xts(aes)", "ctr(aes)", "xctr(aes)"
	};
	unsigned mode;
	for (mode = 0; mode < ARRAY_SIZE(names); ++mode)
		if (!strcmp(name, names[mode])) break;
	if (mode == ARRAY_SIZE(names)) return ERR_PTR(-ENOENT);
	struct crypto_skcipher *tfm = sevenzip_skcipher_create(sizeof(struct aes_provider_ctx));
	if (IS_ERR(tfm)) return tfm;
	((struct aes_provider_ctx *)tfm->ctx)->mode = (enum aes_provider_mode)mode;
	tfm->ivsize = mode == AES_ECB ? 0 : AES_BLOCK_SIZE;
	tfm->blocksize = mode == AES_CTR || mode == AES_XCTR ? 1 : AES_BLOCK_SIZE;
	tfm->setkey = library_aes_setkey;
	tfm->encrypt = library_aes_encrypt;
	tfm->decrypt = library_aes_decrypt;
	return tfm;
}

/* SPDX-License-Identifier: GPL-2.0-only */
/* Ownership and synchronous dispatch services; cipher/mode algorithms are Linux. */
#include "skciphercompat.h"

struct crypto_skcipher *sevenzip_skcipher_create(size_t ctxsize)
{
	struct crypto_skcipher *tfm = calloc(1, sizeof(*tfm));
	if (!tfm) return ERR_PTR(-ENOMEM);
	tfm->ctx = calloc(1, ctxsize);
	if (!tfm->ctx) { free(tfm); return ERR_PTR(-ENOMEM); }
	tfm->ctxsize = ctxsize;
	tfm->ivsize = AES_BLOCK_SIZE;
	tfm->blocksize = AES_BLOCK_SIZE;
	return tfm;
}

struct crypto_skcipher *crypto_alloc_skcipher(const char *name, unsigned type, unsigned mask)
{
	(void)type; (void)mask;
	if (!strcmp(name, "hctr2(aes)")) return sevenzip_hctr2_alloc();
	if (!strcmp(name, "essiv(cbc(aes),sha256)")) return sevenzip_essiv_alloc();
	if (!strcmp(name, "cts(cbc(aes))")) return sevenzip_cts_alloc("cbc(aes)");
	if (!strcmp(name, "cbc(sm4)")) return sevenzip_cbc_alloc("sm4");
	if (!strcmp(name, "cts(cbc(sm4))")) return sevenzip_cts_alloc("cbc(sm4)");
	if (!strcmp(name, "ecb(sm4)")) return sevenzip_ecb_alloc("sm4");
	if (!strcmp(name, "xts(sm4)")) return sevenzip_xts_alloc("ecb(sm4)", "sm4");
	if (!strcmp(name, "adiantum(xchacha12,aes)")) return sevenzip_adiantum_alloc("xchacha12");
	if (!strcmp(name, "adiantum(xchacha20,aes)")) return sevenzip_adiantum_alloc("xchacha20");
	if (!strcmp(name, "chacha20") || !strcmp(name, "xchacha12") || !strcmp(name, "xchacha20"))
		return sevenzip_chacha_alloc(name);
	return sevenzip_aes_alloc(name);
}

void crypto_free_skcipher(struct crypto_skcipher *tfm)
{
	if (!tfm || IS_ERR(tfm)) return;
	if (tfm->exit) tfm->exit(tfm);
	memzero_explicit(tfm->ctx, tfm->ctxsize);
	free(tfm->ctx);
	memzero_explicit(tfm, sizeof(*tfm));
	free(tfm);
}

struct crypto_cipher *sevenzip_cipher_create(size_t ctxsize)
{
	struct crypto_cipher *tfm = calloc(1, sizeof(*tfm));
	if (!tfm) return ERR_PTR(-ENOMEM);
	tfm->base.ctx = calloc(1, ctxsize);
	if (!tfm->base.ctx) { free(tfm); return ERR_PTR(-ENOMEM); }
	tfm->ctxsize = ctxsize;
	return tfm;
}
static int raw_aes_setkey(struct crypto_tfm *tfm, const u8 *key, unsigned len)
{ return aes_preparekey(tfm->ctx, key, len); }
static void raw_aes_encrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{ aes_encrypt(tfm->ctx, dst, src); }
static void raw_aes_decrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{ aes_decrypt(tfm->ctx, dst, src); }
struct crypto_cipher *crypto_alloc_cipher(const char *name, unsigned type, unsigned mask)
{
	(void)type; (void)mask;
	if (!strcmp(name, "sm4")) return sevenzip_sm4_alloc();
	if (strcmp(name, "aes")) return ERR_PTR(-ENOENT);
	struct crypto_cipher *tfm = sevenzip_cipher_create(sizeof(struct aes_key));
	if (IS_ERR(tfm)) return tfm;
	tfm->setkey = raw_aes_setkey;
	tfm->encrypt = raw_aes_encrypt;
	tfm->decrypt = raw_aes_decrypt;
	return tfm;
}
void crypto_free_cipher(struct crypto_cipher *tfm)
{
	if (!tfm || IS_ERR(tfm)) return;
	memzero_explicit(tfm->base.ctx, tfm->ctxsize);
	free(tfm->base.ctx);
	memzero_explicit(tfm, sizeof(*tfm));
	free(tfm);
}

struct crypto_shash *crypto_alloc_shash(const char *name, unsigned type, unsigned mask)
{
	(void)type; (void)mask;
	if (strcmp(name, "sha256")) return ERR_PTR(-ENOENT);
	struct crypto_shash *hash = calloc(1, sizeof(*hash));
	return hash ? hash : ERR_PTR(-ENOMEM);
}
void crypto_free_shash(struct crypto_shash *hash) { if (!IS_ERR(hash)) free(hash); }

struct skcipher_request *skcipher_request_alloc(struct crypto_skcipher *tfm, unsigned flags)
{
	(void)flags;
	if (tfm->reqsize > SIZE_MAX - sizeof(struct skcipher_request)) return NULL;
	struct skcipher_request *req = calloc(1, sizeof(*req) + tfm->reqsize);
	if (req) req->tfm = tfm;
	return req;
}
void skcipher_request_free(struct skcipher_request *req)
{
	if (!req) return;
	memzero_explicit(req, sizeof(*req) + req->tfm->reqsize);
	free(req);
}

static int skcipher_crypt(struct skcipher_request *req, bool decrypt)
{
	if (!req || !req->tfm || !req->src || !req->dst || (req->tfm->ivsize && !req->iv) ||
	    req->cryptlen > req->src->len || req->cryptlen > req->dst->len) return -EINVAL;
	if (!req->tfm->key_ready) return -ENOKEY;
	return decrypt ? req->tfm->decrypt(req) : req->tfm->encrypt(req);
}
int crypto_skcipher_encrypt(struct skcipher_request *req) { return skcipher_crypt(req, false); }
int crypto_skcipher_decrypt(struct skcipher_request *req) { return skcipher_crypt(req, true); }

// SPDX-License-Identifier: GPL-2.0-or-later
/* XTS: as defined in IEEE1619/D16
 *	http://grouper.ieee.org/groups/1619/email/pdf00086.pdf
 *
 * Copyright (c) 2007 Rik Snel <rsnel@cube.dyndns.org>
 *
 * Based on ecb.c
 * Copyright (c) 2006 Herbert Xu <herbert@gondor.apana.org.au>
 */
#include "crypto/modes/skciphercompat.h"

#include <crypto/xts.h>
#include <crypto/b128ops.h>
#include <crypto/gf128mul.h>

struct xts_tfm_ctx {
	struct crypto_skcipher *child;
	struct crypto_cipher *tweak;
};

struct xts_request_ctx {
	le128 t;
	struct scatterlist *tail;
	struct scatterlist sg[2];
	struct skcipher_request subreq;
};

static int xts_setkey(struct crypto_skcipher *parent, const u8 *key,
		      unsigned int keylen)
{
	struct xts_tfm_ctx *ctx = crypto_skcipher_ctx(parent);
	struct crypto_skcipher *child;
	struct crypto_cipher *tweak;
	int err;

	err = xts_verify_key(parent, key, keylen);
	if (err)
		return err;

	keylen /= 2;

	/* we need two cipher instances: one to compute the initial 'tweak'
	 * by encrypting the IV (usually the 'plain' iv) and the other
	 * one to encrypt and decrypt the data */

	/* tweak cipher, uses Key2 i.e. the second half of *key */
	tweak = ctx->tweak;
	crypto_cipher_clear_flags(tweak, CRYPTO_TFM_REQ_MASK);
	crypto_cipher_set_flags(tweak, crypto_skcipher_get_flags(parent) &
				       CRYPTO_TFM_REQ_MASK);
	err = crypto_cipher_setkey(tweak, key + keylen, keylen);
	if (err)
		return err;

	/* data cipher, uses Key1 i.e. the first half of *key */
	child = ctx->child;
	crypto_skcipher_clear_flags(child, CRYPTO_TFM_REQ_MASK);
	crypto_skcipher_set_flags(child, crypto_skcipher_get_flags(parent) &
					 CRYPTO_TFM_REQ_MASK);
	return crypto_skcipher_setkey(child, key, keylen);
}

/*
 * We compute the tweak masks twice (both before and after the ECB encryption or
 * decryption) to avoid having to allocate a temporary buffer and/or make
 * multiple calls to the 'ecb(..)' instance, which usually would be slower than
 * just doing the gf128mul_x_ble() calls again.
 */
static int xts_xor_tweak(struct skcipher_request *req, bool second_pass,
			 bool enc)
{
	struct xts_request_ctx *rctx = skcipher_request_ctx(req);
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	const bool cts = (req->cryptlen % XTS_BLOCK_SIZE);
	const int bs = XTS_BLOCK_SIZE;
	struct skcipher_walk w;
	le128 t = rctx->t;
	int err;

	if (second_pass) {
		req = &rctx->subreq;
		/* set to our TFM to enforce correct alignment: */
		skcipher_request_set_tfm(req, tfm);
	}
	err = skcipher_walk_virt(&w, req, false);

	while (w.nbytes) {
		unsigned int avail = w.nbytes;
		const le128 *wsrc;
		le128 *wdst;

		wsrc = w.src.virt.addr;
		wdst = w.dst.virt.addr;

		do {
			if (unlikely(cts) &&
			    w.total - w.nbytes + avail < 2 * XTS_BLOCK_SIZE) {
				if (!enc) {
					if (second_pass)
						rctx->t = t;
					gf128mul_x_ble(&t, &t);
				}
				le128_xor(wdst, &t, wsrc);
				if (enc && second_pass)
					gf128mul_x_ble(&rctx->t, &t);
				skcipher_walk_done(&w, avail - bs);
				return 0;
			}

			le128_xor(wdst++, &t, wsrc++);
			gf128mul_x_ble(&t, &t);
		} while ((avail -= bs) >= bs);

		err = skcipher_walk_done(&w, avail);
	}

	return err;
}

static int xts_xor_tweak_pre(struct skcipher_request *req, bool enc)
{
	return xts_xor_tweak(req, false, enc);
}

static int xts_xor_tweak_post(struct skcipher_request *req, bool enc)
{
	return xts_xor_tweak(req, true, enc);
}

static void xts_cts_done(void *data, int err)
{
	struct skcipher_request *req = data;
	le128 b;

	if (!err) {
		struct xts_request_ctx *rctx = skcipher_request_ctx(req);

		scatterwalk_map_and_copy(&b, rctx->tail, 0, XTS_BLOCK_SIZE, 0);
		le128_xor(&b, &rctx->t, &b);
		scatterwalk_map_and_copy(&b, rctx->tail, 0, XTS_BLOCK_SIZE, 1);
	}

	skcipher_request_complete(req, err);
}

static int xts_cts_final(struct skcipher_request *req,
			 int (*crypt)(struct skcipher_request *req))
{
	const struct xts_tfm_ctx *ctx =
		crypto_skcipher_ctx(crypto_skcipher_reqtfm(req));
	int offset = req->cryptlen & ~(XTS_BLOCK_SIZE - 1);
	struct xts_request_ctx *rctx = skcipher_request_ctx(req);
	struct skcipher_request *subreq = &rctx->subreq;
	int tail = req->cryptlen % XTS_BLOCK_SIZE;
	le128 b[2];
	int err;

	rctx->tail = scatterwalk_ffwd(rctx->sg, req->dst,
				      offset - XTS_BLOCK_SIZE);

	scatterwalk_map_and_copy(b, rctx->tail, 0, XTS_BLOCK_SIZE, 0);
	b[1] = b[0];
	scatterwalk_map_and_copy(b, req->src, offset, tail, 0);

	le128_xor(b, &rctx->t, b);

	scatterwalk_map_and_copy(b, rctx->tail, 0, XTS_BLOCK_SIZE + tail, 1);

	skcipher_request_set_tfm(subreq, ctx->child);
	skcipher_request_set_callback(subreq, req->base.flags, xts_cts_done,
				      req);
	skcipher_request_set_crypt(subreq, rctx->tail, rctx->tail,
				   XTS_BLOCK_SIZE, NULL);

	err = crypt(subreq);
	if (err)
		return err;

	scatterwalk_map_and_copy(b, rctx->tail, 0, XTS_BLOCK_SIZE, 0);
	le128_xor(b, &rctx->t, b);
	scatterwalk_map_and_copy(b, rctx->tail, 0, XTS_BLOCK_SIZE, 1);

	return 0;
}

static void xts_encrypt_done(void *data, int err)
{
	struct skcipher_request *req = data;

	if (!err) {
		struct xts_request_ctx *rctx = skcipher_request_ctx(req);

		rctx->subreq.base.flags &= CRYPTO_TFM_REQ_MAY_BACKLOG;
		err = xts_xor_tweak_post(req, true);

		if (!err && unlikely(req->cryptlen % XTS_BLOCK_SIZE)) {
			err = xts_cts_final(req, crypto_skcipher_encrypt);
			if (err == -EINPROGRESS || err == -EBUSY)
				return;
		}
	}

	skcipher_request_complete(req, err);
}

static void xts_decrypt_done(void *data, int err)
{
	struct skcipher_request *req = data;

	if (!err) {
		struct xts_request_ctx *rctx = skcipher_request_ctx(req);

		rctx->subreq.base.flags &= CRYPTO_TFM_REQ_MAY_BACKLOG;
		err = xts_xor_tweak_post(req, false);

		if (!err && unlikely(req->cryptlen % XTS_BLOCK_SIZE)) {
			err = xts_cts_final(req, crypto_skcipher_decrypt);
			if (err == -EINPROGRESS || err == -EBUSY)
				return;
		}
	}

	skcipher_request_complete(req, err);
}

static int xts_init_crypt(struct skcipher_request *req,
			  crypto_completion_t compl)
{
	const struct xts_tfm_ctx *ctx =
		crypto_skcipher_ctx(crypto_skcipher_reqtfm(req));
	struct xts_request_ctx *rctx = skcipher_request_ctx(req);
	struct skcipher_request *subreq = &rctx->subreq;

	if (req->cryptlen < XTS_BLOCK_SIZE)
		return -EINVAL;

	skcipher_request_set_tfm(subreq, ctx->child);
	skcipher_request_set_callback(subreq, req->base.flags, compl, req);
	skcipher_request_set_crypt(subreq, req->dst, req->dst,
				   req->cryptlen & ~(XTS_BLOCK_SIZE - 1), NULL);

	/* calculate first value of T */
	crypto_cipher_encrypt_one(ctx->tweak, (u8 *)&rctx->t, req->iv);

	return 0;
}

static int xts_encrypt(struct skcipher_request *req)
{
	struct xts_request_ctx *rctx = skcipher_request_ctx(req);
	struct skcipher_request *subreq = &rctx->subreq;
	int err;

	err = xts_init_crypt(req, xts_encrypt_done);
	if (!err) err = xts_xor_tweak_pre(req, true);
	if (!err) err = crypto_skcipher_encrypt(subreq);
	if (!err) err = xts_xor_tweak_post(req, true);

	if (err || likely((req->cryptlen % XTS_BLOCK_SIZE) == 0))
		return err;

	return xts_cts_final(req, crypto_skcipher_encrypt);
}

static int xts_decrypt(struct skcipher_request *req)
{
	struct xts_request_ctx *rctx = skcipher_request_ctx(req);
	struct skcipher_request *subreq = &rctx->subreq;
	int err;

	err = xts_init_crypt(req, xts_decrypt_done);
	if (!err) err = xts_xor_tweak_pre(req, false);
	if (!err) err = crypto_skcipher_decrypt(subreq);
	if (!err) err = xts_xor_tweak_post(req, false);

	if (err || likely((req->cryptlen % XTS_BLOCK_SIZE) == 0))
		return err;

	return xts_cts_final(req, crypto_skcipher_decrypt);
}

static int xts_init_tfm(struct crypto_skcipher *tfm, const char *childname, const char *tweakname)
{
	struct xts_tfm_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct crypto_skcipher *child;
	struct crypto_cipher *tweak;

	child = crypto_alloc_skcipher(childname, 0, 0);
	if (IS_ERR(child))
		return PTR_ERR(child);

	ctx->child = child;

	tweak = crypto_alloc_cipher(tweakname, 0, 0);
	if (IS_ERR(tweak)) {
		crypto_free_skcipher(ctx->child);
		return PTR_ERR(tweak);
	}

	ctx->tweak = tweak;

	crypto_skcipher_set_reqsize(tfm, crypto_skcipher_reqsize(child) +
					 sizeof(struct xts_request_ctx));

	return 0;
}

static void xts_exit_tfm(struct crypto_skcipher *tfm)
{
	struct xts_tfm_ctx *ctx = crypto_skcipher_ctx(tfm);

	crypto_free_skcipher(ctx->child);
	crypto_free_cipher(ctx->tweak);
}

struct crypto_skcipher *sevenzip_xts_alloc(const char *childname, const char *tweakname)
{
	struct crypto_skcipher *tfm = sevenzip_skcipher_create(sizeof(struct xts_tfm_ctx));
	if (IS_ERR(tfm)) return tfm;
	int err = xts_init_tfm(tfm, childname, tweakname);
	if (err) { crypto_free_skcipher(tfm); return ERR_PTR(err); }
	tfm->setkey = xts_setkey;
	tfm->encrypt = xts_encrypt;
	tfm->decrypt = xts_decrypt;
	tfm->exit = xts_exit_tfm;
	return tfm;
}

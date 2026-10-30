// SPDX-License-Identifier: GPL-2.0
/*
 * HCTR2 length-preserving encryption mode
 *
 * Copyright 2021 Google LLC
 */


/*
 * HCTR2 is a length-preserving encryption mode that is efficient on
 * processors with instructions to accelerate AES and carryless
 * multiplication, e.g. x86 processors with AES-NI and CLMUL, and ARM
 * processors with the ARMv8 crypto extensions.
 *
 * For more details, see the paper: "Length-preserving encryption with HCTR2"
 * (https://eprint.iacr.org/2021/1441.pdf)
 */

#include "crypto/modes/skciphercompat.h"
#include <crypto/gf128hash.h>

#define BLOCKCIPHER_BLOCK_SIZE		16

/*
 * The specification allows variable-length tweaks, but Linux's crypto API
 * currently only allows algorithms to support a single length.  The "natural"
 * tweak length for HCTR2 is 16, since that fits into one POLYVAL block for
 * the best performance.  But longer tweaks are useful for fscrypt, to avoid
 * needing to derive per-file keys.  So instead we use two blocks, or 32 bytes.
 */
#define TWEAK_SIZE		32

struct hctr2_tfm_ctx {
	struct crypto_cipher *blockcipher;
	struct crypto_skcipher *xctr;
	struct polyval_key poly_key;
	struct polyval_elem hashed_tweaklens[2];
	u8 L[BLOCKCIPHER_BLOCK_SIZE];
};

struct hctr2_request_ctx {
	u8 first_block[BLOCKCIPHER_BLOCK_SIZE];
	u8 xctr_iv[BLOCKCIPHER_BLOCK_SIZE];
	struct scatterlist *bulk_part_dst;
	struct scatterlist *bulk_part_src;
	struct scatterlist sg_src[2];
	struct scatterlist sg_dst[2];
	struct polyval_elem hashed_tweak;
	/*
	 * skcipher sub-request size is unknown at compile-time, so it needs to
	 * go after the members with known sizes.
	 */
	union {
		struct polyval_ctx poly_ctx;
		struct skcipher_request xctr_req;
	} u;
};

/*
 * The input data for each HCTR2 hash step begins with a 16-byte block that
 * contains the tweak length and a flag that indicates whether the input is evenly
 * divisible into blocks.  Since this implementation only supports one tweak
 * length, we precompute the two hash states resulting from hashing the two
 * possible values of this initial block.  This reduces by one block the amount of
 * data that needs to be hashed for each encryption/decryption
 *
 * These precomputed hashes are stored in hctr2_tfm_ctx.
 */
static void hctr2_hash_tweaklens(struct hctr2_tfm_ctx *tctx)
{
	struct polyval_ctx ctx;

	for (int has_remainder = 0; has_remainder < 2; has_remainder++) {
		const __le64 tweak_length_block[2] = {
			cpu_to_le64(TWEAK_SIZE * 8 * 2 + 2 + has_remainder),
		};

		polyval_init(&ctx, &tctx->poly_key);
		polyval_update(&ctx, (const u8 *)&tweak_length_block,
			       sizeof(tweak_length_block));
		static_assert(sizeof(tweak_length_block) == POLYVAL_BLOCK_SIZE);
		polyval_export_blkaligned(
			&ctx, &tctx->hashed_tweaklens[has_remainder]);
	}
	memzero_explicit(&ctx, sizeof(ctx));
}

static int hctr2_setkey(struct crypto_skcipher *tfm, const u8 *key,
			unsigned int keylen)
{
	struct hctr2_tfm_ctx *tctx = crypto_skcipher_ctx(tfm);
	u8 hbar[BLOCKCIPHER_BLOCK_SIZE];
	int err;

	crypto_cipher_clear_flags(tctx->blockcipher, CRYPTO_TFM_REQ_MASK);
	crypto_cipher_set_flags(tctx->blockcipher,
				crypto_skcipher_get_flags(tfm) &
				CRYPTO_TFM_REQ_MASK);
	err = crypto_cipher_setkey(tctx->blockcipher, key, keylen);
	if (err)
		return err;

	crypto_skcipher_clear_flags(tctx->xctr, CRYPTO_TFM_REQ_MASK);
	crypto_skcipher_set_flags(tctx->xctr,
				  crypto_skcipher_get_flags(tfm) &
				  CRYPTO_TFM_REQ_MASK);
	err = crypto_skcipher_setkey(tctx->xctr, key, keylen);
	if (err)
		return err;

	memset(hbar, 0, sizeof(hbar));
	crypto_cipher_encrypt_one(tctx->blockcipher, hbar, hbar);

	memset(tctx->L, 0, sizeof(tctx->L));
	tctx->L[0] = 0x01;
	crypto_cipher_encrypt_one(tctx->blockcipher, tctx->L, tctx->L);

	static_assert(sizeof(hbar) == POLYVAL_BLOCK_SIZE);
	polyval_preparekey(&tctx->poly_key, hbar);
	memzero_explicit(hbar, sizeof(hbar));

	hctr2_hash_tweaklens(tctx);
	return 0;
}

static void hctr2_hash_tweak(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	const struct hctr2_tfm_ctx *tctx = crypto_skcipher_ctx(tfm);
	struct hctr2_request_ctx *rctx = skcipher_request_ctx(req);
	struct polyval_ctx *poly_ctx = &rctx->u.poly_ctx;
	bool has_remainder = req->cryptlen % POLYVAL_BLOCK_SIZE;

	polyval_import_blkaligned(poly_ctx, &tctx->poly_key,
				  &tctx->hashed_tweaklens[has_remainder]);
	polyval_update(poly_ctx, req->iv, TWEAK_SIZE);

	// Store the hashed tweak, since we need it when computing both
	// H(T || N) and H(T || V).
	static_assert(TWEAK_SIZE % POLYVAL_BLOCK_SIZE == 0);
	polyval_export_blkaligned(poly_ctx, &rctx->hashed_tweak);
}

static void hctr2_hash_message(struct skcipher_request *req,
			       struct scatterlist *sgl,
			       u8 digest[POLYVAL_DIGEST_SIZE])
{
	static const u8 padding = 0x1;
	struct hctr2_request_ctx *rctx = skcipher_request_ctx(req);
	struct polyval_ctx *poly_ctx = &rctx->u.poly_ctx;
	const unsigned int bulk_len = req->cryptlen - BLOCKCIPHER_BLOCK_SIZE;
	struct sg_mapping_iter miter;
	int i;
	int n = 0;

	sg_miter_start(&miter, sgl, sg_nents(sgl),
		       SG_MITER_FROM_SG | SG_MITER_ATOMIC);
	for (i = 0; i < bulk_len; i += n) {
		sg_miter_next(&miter);
		n = min_t(unsigned int, miter.length, bulk_len - i);
		polyval_update(poly_ctx, miter.addr, n);
	}
	sg_miter_stop(&miter);

	if (req->cryptlen % BLOCKCIPHER_BLOCK_SIZE)
		polyval_update(poly_ctx, &padding, 1);
	polyval_final(poly_ctx, digest);
}

static int hctr2_finish(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	const struct hctr2_tfm_ctx *tctx = crypto_skcipher_ctx(tfm);
	struct hctr2_request_ctx *rctx = skcipher_request_ctx(req);
	struct polyval_ctx *poly_ctx = &rctx->u.poly_ctx;
	u8 digest[POLYVAL_DIGEST_SIZE];

	// U = UU ^ H(T || V)
	// or M = MM ^ H(T || N)
	polyval_import_blkaligned(poly_ctx, &tctx->poly_key,
				  &rctx->hashed_tweak);
	hctr2_hash_message(req, rctx->bulk_part_dst, digest);
	crypto_xor(rctx->first_block, digest, BLOCKCIPHER_BLOCK_SIZE);

	// Copy U (or M) into dst scatterlist
	scatterwalk_map_and_copy(rctx->first_block, req->dst,
				 0, BLOCKCIPHER_BLOCK_SIZE, 1);
	return 0;
}

static void hctr2_xctr_done(void *data, int err)
{
	struct skcipher_request *req = data;

	if (!err)
		err = hctr2_finish(req);

	skcipher_request_complete(req, err);
}

static int hctr2_crypt(struct skcipher_request *req, bool enc)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	const struct hctr2_tfm_ctx *tctx = crypto_skcipher_ctx(tfm);
	struct hctr2_request_ctx *rctx = skcipher_request_ctx(req);
	u8 digest[POLYVAL_DIGEST_SIZE];
	int bulk_len = req->cryptlen - BLOCKCIPHER_BLOCK_SIZE;

	// Requests must be at least one block
	if (req->cryptlen < BLOCKCIPHER_BLOCK_SIZE)
		return -EINVAL;

	// Copy M (or U) into a temporary buffer
	scatterwalk_map_and_copy(rctx->first_block, req->src,
				 0, BLOCKCIPHER_BLOCK_SIZE, 0);

	// Create scatterlists for N and V
	rctx->bulk_part_src = scatterwalk_ffwd(rctx->sg_src, req->src,
					       BLOCKCIPHER_BLOCK_SIZE);
	rctx->bulk_part_dst = scatterwalk_ffwd(rctx->sg_dst, req->dst,
					       BLOCKCIPHER_BLOCK_SIZE);

	// MM = M ^ H(T || N)
	// or UU = U ^ H(T || V)
	hctr2_hash_tweak(req);
	hctr2_hash_message(req, rctx->bulk_part_src, digest);
	crypto_xor(digest, rctx->first_block, BLOCKCIPHER_BLOCK_SIZE);

	// UU = E(MM)
	// or MM = D(UU)
	if (enc)
		crypto_cipher_encrypt_one(tctx->blockcipher, rctx->first_block,
					  digest);
	else
		crypto_cipher_decrypt_one(tctx->blockcipher, rctx->first_block,
					  digest);

	// S = MM ^ UU ^ L
	crypto_xor(digest, rctx->first_block, BLOCKCIPHER_BLOCK_SIZE);
	crypto_xor_cpy(rctx->xctr_iv, digest, tctx->L, BLOCKCIPHER_BLOCK_SIZE);

	// V = XCTR(S, N)
	// or N = XCTR(S, V)
	skcipher_request_set_tfm(&rctx->u.xctr_req, tctx->xctr);
	skcipher_request_set_crypt(&rctx->u.xctr_req, rctx->bulk_part_src,
				   rctx->bulk_part_dst, bulk_len,
				   rctx->xctr_iv);
	skcipher_request_set_callback(&rctx->u.xctr_req,
				      req->base.flags,
				      hctr2_xctr_done, req);
	int err = crypto_skcipher_encrypt(&rctx->u.xctr_req);
	return err ? err : hctr2_finish(req);
}

static int hctr2_encrypt(struct skcipher_request *req)
{
	return hctr2_crypt(req, true);
}

static int hctr2_decrypt(struct skcipher_request *req)
{
	return hctr2_crypt(req, false);
}

static int hctr2_init_tfm(struct crypto_skcipher *tfm)
{
	struct hctr2_tfm_ctx *tctx = crypto_skcipher_ctx(tfm);
	struct crypto_skcipher *xctr;
	struct crypto_cipher *blockcipher;
	int err;

	xctr = crypto_alloc_skcipher("xctr(aes)", 0, 0);
	if (IS_ERR(xctr))
		return PTR_ERR(xctr);

	blockcipher = crypto_alloc_cipher("aes", 0, 0);
	if (IS_ERR(blockcipher)) {
		err = PTR_ERR(blockcipher);
		goto err_free_xctr;
	}

	tctx->xctr = xctr;
	tctx->blockcipher = blockcipher;

	BUILD_BUG_ON(offsetofend(struct hctr2_request_ctx, u) !=
				 sizeof(struct hctr2_request_ctx));
	crypto_skcipher_set_reqsize(
		tfm, max(sizeof(struct hctr2_request_ctx),
			 offsetofend(struct hctr2_request_ctx, u.xctr_req) +
				 crypto_skcipher_reqsize(xctr)));
	return 0;

err_free_xctr:
	crypto_free_skcipher(xctr);
	return err;
}

static void hctr2_exit_tfm(struct crypto_skcipher *tfm)
{
	struct hctr2_tfm_ctx *tctx = crypto_skcipher_ctx(tfm);

	crypto_free_cipher(tctx->blockcipher);
	crypto_free_skcipher(tctx->xctr);
}

struct crypto_skcipher *sevenzip_hctr2_alloc(void)
{
	struct crypto_skcipher *tfm = sevenzip_skcipher_create(sizeof(struct hctr2_tfm_ctx));
	if (IS_ERR(tfm)) return tfm;
	int err = hctr2_init_tfm(tfm);
	if (err) { crypto_free_skcipher(tfm); return ERR_PTR(err); }
	tfm->setkey = hctr2_setkey;
	tfm->encrypt = hctr2_encrypt;
	tfm->decrypt = hctr2_decrypt;
	tfm->exit = hctr2_exit_tfm;
	tfm->ivsize = TWEAK_SIZE;
	return tfm;
}

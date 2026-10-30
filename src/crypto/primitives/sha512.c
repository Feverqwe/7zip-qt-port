// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SHA-384, SHA-512, HMAC-SHA384, and HMAC-SHA512 library functions
 *
 * Copyright (c) Jean-Luc Cooke <jlcooke@certainkey.com>
 * Copyright (c) Andrew McDonald <andrew@mcdonald.org.uk>
 * Copyright (c) 2003 Kyle McMartin <kyle@debian.org>
 * Copyright 2025 Google LLC
 */

#include <crypto/hmac.h>
#include <crypto/sha2.h>
#include "crypto/primitives/sdkhashblocks.h"

static const struct sha512_block_state sha384_iv = {
	.h = {
		SHA384_H0, SHA384_H1, SHA384_H2, SHA384_H3,
		SHA384_H4, SHA384_H5, SHA384_H6, SHA384_H7,
	},
};

static const struct sha512_block_state sha512_iv = {
	.h = {
		SHA512_H0, SHA512_H1, SHA512_H2, SHA512_H3,
		SHA512_H4, SHA512_H5, SHA512_H6, SHA512_H7,
	},
};

/* The portable SDK already owns the compression algorithm in this module.
 * Keep Linux's stream/HMAC state and finalization around that implementation. */
static void __maybe_unused
sha512_blocks_generic(struct sha512_block_state *state,
		      const u8 *data, size_t nblocks)
{
	sevenzip_gpl_sha512_blocks(state->h, data, nblocks);
}

#ifdef CONFIG_CRYPTO_LIB_SHA512_ARCH
#include "sha512.h" /* $(SRCARCH)/sha512.h */
#else
#define sha512_blocks sha512_blocks_generic
#endif

static void __sha512_init(struct __sha512_ctx *ctx,
			  const struct sha512_block_state *iv,
			  u64 initial_bytecount)
{
	ctx->state = *iv;
	ctx->bytecount_lo = initial_bytecount;
	ctx->bytecount_hi = 0;
}

void sha384_init(struct sha384_ctx *ctx)
{
	__sha512_init(&ctx->ctx, &sha384_iv, 0);
}
EXPORT_SYMBOL_GPL(sha384_init);

void sha512_init(struct sha512_ctx *ctx)
{
	__sha512_init(&ctx->ctx, &sha512_iv, 0);
}
EXPORT_SYMBOL_GPL(sha512_init);

void __sha512_update(struct __sha512_ctx *ctx, const u8 *data, size_t len)
{
	size_t partial = ctx->bytecount_lo % SHA512_BLOCK_SIZE;

	if (check_add_overflow(ctx->bytecount_lo, len, &ctx->bytecount_lo))
		ctx->bytecount_hi++;

	if (partial + len >= SHA512_BLOCK_SIZE) {
		size_t nblocks;

		if (partial) {
			size_t l = SHA512_BLOCK_SIZE - partial;

			memcpy(&ctx->buf[partial], data, l);
			data += l;
			len -= l;

			sha512_blocks(&ctx->state, ctx->buf, 1);
		}

		nblocks = len / SHA512_BLOCK_SIZE;
		len %= SHA512_BLOCK_SIZE;

		if (nblocks) {
			sha512_blocks(&ctx->state, data, nblocks);
			data += nblocks * SHA512_BLOCK_SIZE;
		}
		partial = 0;
	}
	if (len)
		memcpy(&ctx->buf[partial], data, len);
}
EXPORT_SYMBOL_GPL(__sha512_update);

static void __sha512_final(struct __sha512_ctx *ctx,
			   u8 *out, size_t digest_size)
{
	u64 bitcount_hi = (ctx->bytecount_hi << 3) | (ctx->bytecount_lo >> 61);
	u64 bitcount_lo = ctx->bytecount_lo << 3;
	size_t partial = ctx->bytecount_lo % SHA512_BLOCK_SIZE;

	ctx->buf[partial++] = 0x80;
	if (partial > SHA512_BLOCK_SIZE - 16) {
		memset(&ctx->buf[partial], 0, SHA512_BLOCK_SIZE - partial);
		sha512_blocks(&ctx->state, ctx->buf, 1);
		partial = 0;
	}
	memset(&ctx->buf[partial], 0, SHA512_BLOCK_SIZE - 16 - partial);
	*(__be64 *)&ctx->buf[SHA512_BLOCK_SIZE - 16] = cpu_to_be64(bitcount_hi);
	*(__be64 *)&ctx->buf[SHA512_BLOCK_SIZE - 8] = cpu_to_be64(bitcount_lo);
	sha512_blocks(&ctx->state, ctx->buf, 1);

	for (size_t i = 0; i < digest_size; i += 8)
		put_unaligned_be64(ctx->state.h[i / 8], out + i);
}

void sha384_final(struct sha384_ctx *ctx, u8 out[SHA384_DIGEST_SIZE])
{
	__sha512_final(&ctx->ctx, out, SHA384_DIGEST_SIZE);
	memzero_explicit(ctx, sizeof(*ctx));
}
EXPORT_SYMBOL_GPL(sha384_final);

void sha512_final(struct sha512_ctx *ctx, u8 out[SHA512_DIGEST_SIZE])
{
	__sha512_final(&ctx->ctx, out, SHA512_DIGEST_SIZE);
	memzero_explicit(ctx, sizeof(*ctx));
}
EXPORT_SYMBOL_GPL(sha512_final);

void sha384(const u8 *data, size_t len, u8 out[SHA384_DIGEST_SIZE])
{
	struct sha384_ctx ctx;

	sha384_init(&ctx);
	sha384_update(&ctx, data, len);
	sha384_final(&ctx, out);
}
EXPORT_SYMBOL_GPL(sha384);

void sha512(const u8 *data, size_t len, u8 out[SHA512_DIGEST_SIZE])
{
	struct sha512_ctx ctx;

	sha512_init(&ctx);
	sha512_update(&ctx, data, len);
	sha512_final(&ctx, out);
}
EXPORT_SYMBOL_GPL(sha512);

static void __hmac_sha512_preparekey(struct sha512_block_state *istate,
				     struct sha512_block_state *ostate,
				     const u8 *raw_key, size_t raw_key_len,
				     const struct sha512_block_state *iv)
{
	union {
		u8 b[SHA512_BLOCK_SIZE];
		unsigned long w[SHA512_BLOCK_SIZE / sizeof(unsigned long)];
	} derived_key = { 0 };

	if (unlikely(raw_key_len > SHA512_BLOCK_SIZE)) {
		if (iv == &sha384_iv)
			sha384(raw_key, raw_key_len, derived_key.b);
		else
			sha512(raw_key, raw_key_len, derived_key.b);
	} else {
		memcpy(derived_key.b, raw_key, raw_key_len);
	}

	for (size_t i = 0; i < ARRAY_SIZE(derived_key.w); i++)
		derived_key.w[i] ^= REPEAT_BYTE(HMAC_IPAD_VALUE);
	*istate = *iv;
	sha512_blocks(istate, derived_key.b, 1);

	for (size_t i = 0; i < ARRAY_SIZE(derived_key.w); i++)
		derived_key.w[i] ^= REPEAT_BYTE(HMAC_OPAD_VALUE ^
						HMAC_IPAD_VALUE);
	*ostate = *iv;
	sha512_blocks(ostate, derived_key.b, 1);

	memzero_explicit(&derived_key, sizeof(derived_key));
}

void hmac_sha384_preparekey(struct hmac_sha384_key *key,
			    const u8 *raw_key, size_t raw_key_len)
{
	__hmac_sha512_preparekey(&key->key.istate, &key->key.ostate,
				 raw_key, raw_key_len, &sha384_iv);
}
EXPORT_SYMBOL_GPL(hmac_sha384_preparekey);

void hmac_sha512_preparekey(struct hmac_sha512_key *key,
			    const u8 *raw_key, size_t raw_key_len)
{
	__hmac_sha512_preparekey(&key->key.istate, &key->key.ostate,
				 raw_key, raw_key_len, &sha512_iv);
}
EXPORT_SYMBOL_GPL(hmac_sha512_preparekey);

void __hmac_sha512_init(struct __hmac_sha512_ctx *ctx,
			const struct __hmac_sha512_key *key)
{
	__sha512_init(&ctx->sha_ctx, &key->istate, SHA512_BLOCK_SIZE);
	ctx->ostate = key->ostate;
}
EXPORT_SYMBOL_GPL(__hmac_sha512_init);

void hmac_sha384_init_usingrawkey(struct hmac_sha384_ctx *ctx,
				  const u8 *raw_key, size_t raw_key_len)
{
	__hmac_sha512_preparekey(&ctx->ctx.sha_ctx.state, &ctx->ctx.ostate,
				 raw_key, raw_key_len, &sha384_iv);
	ctx->ctx.sha_ctx.bytecount_lo = SHA512_BLOCK_SIZE;
	ctx->ctx.sha_ctx.bytecount_hi = 0;
}
EXPORT_SYMBOL_GPL(hmac_sha384_init_usingrawkey);

void hmac_sha512_init_usingrawkey(struct hmac_sha512_ctx *ctx,
				  const u8 *raw_key, size_t raw_key_len)
{
	__hmac_sha512_preparekey(&ctx->ctx.sha_ctx.state, &ctx->ctx.ostate,
				 raw_key, raw_key_len, &sha512_iv);
	ctx->ctx.sha_ctx.bytecount_lo = SHA512_BLOCK_SIZE;
	ctx->ctx.sha_ctx.bytecount_hi = 0;
}
EXPORT_SYMBOL_GPL(hmac_sha512_init_usingrawkey);

static void __hmac_sha512_final(struct __hmac_sha512_ctx *ctx,
				u8 *out, size_t digest_size)
{
	/* Generate the padded input for the outer hash in ctx->sha_ctx.buf. */
	__sha512_final(&ctx->sha_ctx, ctx->sha_ctx.buf, digest_size);
	memset(&ctx->sha_ctx.buf[digest_size], 0,
	       SHA512_BLOCK_SIZE - digest_size);
	ctx->sha_ctx.buf[digest_size] = 0x80;
	*(__be32 *)&ctx->sha_ctx.buf[SHA512_BLOCK_SIZE - 4] =
		cpu_to_be32(8 * (SHA512_BLOCK_SIZE + digest_size));

	/* Compute the outer hash, which gives the HMAC value. */
	sha512_blocks(&ctx->ostate, ctx->sha_ctx.buf, 1);
	for (size_t i = 0; i < digest_size; i += 8)
		put_unaligned_be64(ctx->ostate.h[i / 8], out + i);

	memzero_explicit(ctx, sizeof(*ctx));
}

void hmac_sha384_final(struct hmac_sha384_ctx *ctx,
		       u8 out[SHA384_DIGEST_SIZE])
{
	__hmac_sha512_final(&ctx->ctx, out, SHA384_DIGEST_SIZE);
}
EXPORT_SYMBOL_GPL(hmac_sha384_final);

void hmac_sha512_final(struct hmac_sha512_ctx *ctx,
		       u8 out[SHA512_DIGEST_SIZE])
{
	__hmac_sha512_final(&ctx->ctx, out, SHA512_DIGEST_SIZE);
}
EXPORT_SYMBOL_GPL(hmac_sha512_final);

void hmac_sha384(const struct hmac_sha384_key *key,
		 const u8 *data, size_t data_len, u8 out[SHA384_DIGEST_SIZE])
{
	struct hmac_sha384_ctx ctx;

	hmac_sha384_init(&ctx, key);
	hmac_sha384_update(&ctx, data, data_len);
	hmac_sha384_final(&ctx, out);
}
EXPORT_SYMBOL_GPL(hmac_sha384);

void hmac_sha512(const struct hmac_sha512_key *key,
		 const u8 *data, size_t data_len, u8 out[SHA512_DIGEST_SIZE])
{
	struct hmac_sha512_ctx ctx;

	hmac_sha512_init(&ctx, key);
	hmac_sha512_update(&ctx, data, data_len);
	hmac_sha512_final(&ctx, out);
}
EXPORT_SYMBOL_GPL(hmac_sha512);

void hmac_sha384_usingrawkey(const u8 *raw_key, size_t raw_key_len,
			     const u8 *data, size_t data_len,
			     u8 out[SHA384_DIGEST_SIZE])
{
	struct hmac_sha384_ctx ctx;

	hmac_sha384_init_usingrawkey(&ctx, raw_key, raw_key_len);
	hmac_sha384_update(&ctx, data, data_len);
	hmac_sha384_final(&ctx, out);
}
EXPORT_SYMBOL_GPL(hmac_sha384_usingrawkey);

void hmac_sha512_usingrawkey(const u8 *raw_key, size_t raw_key_len,
			     const u8 *data, size_t data_len,
			     u8 out[SHA512_DIGEST_SIZE])
{
	struct hmac_sha512_ctx ctx;

	hmac_sha512_init_usingrawkey(&ctx, raw_key, raw_key_len);
	hmac_sha512_update(&ctx, data, data_len);
	hmac_sha512_final(&ctx, out);
}
EXPORT_SYMBOL_GPL(hmac_sha512_usingrawkey);

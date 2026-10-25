// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SHA-3, as specified in
 * https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.202.pdf
 *
 * SHA-3 code by Jeff Garzik <jeff@garzik.org>
 *               Ard Biesheuvel <ard.biesheuvel@linaro.org>
 *               David Howells <dhowells@redhat.com>
 *
 * See also Documentation/crypto/sha3.rst
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <crypto/sha3.h>
#include "sdkhashblocks.h"
#include <crypto/hmac.h>
#include <crypto/utils.h>

/* Generic Keccak permutation through the existing portable SDK library. */
static void sha3_keccakf_generic(struct sha3_state *state)
{
	for (int i = 0; i < ARRAY_SIZE(state->words); i++)
		state->native_words[i] = le64_to_cpu(state->words[i]);

	sevenzip_gpl_keccakf(state->native_words);

	for (int i = 0; i < ARRAY_SIZE(state->words); i++)
		state->words[i] = cpu_to_le64(state->native_words[i]);
}

/*
 * Generic implementation of absorbing the given nonzero number of full blocks
 * into the sponge function Keccak[r=8*block_size, c=1600-8*block_size].
 */
static void __maybe_unused
sha3_absorb_blocks_generic(struct sha3_state *state, const u8 *data,
			   size_t nblocks, size_t block_size)
{
	do {
		for (size_t i = 0; i < block_size; i += 8)
			state->words[i / 8] ^= get_unaligned((__le64 *)&data[i]);
		sha3_keccakf_generic(state);
		data += block_size;
	} while (--nblocks);
}

#ifdef CONFIG_CRYPTO_LIB_SHA3_ARCH
#include "sha3.h" /* $(SRCARCH)/sha3.h */
#else
#define sha3_keccakf		sha3_keccakf_generic
#define sha3_absorb_blocks	sha3_absorb_blocks_generic
#endif

void __sha3_update(struct __sha3_ctx *ctx, const u8 *in, size_t in_len)
{
	const size_t block_size = ctx->block_size;
	size_t absorb_offset = ctx->absorb_offset;

	/* Warn if squeezing has already begun. */
	WARN_ON_ONCE(absorb_offset >= block_size);

	if (absorb_offset && absorb_offset + in_len >= block_size) {
		crypto_xor(&ctx->state.bytes[absorb_offset], in,
			   block_size - absorb_offset);
		in += block_size - absorb_offset;
		in_len -= block_size - absorb_offset;
		sha3_keccakf(&ctx->state);
		absorb_offset = 0;
	}

	if (in_len >= block_size) {
		size_t nblocks = in_len / block_size;

		sha3_absorb_blocks(&ctx->state, in, nblocks, block_size);
		in += nblocks * block_size;
		in_len -= nblocks * block_size;
	}

	if (in_len) {
		crypto_xor(&ctx->state.bytes[absorb_offset], in, in_len);
		absorb_offset += in_len;
	}
	ctx->absorb_offset = absorb_offset;
}
EXPORT_SYMBOL_GPL(__sha3_update);

void sha3_final(struct sha3_ctx *sha3_ctx, u8 *out)
{
	struct __sha3_ctx *ctx = &sha3_ctx->ctx;

	ctx->state.bytes[ctx->absorb_offset] ^= 0x06;
	ctx->state.bytes[ctx->block_size - 1] ^= 0x80;
	sha3_keccakf(&ctx->state);
	memcpy(out, ctx->state.bytes, ctx->digest_size);
	sha3_zeroize_ctx(sha3_ctx);
}
EXPORT_SYMBOL_GPL(sha3_final);

void shake_squeeze(struct shake_ctx *shake_ctx, u8 *out, size_t out_len)
{
	struct __sha3_ctx *ctx = &shake_ctx->ctx;
	const size_t block_size = ctx->block_size;
	size_t squeeze_offset = ctx->squeeze_offset;

	if (ctx->absorb_offset < block_size) {
		/* First squeeze: */

		/* Add the domain separation suffix and padding. */
		ctx->state.bytes[ctx->absorb_offset] ^= 0x1f;
		ctx->state.bytes[block_size - 1] ^= 0x80;

		/* Indicate that squeezing has begun. */
		ctx->absorb_offset = block_size;

		/*
		 * Indicate that no output is pending yet, i.e. sha3_keccakf()
		 * will need to be called before the first copy.
		 */
		squeeze_offset = block_size;
	}
	while (out_len) {
		if (squeeze_offset == block_size) {
			sha3_keccakf(&ctx->state);
			squeeze_offset = 0;
		}
		size_t copy = min(out_len, block_size - squeeze_offset);

		memcpy(out, &ctx->state.bytes[squeeze_offset], copy);
		out += copy;
		out_len -= copy;
		squeeze_offset += copy;
	}
	ctx->squeeze_offset = squeeze_offset;
}
EXPORT_SYMBOL_GPL(shake_squeeze);

#ifndef sha3_224_arch
static inline bool sha3_224_arch(const u8 *in, size_t in_len,
				 u8 out[SHA3_224_DIGEST_SIZE])
{
	return false;
}
#endif
#ifndef sha3_256_arch
static inline bool sha3_256_arch(const u8 *in, size_t in_len,
				 u8 out[SHA3_256_DIGEST_SIZE])
{
	return false;
}
#endif
#ifndef sha3_384_arch
static inline bool sha3_384_arch(const u8 *in, size_t in_len,
				 u8 out[SHA3_384_DIGEST_SIZE])
{
	return false;
}
#endif
#ifndef sha3_512_arch
static inline bool sha3_512_arch(const u8 *in, size_t in_len,
				 u8 out[SHA3_512_DIGEST_SIZE])
{
	return false;
}
#endif

void sha3_224(const u8 *in, size_t in_len, u8 out[SHA3_224_DIGEST_SIZE])
{
	struct sha3_ctx ctx;

	if (sha3_224_arch(in, in_len, out))
		return;
	sha3_224_init(&ctx);
	sha3_update(&ctx, in, in_len);
	sha3_final(&ctx, out);
}
EXPORT_SYMBOL_GPL(sha3_224);

void sha3_256(const u8 *in, size_t in_len, u8 out[SHA3_256_DIGEST_SIZE])
{
	struct sha3_ctx ctx;

	if (sha3_256_arch(in, in_len, out))
		return;
	sha3_256_init(&ctx);
	sha3_update(&ctx, in, in_len);
	sha3_final(&ctx, out);
}
EXPORT_SYMBOL_GPL(sha3_256);

void sha3_384(const u8 *in, size_t in_len, u8 out[SHA3_384_DIGEST_SIZE])
{
	struct sha3_ctx ctx;

	if (sha3_384_arch(in, in_len, out))
		return;
	sha3_384_init(&ctx);
	sha3_update(&ctx, in, in_len);
	sha3_final(&ctx, out);
}
EXPORT_SYMBOL_GPL(sha3_384);

void sha3_512(const u8 *in, size_t in_len, u8 out[SHA3_512_DIGEST_SIZE])
{
	struct sha3_ctx ctx;

	if (sha3_512_arch(in, in_len, out))
		return;
	sha3_512_init(&ctx);
	sha3_update(&ctx, in, in_len);
	sha3_final(&ctx, out);
}
EXPORT_SYMBOL_GPL(sha3_512);

void shake128(const u8 *in, size_t in_len, u8 *out, size_t out_len)
{
	struct shake_ctx ctx;

	shake128_init(&ctx);
	shake_update(&ctx, in, in_len);
	shake_squeeze(&ctx, out, out_len);
	shake_zeroize_ctx(&ctx);
}
EXPORT_SYMBOL_GPL(shake128);

void shake256(const u8 *in, size_t in_len, u8 *out, size_t out_len)
{
	struct shake_ctx ctx;

	shake256_init(&ctx);
	shake_update(&ctx, in, in_len);
	shake_squeeze(&ctx, out, out_len);
	shake_zeroize_ctx(&ctx);
}
EXPORT_SYMBOL_GPL(shake256);


MODULE_DESCRIPTION("SHA-3 library functions");
MODULE_LICENSE("GPL");

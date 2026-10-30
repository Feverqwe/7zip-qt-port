// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2017-2019 Linaro Ltd <ard.biesheuvel@linaro.org>
 * Copyright 2026 Google LLC
 */

#include <crypto/aes-cbc.h>
#include <crypto/aes-ctr.h>
#include <crypto/aes-ecb.h>
#include <crypto/aes-xts.h>
#include <crypto/aes.h>
#include <crypto/gf128mul.h>
#include <crypto/utils.h>
#include "crypto/primitives/sdkaesblocks.h"

/* Linux keeps its key/state and mode APIs around the existing SDK blocks. */
static void aes_expandkey_generic(u32 rndkeys[], u32 *inv_rndkeys,
                                  const u8 *in_key, int key_len)
{
	sevenzip_gpl_aes_prepare(rndkeys, inv_rndkeys, in_key, key_len);
}

int aes_expandkey(struct crypto_aes_ctx *ctx, const u8 *in_key,
                  unsigned int key_len)
{
	if (aes_check_keylen(key_len) != 0)
		return -EINVAL;
	ctx->key_length = key_len;
	aes_expandkey_generic(ctx->key_enc, ctx->key_dec, in_key, key_len);
	return 0;
}
EXPORT_SYMBOL(aes_expandkey);

static void __maybe_unused aes_encrypt_generic(const u32 rndkeys[], int nrounds,
                                               u8 out[AES_BLOCK_SIZE],
                                               const u8 in[AES_BLOCK_SIZE])
{
	sevenzip_gpl_aes_block(rndkeys, nrounds, out, in, 0);
}

static void __maybe_unused aes_decrypt_generic(const u32 inv_rndkeys[], int nrounds,
                                               u8 out[AES_BLOCK_SIZE],
                                               const u8 in[AES_BLOCK_SIZE])
{
	sevenzip_gpl_aes_block(inv_rndkeys, nrounds, out, in, 1);
}

/*
 * Note: the aes_prepare*key_* names reflect the fact that the implementation
 * might not actually expand the key.  (The s390 code for example doesn't.)
 * Where the key is expanded we use the more specific names aes_expandkey_*.
 *
 * aes_preparekey_arch() is passed an optional pointer 'inv_k' which points to
 * the area to store the prepared decryption key.  It will be NULL if the user
 * is requesting encryption-only.  aes_preparekey_arch() is also passed a valid
 * 'key_len' and 'nrounds', corresponding to AES-128, AES-192, or AES-256.
 */
#ifdef CONFIG_CRYPTO_LIB_AES_ARCH
/* An arch-specific implementation of AES is available.  Include it. */
#include "aes.h" /* $(SRCARCH)/aes.h */
#else
/* No arch-specific implementation of AES is available.  Use generic code. */

static void aes_preparekey_arch(union aes_enckey_arch *k,
				union aes_invkey_arch *inv_k,
				const u8 *in_key, int key_len, int nrounds)
{
	aes_expandkey_generic(k->rndkeys, inv_k ? inv_k->inv_rndkeys : NULL,
			      in_key, key_len);
}

static void aes_encrypt_arch(const struct aes_enckey *key,
			     u8 out[AES_BLOCK_SIZE],
			     const u8 in[AES_BLOCK_SIZE])
{
	aes_encrypt_generic(key->k.rndkeys, key->nrounds, out, in);
}

static void aes_decrypt_arch(const struct aes_key *key,
			     u8 out[AES_BLOCK_SIZE],
			     const u8 in[AES_BLOCK_SIZE])
{
	aes_decrypt_generic(key->inv_k.inv_rndkeys, key->enc.nrounds, out, in);
}
#endif

static int __aes_preparekey(struct aes_enckey *enc_key,
			    union aes_invkey_arch *inv_k,
			    const u8 *in_key, size_t key_len)
{
	if (aes_check_keylen(key_len) != 0)
		return -EINVAL;
	enc_key->len = key_len;
	enc_key->nrounds = 6 + key_len / 4;
	aes_preparekey_arch(&enc_key->k, inv_k, in_key, key_len,
			    enc_key->nrounds);
	return 0;
}

int aes_preparekey(struct aes_key *key, const u8 *in_key, size_t key_len)
{
	return __aes_preparekey((struct aes_enckey *)key, &key->inv_k,
				in_key, key_len);
}
EXPORT_SYMBOL(aes_preparekey);

int aes_prepareenckey(struct aes_enckey *key, const u8 *in_key, size_t key_len)
{
	return __aes_preparekey(key, NULL, in_key, key_len);
}
EXPORT_SYMBOL(aes_prepareenckey);

void aes_encrypt(aes_encrypt_arg key, u8 out[AES_BLOCK_SIZE],
		 const u8 in[AES_BLOCK_SIZE])
{
	aes_encrypt_arch((const struct aes_enckey *)key, out, in);
}
EXPORT_SYMBOL(aes_encrypt);

void aes_decrypt(const struct aes_key *key, u8 out[AES_BLOCK_SIZE],
		 const u8 in[AES_BLOCK_SIZE])
{
	aes_decrypt_arch(key, out, in);
}
EXPORT_SYMBOL(aes_decrypt);


/*
 * Hooks for optimized AES-ECB implementations, overridable by the architecture.
 * They are called with len > 0 && len % AES_BLOCK_SIZE == 0.  Returning false
 * causes the fallback implementation to be used instead.
 */
#ifndef aes_ecb_encrypt_arch
static bool aes_ecb_encrypt_arch(u8 *dst, const u8 *src, size_t len,
				 const struct aes_enckey *key)
{
	return false;
}
#endif
#ifndef aes_ecb_decrypt_arch
static bool aes_ecb_decrypt_arch(u8 *dst, const u8 *src, size_t len,
				 const struct aes_key *key)
{
	return false;
}
#endif

void aes_ecb_encrypt(u8 *dst, const u8 *src, size_t len, aes_encrypt_arg key)
{
	if (WARN_ON_ONCE(len % AES_BLOCK_SIZE))
		len = round_down(len, AES_BLOCK_SIZE);

	if (unlikely(len == 0))
		return;

	if (likely(aes_ecb_encrypt_arch(dst, src, len, (const struct aes_enckey *)key)))
		return;

	for (size_t i = 0; i < len; i += AES_BLOCK_SIZE)
		aes_encrypt(key, &dst[i], &src[i]);
}
EXPORT_SYMBOL_GPL(aes_ecb_encrypt);

void aes_ecb_decrypt(u8 *dst, const u8 *src, size_t len,
		     const struct aes_key *key)
{
	if (WARN_ON_ONCE(len % AES_BLOCK_SIZE))
		len = round_down(len, AES_BLOCK_SIZE);

	if (unlikely(len == 0))
		return;

	if (likely(aes_ecb_decrypt_arch(dst, src, len, key)))
		return;

	for (size_t i = 0; i < len; i += AES_BLOCK_SIZE)
		aes_decrypt(key, &dst[i], &src[i]);
}
EXPORT_SYMBOL_GPL(aes_ecb_decrypt);

#ifndef aes_cbc_encrypt_arch
static bool aes_cbc_encrypt_arch(u8 *dst, const u8 *src, size_t len,
				 u8 iv[AES_BLOCK_SIZE],
				 const struct aes_enckey *key)
{
	return false;
}
#endif
#ifndef aes_cbc_decrypt_arch
static bool aes_cbc_decrypt_arch(u8 *dst, const u8 *src, size_t len,
				 u8 iv[AES_BLOCK_SIZE],
				 const struct aes_key *key)
{
	return false;
}
#endif

void aes_cbc_encrypt(u8 *dst, const u8 *src, size_t len, u8 iv[AES_BLOCK_SIZE],
		     aes_encrypt_arg key)
{
	const u8 *prev = iv;

	if (WARN_ON_ONCE(len % AES_BLOCK_SIZE))
		len = round_down(len, AES_BLOCK_SIZE);

	if (unlikely(len == 0))
		return;

	if (likely(aes_cbc_encrypt_arch(dst, src, len, iv, (const struct aes_enckey *)key)))
		return;

	do {
		crypto_xor_cpy(dst, src, prev, AES_BLOCK_SIZE);
		aes_encrypt(key, dst, dst);
		prev = dst;
		dst += AES_BLOCK_SIZE;
		src += AES_BLOCK_SIZE;
		len -= AES_BLOCK_SIZE;
	} while (len);
	memcpy(iv, prev, AES_BLOCK_SIZE);
}
EXPORT_SYMBOL_GPL(aes_cbc_encrypt);

void aes_cbc_decrypt(u8 *dst, const u8 *src, size_t len, u8 iv[AES_BLOCK_SIZE],
		     const struct aes_key *key)
{
	u8 next_iv[AES_BLOCK_SIZE];

	if (WARN_ON_ONCE(len % AES_BLOCK_SIZE))
		len = round_down(len, AES_BLOCK_SIZE);

	if (unlikely(len == 0))
		return;

	if (likely(aes_cbc_decrypt_arch(dst, src, len, iv, key)))
		return;

	len -= AES_BLOCK_SIZE;
	dst += len;
	src += len;
	memcpy(next_iv, src, AES_BLOCK_SIZE);
	for (;;) {
		aes_decrypt(key, dst, src);
		if (len == 0)
			break;
		src -= AES_BLOCK_SIZE;
		crypto_xor(dst, src, AES_BLOCK_SIZE);
		dst -= AES_BLOCK_SIZE;
		len -= AES_BLOCK_SIZE;
	}
	crypto_xor(dst, iv, AES_BLOCK_SIZE);
	memcpy(iv, next_iv, AES_BLOCK_SIZE);
}
EXPORT_SYMBOL_GPL(aes_cbc_decrypt);

/*
 * Hooks for optimized AES-CBC-CTS implementations, overridable by the
 * architecture.  They are called with len > AES_BLOCK_SIZE.  Returning false
 * causes the fallback implementation to be used instead.  The fallback
 * implementation still uses the arch-optimized AES-CBC code if available, but
 * direct implementation of AES-CBC-CTS is helpful on short messages.
 */
#ifndef aes_cbc_cts_encrypt_arch
static bool aes_cbc_cts_encrypt_arch(u8 *dst, const u8 *src, size_t len,
				     u8 iv[AES_BLOCK_SIZE],
				     const struct aes_enckey *key)
{
	return false;
}
#endif
#ifndef aes_cbc_cts_decrypt_arch
static bool aes_cbc_cts_decrypt_arch(u8 *dst, const u8 *src, size_t len,
				     u8 iv[AES_BLOCK_SIZE],
				     const struct aes_key *key)
{
	return false;
}
#endif

void aes_cbc_cts_encrypt(u8 *dst, const u8 *src, size_t len,
			 u8 iv[AES_BLOCK_SIZE], aes_encrypt_arg key)
{
	/* Offset to P[n] and C[n] (last plaintext and ciphertext block) */
	size_t pn_offset = round_down(len - 1, AES_BLOCK_SIZE);
	/* Length of P[n] and C[n], 1 <= pn_len <= AES_BLOCK_SIZE */
	size_t pn_len = len - pn_offset;
	_Alignas(sizeof(long)) u8 tmp[AES_BLOCK_SIZE];
	u8 *pad;

	if (WARN_ON_ONCE(len < AES_BLOCK_SIZE))
		return;

	if (len == AES_BLOCK_SIZE) {
		aes_cbc_encrypt(dst, src, len, iv, key);
		return;
	}
	if (likely(aes_cbc_cts_encrypt_arch(dst, src, len, iv, (const struct aes_enckey *)key)))
		return;

	/* CBC-encrypt all blocks except the last. */
	aes_cbc_encrypt(dst, src, pn_offset, iv, key);

	/*
	 * Compute C[n] and C[n - 1].
	 *
	 * Careful: src may equal dst (i.e., the encryption can be in-place), so
	 * src[pn_offset..] can't be read after dst[pn_offset..] is written.
	 */
	pad = &dst[pn_offset - AES_BLOCK_SIZE];
	memcpy(tmp, pad, AES_BLOCK_SIZE);
	crypto_xor(tmp, &src[pn_offset], pn_len);
	memcpy(&dst[pn_offset], pad, pn_len); /* C[n] */
	aes_encrypt(key, pad, tmp); /* C[n - 1] */

	memzero_explicit(tmp, sizeof(tmp));
}
EXPORT_SYMBOL_GPL(aes_cbc_cts_encrypt);

void aes_cbc_cts_decrypt(u8 *dst, const u8 *src, size_t len,
			 u8 iv[AES_BLOCK_SIZE], const struct aes_key *key)
{
	/* Offset to P[n] and C[n] (last plaintext and ciphertext block) */
	size_t pn_offset = round_down(len - 1, AES_BLOCK_SIZE);
	/* Length of P[n] and C[n], 1 <= pn_len <= AES_BLOCK_SIZE */
	size_t pn_len = len - pn_offset;
	u8 *pad;

	if (WARN_ON_ONCE(len < AES_BLOCK_SIZE))
		return;

	if (len == AES_BLOCK_SIZE) {
		aes_cbc_decrypt(dst, src, len, iv, key);
		return;
	}
	if (likely(aes_cbc_cts_decrypt_arch(dst, src, len, iv, key)))
		return;

	/* Compute P[0]..P[n - 2]. */
	aes_cbc_decrypt(dst, src, pn_offset - AES_BLOCK_SIZE, iv, key);

	/*
	 * Compute P[n] and P[n - 1].
	 *
	 * Careful: src may equal dst (i.e., the decryption can be in-place), so
	 * src[pn_offset..] can't be read after dst[pn_offset..] is written.
	 *
	 * To avoid needing a temporary buffer, do a "redundant" XOR to recover
	 * src[pn_offset..] from dst[pn_offset..] after the latter is written.
	 */
	pad = &dst[pn_offset - AES_BLOCK_SIZE];
	aes_decrypt(key, pad, &src[pn_offset - AES_BLOCK_SIZE]);
	crypto_xor_cpy(&dst[pn_offset], &src[pn_offset], pad,
		       pn_len); /* P[n] */
	crypto_xor(pad, &dst[pn_offset], pn_len);
	aes_decrypt(key, pad, pad);
	crypto_xor(pad, iv, AES_BLOCK_SIZE); /* P[n - 1] */
}
EXPORT_SYMBOL_GPL(aes_cbc_cts_decrypt);

#ifndef aes_ctr_arch
static bool aes_ctr_arch(u8 *dst, const u8 *src, size_t len,
			 u8 ctr[AES_BLOCK_SIZE], const struct aes_enckey *key)
{
	return false;
}
#endif
#ifndef aes_xctr_arch
static bool aes_xctr_arch(u8 *dst, const u8 *src, size_t len, u64 *ctr,
			  const u8 iv[AES_BLOCK_SIZE],
			  const struct aes_enckey *key)
{
	return false;
}
#endif

static __always_inline void inc_be128_ctr(u8 ctr[AES_BLOCK_SIZE])
{
	/*
	 * 255 times out of 256 the first iteration is enough, so unroll the
	 * first iteration as a micro-optimization.
	 */
	if ((++ctr[AES_BLOCK_SIZE - 1]) != 0)
		return;
	for (int i = AES_BLOCK_SIZE - 2; i >= 0; i--) {
		if (++ctr[i] != 0)
			break;
	}
}

void aes_ctr(u8 *dst, const u8 *src, size_t len, u8 ctr[AES_BLOCK_SIZE],
	     aes_encrypt_arg key)
{
	_Alignas(sizeof(long)) u8 keystream[AES_BLOCK_SIZE];

	if (likely(aes_ctr_arch(dst, src, len, ctr, (const struct aes_enckey *)key)))
		return;

	/* Handle the full blocks. */
	for (; len >= AES_BLOCK_SIZE; len -= AES_BLOCK_SIZE) {
		aes_encrypt(key, keystream, ctr);
		crypto_xor_cpy(dst, src, keystream, AES_BLOCK_SIZE);
		inc_be128_ctr(ctr);
		dst += AES_BLOCK_SIZE;
		src += AES_BLOCK_SIZE;
	}
	/* Handle any partial block at the end. */
	if (len) {
		aes_encrypt(key, keystream, ctr);
		crypto_xor_cpy(dst, src, keystream, len);
		/* Counter is incremented even with just a partial block. */
		inc_be128_ctr(ctr);
	}
	memzero_explicit(keystream, sizeof(keystream));
}
EXPORT_SYMBOL_GPL(aes_ctr);

void aes_xctr(u8 *dst, const u8 *src, size_t len, u64 *ctr,
	      const u8 iv[AES_BLOCK_SIZE], aes_encrypt_arg key)
{
	const __le64 iv0 = crypto_load_u64(&iv[0]);
	__le64 aes_input[2];
	_Alignas(sizeof(long)) u8 keystream[AES_BLOCK_SIZE];

	if (likely(aes_xctr_arch(dst, src, len, ctr, iv, (const struct aes_enckey *)key)))
		return;

	aes_input[1] = crypto_load_u64(&iv[8]);
	/* Handle the full blocks. */
	for (; len >= AES_BLOCK_SIZE; len -= AES_BLOCK_SIZE) {
		aes_input[0] = iv0 ^ cpu_to_le64((*ctr)++);
		aes_encrypt(key, keystream, (const u8 *)aes_input);
		crypto_xor_cpy(dst, src, keystream, AES_BLOCK_SIZE);
		dst += AES_BLOCK_SIZE;
		src += AES_BLOCK_SIZE;
	}
	/* Handle any partial block at the end. */
	if (len) {
		/* Counter is incremented even with just a partial block. */
		aes_input[0] = iv0 ^ cpu_to_le64((*ctr)++);
		aes_encrypt(key, keystream, (const u8 *)aes_input);
		crypto_xor_cpy(dst, src, keystream, len);
	}
	memzero_explicit(keystream, sizeof(keystream));
	memzero_explicit(aes_input, sizeof(aes_input));
}
EXPORT_SYMBOL_GPL(aes_xctr);

int aes_xts_preparekey(struct aes_xts_key *key, const u8 *in_key,
		       size_t key_len, int flags)
{
	int err;

	err = __xts_verify_key(in_key, key_len, flags);
	if (unlikely(err))
		goto out_zeroize;
	/* First half of XTS key is the main key */
	err = aes_preparekey(&key->main_key, in_key, key_len / 2);
	if (unlikely(err))
		goto out_zeroize;
	/* Second half of XTS key is the tweak key */
	err = aes_prepareenckey(&key->tweak_key, &in_key[key_len / 2],
				key_len / 2);
	if (unlikely(err))
		goto out_zeroize;
	return 0;

out_zeroize:
	memzero_explicit(key, sizeof(*key));
	return err;
}
EXPORT_SYMBOL_GPL(aes_xts_preparekey);

/*
 * Hooks for optimized AES-XTS implementations, overridable by the architecture.
 * They are called with len > 0 && len % AES_BLOCK_SIZE == 0.  In other words,
 * they aren't expected to handle ciphertext stealing or empty inputs.
 * Returning false causes the fallback implementation to be used instead.
 *
 * (Currently, all users of AES-XTS in the kernel seem to en/decrypt whole
 * numbers of blocks anyway, with len >= 512.  So there's no need to heavily
 * optimize ciphertext stealing for short messages.)
 */
#ifndef aes_xts_encrypt_arch
static bool aes_xts_encrypt_arch(u8 *dst, const u8 *src, size_t len,
				 u8 tweak[AES_BLOCK_SIZE],
				 const struct aes_xts_key *key, bool cont)
{
	return false;
}
#endif
#ifndef aes_xts_decrypt_arch
static bool aes_xts_decrypt_arch(u8 *dst, const u8 *src, size_t len,
				 u8 tweak[AES_BLOCK_SIZE],
				 const struct aes_xts_key *key, bool cont)
{
	return false;
}
#endif

static SEVENZIP_CRYPTO_NOINLINE void aes_xts_crypt_nocts_blockbyblock(
	u8 *dst, const u8 *src, size_t len, u8 tweak[AES_BLOCK_SIZE],
	const struct aes_xts_key *key, bool cont, bool enc)
{
	le128 t;

	if (cont)
		memcpy(&t, tweak, sizeof(t));
	else
		aes_encrypt(&key->tweak_key, (u8 *)&t, tweak);
	do {
		crypto_xor_cpy(dst, src, (const u8 *)&t, AES_BLOCK_SIZE);
		if (enc)
			aes_encrypt(&key->main_key, dst, dst);
		else
			aes_decrypt(&key->main_key, dst, dst);
		crypto_xor(dst, (const u8 *)&t, AES_BLOCK_SIZE);
		gf128mul_x_ble(&t, &t);
		dst += AES_BLOCK_SIZE;
		src += AES_BLOCK_SIZE;
		len -= AES_BLOCK_SIZE;
	} while (len);
	memcpy(tweak, &t, sizeof(t));
	memzero_explicit(&t, sizeof(t));
}

/* Requires len > 0 && len % AES_BLOCK_SIZE == 0 */
static __always_inline void aes_xts_encrypt_nocts(u8 *dst, const u8 *src,
						  size_t len,
						  u8 tweak[AES_BLOCK_SIZE],
						  const struct aes_xts_key *key,
						  bool cont)
{
	if (likely(aes_xts_encrypt_arch(dst, src, len, tweak, key, cont)))
		return;

	/*
	 * For the fallback, just go block-by-block.  It could be implemented on
	 * top of AES-ECB, which could be significantly faster than this if the
	 * arch has optimized AES-ECB code but not AES-XTS.  However, AES-XTS
	 * performance is important enough that it needs to be (and has been)
	 * implemented directly by every non-obsolete arch anyway.
	 */
	aes_xts_crypt_nocts_blockbyblock(dst, src, len, tweak, key, cont,
					 /* enc= */ true);
}

/* Requires len > 0 && len % AES_BLOCK_SIZE == 0 */
static __always_inline void aes_xts_decrypt_nocts(u8 *dst, const u8 *src,
						  size_t len,
						  u8 tweak[AES_BLOCK_SIZE],
						  const struct aes_xts_key *key,
						  bool cont)
{
	if (likely(aes_xts_decrypt_arch(dst, src, len, tweak, key, cont)))
		return;

	/* Just go block-by-block.  See comment in aes_xts_encrypt_nocts(). */
	aes_xts_crypt_nocts_blockbyblock(dst, src, len, tweak, key, cont,
					 /* enc= */ false);
}

static SEVENZIP_CRYPTO_NOINLINE void aes_xts_encrypt_cts(u8 *dst, const u8 *src, size_t len,
					 u8 tweak[AES_BLOCK_SIZE],
					 const struct aes_xts_key *key,
					 bool cont)
{
	size_t partial_len = len % AES_BLOCK_SIZE; /* Length of partial block */
	size_t nocts_len = round_down(len, AES_BLOCK_SIZE);
	_Alignas(sizeof(long)) u8 tmp_block[AES_BLOCK_SIZE];

	/* Encrypt all full blocks. */
	aes_xts_encrypt_nocts(dst, src, nocts_len, tweak, key, cont);
	dst += nocts_len - AES_BLOCK_SIZE;
	src += nocts_len - AES_BLOCK_SIZE;

	/*
	 * Swap the partial block with the first 'partial_len' bytes of the
	 * encrypted last full block.  Note that a temporary buffer is needed to
	 * support in-place encryption.
	 */
	memcpy(tmp_block, src + AES_BLOCK_SIZE, partial_len);
	memcpy(dst + AES_BLOCK_SIZE, dst, partial_len);
	memcpy(dst, tmp_block, partial_len);

	/* Encrypt the last full block again. */
	crypto_xor(dst, tweak, AES_BLOCK_SIZE);
	aes_encrypt(&key->main_key, dst, dst);
	crypto_xor(dst, tweak, AES_BLOCK_SIZE);
	memzero_explicit(tmp_block, sizeof(tmp_block));
}

static SEVENZIP_CRYPTO_NOINLINE void aes_xts_decrypt_cts(u8 *dst, const u8 *src, size_t len,
					 u8 tweak[AES_BLOCK_SIZE],
					 const struct aes_xts_key *key,
					 bool cont)
{
	size_t partial_len = len % AES_BLOCK_SIZE; /* Length of partial block */
	size_t nocts_len = round_down(len, AES_BLOCK_SIZE) - AES_BLOCK_SIZE;
	union { /* le128 already provides at least unsigned-long alignment. */
		u8 block[AES_BLOCK_SIZE];
		le128 tweak;
	} tmp;

	/*
	 * Decrypt all blocks except the last full block and the partial block.
	 * The last full block has to be handled specially because decryption
	 * ciphertext stealing uses the last two tweaks in reverse order.
	 *
	 * nocts_len == 0 is possible here, which aes_xts_decrypt_nocts()
	 * doesn't handle (so that the length doesn't get checked redundantly in
	 * the fast path).  So handle that case specially as well.
	 */
	if (nocts_len)
		aes_xts_decrypt_nocts(dst, src, nocts_len, tweak, key, cont);
	else if (!cont)
		aes_encrypt(&key->tweak_key, tweak, tweak);
	dst += nocts_len;
	src += nocts_len;

	/* Copy the tweak, advance it again, then decrypt last full block. */
	memcpy(&tmp.tweak, tweak, AES_BLOCK_SIZE);
	gf128mul_x_ble(&tmp.tweak, &tmp.tweak);
	crypto_xor_cpy(dst, src, tmp.block, AES_BLOCK_SIZE);
	aes_decrypt(&key->main_key, dst, dst);
	crypto_xor(dst, tmp.block, AES_BLOCK_SIZE);

	/*
	 * Swap the partial block with the first 'partial_len' bytes of the
	 * decrypted last full block.  Note that a temporary buffer is needed to
	 * support in-place decryption.
	 */
	memcpy(tmp.block, src + AES_BLOCK_SIZE, partial_len);
	memcpy(dst + AES_BLOCK_SIZE, dst, partial_len);
	memcpy(dst, tmp.block, partial_len);

	/* Decrypt the last full block again. */
	crypto_xor(dst, tweak, AES_BLOCK_SIZE);
	aes_decrypt(&key->main_key, dst, dst);
	crypto_xor(dst, tweak, AES_BLOCK_SIZE);
	memzero_explicit(&tmp, sizeof(tmp));
}

void aes_xts_encrypt(u8 *dst, const u8 *src, size_t len,
		     u8 tweak[AES_BLOCK_SIZE], const struct aes_xts_key *key,
		     bool cont)
{
	if (WARN_ON_ONCE(len < AES_BLOCK_SIZE))
		return;

	if (unlikely(len % AES_BLOCK_SIZE)) {
		aes_xts_encrypt_cts(dst, src, len, tweak, key, cont);
		return;
	}

	aes_xts_encrypt_nocts(dst, src, len, tweak, key, cont);
}
EXPORT_SYMBOL_GPL(aes_xts_encrypt);

void aes_xts_decrypt(u8 *dst, const u8 *src, size_t len,
		     u8 tweak[AES_BLOCK_SIZE], const struct aes_xts_key *key,
		     bool cont)
{
	if (WARN_ON_ONCE(len < AES_BLOCK_SIZE))
		return;

	if (unlikely(len % AES_BLOCK_SIZE)) {
		aes_xts_decrypt_cts(dst, src, len, tweak, key, cont);
		return;
	}

	aes_xts_decrypt_nocts(dst, src, len, tweak, key, cont);
}
EXPORT_SYMBOL_GPL(aes_xts_decrypt);

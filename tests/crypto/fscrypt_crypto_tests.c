/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Check the host-adapted Linux cipher routines against independent known
 * answers, including unaligned buffers, in-place reads and stream continuation. */
#include <crypto/aes-cbc.h>
#include <crypto/aes-ctr.h>
#include <crypto/aes-xts.h>
#include "crypto/fscrypt/fscrypt_private.h"
#include "crypto/modes/skciphercompat.h"
#include "C/Aes.h"
#include <stdio.h>
int fscrypt_support_tests(void);
int fscrypt_modes_tests(void);
int fscrypt_policy_tests(void);
int fscrypt_reader_tests(void);

enum mode { BLOCK, CBC, CTS, XTS, CTR, XCTR };
struct vector {
	const char *name;
	enum mode mode;
	size_t keylen, len;
	const char *key, *iv, *plain, *cipher, *iv_out;
};
static const struct vector vectors[] = {
#include "fixtures/fscrypt/aes-vectors.h"
};

static int check_provider(const struct vector *v, bool in_place, unsigned offset, bool decrypt)
{
	static const char *const names[] = {
		"ecb(aes)", "cbc(aes)", "cts(cbc(aes))", "xts(aes)", "ctr(aes)", "xctr(aes)"
	};
	struct crypto_skcipher *tfm = crypto_alloc_skcipher(names[v->mode], 0, 0);
	if (IS_ERR(tfm)) return 1;
	int result = 1;
	u8 rawkey[65], input[514], output[514], iv[16] = {0}, original_iv[16];
	memcpy(rawkey + 1, v->key, v->keylen);
	if (crypto_skcipher_setkey(tfm, rawkey + 1, v->keylen)) goto out;
	struct skcipher_request *req = skcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) goto out;
	memcpy(input + offset, decrypt ? v->cipher : v->plain, v->len);
	if (v->iv) memcpy(iv, v->iv, sizeof(iv));
	memcpy(original_iv, iv, sizeof(iv));
	struct scatterlist src, dst;
	sg_init_one(&src, input + offset, v->len);
	sg_init_one(&dst, in_place ? input + offset : output + offset, v->len);
	skcipher_request_set_crypt(req, &src, &dst, v->len, v->mode == BLOCK ? NULL : iv);
	int err = decrypt ? crypto_skcipher_decrypt(req) : crypto_skcipher_encrypt(req);
	result = err || memcmp(dst.data, decrypt ? v->plain : v->cipher, v->len);
	if (v->iv_out && memcmp(iv, v->iv_out, sizeof(iv))) result = 1;
	if ((v->mode == XTS || v->mode == XCTR) && memcmp(iv, original_iv, sizeof(iv))) result = 1;
	if (result) fprintf(stderr, "Crypto API %s: decrypt=%d alias=%d offset=%u err=%d\n",
			    v->name, decrypt, in_place, offset, err);
	skcipher_request_free(req);
out:
	memzero_explicit(rawkey, sizeof(rawkey));
	crypto_free_skcipher(tfm);
	return result;
}
struct hash_vector {
	const char *name;
	bool hmac;
	size_t keylen, len;
	const char *key, *plain, *digest;
};
static const struct hash_vector hash_vectors[] = {
#include "fixtures/fscrypt/hash-vectors.h"
};
struct hkdf_vector {
	unsigned keylen, context, infolen, outlen;
	const char *key, *info, *output;
};
static const struct hkdf_vector hkdf_vectors[] = {
#include "fixtures/fscrypt/hkdf-vectors.h"
};

static int check_hashes(void)
{
	u8 out[128];
	for (size_t i = 0; i < ARRAY_SIZE(hash_vectors); ++i) {
		const struct hash_vector *v = &hash_vectors[i];
		for (unsigned chunk = 1; chunk <= 129; chunk += 16) {
			struct hmac_sha512_key key;
			struct hmac_sha512_ctx hmac_ctx;
			struct sha512_ctx hash_ctx;
			if (v->hmac) {
				hmac_sha512_preparekey(&key, (const u8 *)v->key, v->keylen);
				hmac_sha512_init(&hmac_ctx, &key);
			} else {
				sha512_init(&hash_ctx);
			}
			for (size_t pos = 0; pos < v->len;) {
				size_t len = v->len - pos;
				if (len > chunk) len = chunk;
				if (v->hmac) hmac_sha512_update(&hmac_ctx, v->plain + pos, len);
				else sha512_update(&hash_ctx, (const u8 *)v->plain + pos, len);
				pos += len;
			}
			if (v->hmac) hmac_sha512_final(&hmac_ctx, out);
			else sha512_final(&hash_ctx, out);
			memzero_explicit(&key, sizeof(key));
			if (memcmp(out, v->digest, SHA512_DIGEST_SIZE)) {
				fprintf(stderr, "%s: chunk=%u\n", v->name, chunk);
				return 1;
			}
		}
	}
	for (size_t i = 0; i < ARRAY_SIZE(hkdf_vectors); ++i) {
		const struct hkdf_vector *v = &hkdf_vectors[i];
		struct hmac_sha512_key hkdf;
		fscrypt_init_hkdf(&hkdf, (const u8 *)v->key, v->keylen);
		fscrypt_hkdf_expand(&hkdf, v->context, (const u8 *)v->info, v->infolen, out, v->outlen);
		memzero_explicit(&hkdf, sizeof(hkdf));
		if (memcmp(out, v->output, v->outlen)) {
			fprintf(stderr, "fscrypt HKDF context %u\n", v->context);
			return 1;
		}
	}
	return 0;
}

static int check_vector(const struct vector *v, bool in_place, unsigned offset,
			bool decrypt, bool incremental)
{
	struct aes_key key;
	struct aes_xts_key xts;
	u8 input[514], output[514], rawkey[65], iv[16] = {0};
	u8 *src = input + offset;
	u8 *dst = in_place ? src : output + offset;
	const char *expected = decrypt ? v->plain : v->cipher;
	memcpy(rawkey + 1, v->key, v->keylen);
	memcpy(src, decrypt ? v->cipher : v->plain, v->len);
	if (v->iv)
		memcpy(iv, v->iv, sizeof(iv));
	int err = v->mode == XTS ? aes_xts_preparekey(&xts, rawkey + 1, v->keylen, 0) :
					 aes_preparekey(&key, rawkey + 1, v->keylen);
	if (err)
		return 1;
	u64 counter = 1;
	for (size_t pos = 0; pos < v->len;) {
		size_t len = incremental && pos == 0 ? 16 : v->len - pos;
		switch (v->mode) {
		case BLOCK:
			if (decrypt) aes_decrypt(&key, dst + pos, src + pos);
			else aes_encrypt(&key, dst + pos, src + pos);
			break;
		case CBC:
			if (decrypt) aes_cbc_decrypt(dst + pos, src + pos, len, iv, &key);
			else aes_cbc_encrypt(dst + pos, src + pos, len, iv, &key);
			break;
		case CTS:
			if (decrypt) aes_cbc_cts_decrypt(dst + pos, src + pos, len, iv, &key);
			else aes_cbc_cts_encrypt(dst + pos, src + pos, len, iv, &key);
			break;
		case XTS:
			if (decrypt) aes_xts_decrypt(dst + pos, src + pos, len, iv, &xts, pos != 0);
			else aes_xts_encrypt(dst + pos, src + pos, len, iv, &xts, pos != 0);
			break;
		case CTR:
			aes_ctr(dst + pos, src + pos, len, iv, &key);
			break;
		case XCTR:
			aes_xctr(dst + pos, src + pos, len, &counter, iv, &key);
			break;
		}
		pos += len;
	}
	err = memcmp(dst, expected, v->len) != 0;
	if (v->iv_out && memcmp(iv, v->iv_out, sizeof(iv)))
		err = 1;
	if (err)
		fprintf(stderr, "%s: decrypt=%d in_place=%d offset=%u incremental=%d\n",
			v->name, decrypt, in_place, offset, incremental);
	memzero_explicit(&key, sizeof(key));
	memzero_explicit(&xts, sizeof(xts));
	memzero_explicit(rawkey, sizeof(rawkey));
	return err;
}

static int check_aes_vectors(void)
{
	for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); ++i) {
		const struct vector *v = &vectors[i];
		for (unsigned flags = 0; flags < 16; ++flags) {
			bool incremental = (flags & 8) != 0;
			if (incremental && (v->len <= 16 || v->mode == CTS))
				continue;
			if (check_vector(v, (flags & 1) != 0, (flags >> 1) & 1,
					 (flags & 4) != 0, incremental))
				return 1;
			if (!incremental && check_provider(v, (flags & 1) != 0,
							(flags >> 1) & 1, (flags & 4) != 0))
				return 1;
		}
	}
	return 0;
}

int main(void)
{
	if (check_aes_vectors()) return 1;
	/* Check the supported software fallback even when normal SDK dispatch uses
	 * CPU instructions. The vectors still exercise Linux's public mode APIs. */
	AES_CODE_FUNC encode = g_AesCbc_Encode, decode = g_AesCbc_Decode;
	if (encode != AesCbc_Encode || decode != AesCbc_Decode) {
		g_AesCbc_Encode = AesCbc_Encode;
		g_AesCbc_Decode = AesCbc_Decode;
		int result = check_aes_vectors();
		g_AesCbc_Encode = encode;
		g_AesCbc_Decode = decode;
		if (result) return 1;
	}
	return check_hashes() || fscrypt_support_tests() || fscrypt_modes_tests() || fscrypt_policy_tests() ||
		fscrypt_reader_tests();
}

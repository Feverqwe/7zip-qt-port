/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Host adaptation checks for fscrypt name hashing, inode IVs and SM4 blocks. */
#include <crypto/sha2.h>
#include <crypto/sm4.h>
#include <linux/siphash.h>
#include <linux/base64.h>
#include <stdio.h>

static const struct { size_t len; const char *plain, *digest; } sha_vectors[] = {
#include "fixtures/fscrypt/sha256-vectors.h"
};
static const struct { size_t keylen, len; const char *key, *plain, *cipher; } sm4_vectors[] = {
#include "fixtures/fscrypt/sm4-vectors.h"
};
static const struct { size_t len; const char *data, *encoded; } base64_vectors[] = {
#include "fixtures/fscrypt/base64-vectors.h"
};
#include "fixtures/fscrypt/siphash-vectors.h"

static int check_sha256(void)
{
	u8 digest[SHA256_DIGEST_SIZE];
	for (size_t i = 0; i < ARRAY_SIZE(sha_vectors); ++i) {
		for (unsigned chunk = 1; chunk <= 65; chunk += 16) {
			struct sha256_ctx ctx;
			sha256_init(&ctx);
			for (size_t pos = 0; pos < sha_vectors[i].len;) {
				size_t len = sha_vectors[i].len - pos;
				if (len > chunk) len = chunk;
				sha256_update(&ctx, (const u8 *)sha_vectors[i].plain + pos, len);
				pos += len;
			}
			sha256_final(&ctx, digest);
			if (memcmp(digest, sha_vectors[i].digest, sizeof(digest))) return 1;
		}
	}
	return 0;
}

static int check_sm4(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(sm4_vectors); ++i) {
		struct sm4_ctx key;
		u8 rawkey[17], input[513], output[513];
		memcpy(rawkey + 1, sm4_vectors[i].key, sm4_vectors[i].keylen);
		if (sm4_expandkey(&key, rawkey + 1, sm4_vectors[i].keylen)) return 1;
		for (unsigned flags = 0; flags < 4; ++flags) {
			bool decrypt = (flags & 1) != 0;
			u8 *dst = flags & 2 ? input + 1 : output + 1;
			const char *src = decrypt ? sm4_vectors[i].cipher : sm4_vectors[i].plain;
			const char *expected = decrypt ? sm4_vectors[i].plain : sm4_vectors[i].cipher;
			memcpy(input + 1, src, sm4_vectors[i].len);
			for (size_t pos = 0; pos < sm4_vectors[i].len; pos += SM4_BLOCK_SIZE)
				sm4_crypt_block(decrypt ? key.rkey_dec : key.rkey_enc,
						dst + pos, input + 1 + pos);
			if (memcmp(dst, expected, sm4_vectors[i].len)) return 1;
		}
		memzero_explicit(&key, sizeof(key));
		memzero_explicit(rawkey, sizeof(rawkey));
	}
	return 0;
}

static int check_siphash(void)
{
	_Alignas(8) u8 bytes[65];
	for (unsigned offset = 0; offset <= 1; ++offset) {
		for (unsigned i = 0; i < 64; ++i) bytes[offset + i] = i;
		for (unsigned len = 0; len < 64; ++len)
			if (siphash(bytes + offset, len, &test_key_siphash) != test_vectors_siphash[len])
				return 1;
	}
	return siphash_1u64(0x0706050403020100ULL, &test_key_siphash) != test_vectors_siphash[8];
}

static int check_base64(void)
{
	char encoded[400];
	u8 decoded[300];
	for (size_t i = 0; i < ARRAY_SIZE(base64_vectors); ++i) {
		int len = base64_encode((const u8 *)base64_vectors[i].data,
					base64_vectors[i].len, encoded, false, BASE64_URLSAFE);
		if ((size_t)len != strlen(base64_vectors[i].encoded) ||
		    memcmp(encoded, base64_vectors[i].encoded, len)) return 1;
		int bytes = base64_decode(encoded, len, decoded, false, BASE64_URLSAFE);
		if (bytes != (int)base64_vectors[i].len ||
		    memcmp(decoded, base64_vectors[i].data, bytes)) return 1;
	}
	/* Invalid bytes must never produce a successful no-key name lookup. */
	for (unsigned byte = 0; byte < 256; ++byte) {
		if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
		    (byte >= '0' && byte <= '9') || byte == '-' || byte == '_') continue;
		for (unsigned pos = 0; pos < 4; ++pos) {
			memcpy(encoded, "AAAA", 4);
			encoded[pos] = (char)byte;
			if (base64_decode(encoded, 4, decoded, false, BASE64_URLSAFE) != -1) return 1;
		}
	}
	const char *invalid[] = {"A", "AB", "AAB", "AA=", "AA==", "A/A_", "A+A_"};
	for (size_t i = 0; i < ARRAY_SIZE(invalid); ++i)
		if (base64_decode(invalid[i], strlen(invalid[i]), decoded, false, BASE64_URLSAFE) != -1)
			return 1;
	return 0;
}

int fscrypt_support_tests(void)
{
	if (check_sha256()) { fputs("SHA-256 known answer failed\n", stderr); return 1; }
	if (check_sm4()) { fputs("SM4 known answer failed\n", stderr); return 1; }
	if (check_siphash()) { fputs("SipHash known answer failed\n", stderr); return 1; }
	if (check_base64()) { fputs("base64url name codec failed\n", stderr); return 1; }
	return 0;
}

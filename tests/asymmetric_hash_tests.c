/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Independent digest answers across padding/rate boundaries and split updates. */
#include <crypto/hash.h>
#include <crypto/sha3.h>
#include <stdio.h>

static const struct { const char *algorithm; unsigned size; const char *hex; } vectors[] = {
#include "fixtures/asn1/hash-vectors.h"
};
static bool matches(const u8 *digest, const char *hex, unsigned size)
{
	static const char digits[] = "0123456789abcdef";
	if (strlen(hex) != size * 2) return false;
	for (unsigned i = 0; i < size; ++i)
		if (hex[i * 2] != digits[digest[i] >> 4] || hex[i * 2 + 1] != digits[digest[i] & 15])
			return false;
	return true;
}
static int check_shake(void)
{
	static const struct { unsigned bits, size; const char *hex; } cases[] = {
#include "fixtures/asn1/shake-vectors.h"
	};
	u8 input[258], output[400];
	for (unsigned i = 0; i < sizeof(input) - 1; ++i)
		input[i + 1] = (u8)(i * 37 + 11);
	for (unsigned i = 0; i < ARRAY_SIZE(cases); ++i) {
		struct shake_ctx ctx;
		if (cases[i].bits == 128) shake128_init(&ctx);
		else shake256_init(&ctx);
		unsigned first = min(cases[i].size, 17);
		shake_update(&ctx, input + 1, first);
		shake_update(&ctx, input + 1 + first, cases[i].size - first);
		struct shake_ctx copy = ctx;
		shake_squeeze(&ctx, output, sizeof(output));
		if (!matches(output, cases[i].hex, sizeof(output))) return 1;
		/* Continuation must match a single squeeze across multiple rate blocks. */
		ctx = copy;
		for (unsigned pos = 0; pos < sizeof(output);) {
			unsigned count = min((unsigned)sizeof(output) - pos, 31);
			shake_squeeze(&ctx, output + pos, count);
			pos += count;
		}
		if (!matches(output, cases[i].hex, sizeof(output))) return 1;
		shake_zeroize_ctx(&ctx);
		shake_zeroize_ctx(&copy);
	}
	return 0;
}

int main(void)
{
	if (check_shake()) { fprintf(stderr, "SHAKE streaming/continuation mismatch\n"); return 1; }
	u8 input[4098], output[64];
	for (unsigned i = 0; i < sizeof(input) - 1; ++i) input[i + 1] = (u8)(i * 37 + 11);
	for (size_t i = 0; i < ARRAY_SIZE(vectors); ++i) {
		struct crypto_shash *hash = crypto_alloc_shash(vectors[i].algorithm, 0, 0);
		if (IS_ERR(hash)) return 1;
		unsigned size = crypto_shash_digestsize(hash);
		int result = crypto_shash_tfm_digest(hash, input + 1, vectors[i].size, output);
		if (result || !matches(output, vectors[i].hex, size)) {
			fprintf(stderr, "%s %u one-shot digest mismatch\n", vectors[i].algorithm, vectors[i].size);
			crypto_free_shash(hash);
			return 1;
		}
		for (unsigned chunk = 1; chunk <= 131; chunk += 65) {
			struct shash_desc desc = {.tfm = hash};
			result = crypto_shash_init(&desc);
			for (unsigned pos = 0; !result && pos < vectors[i].size;) {
				unsigned part = min(chunk, vectors[i].size - pos);
				result = crypto_shash_update(&desc, input + 1 + pos, part);
				pos += part;
			}
			if (!result) result = crypto_shash_final(&desc, output);
			if (result || !matches(output, vectors[i].hex, size)) {
				fprintf(stderr, "%s %u split digest mismatch at chunk %u\n",
					vectors[i].algorithm, vectors[i].size, chunk);
				crypto_free_shash(hash);
				return 1;
			}
		}
		crypto_free_shash(hash);
	}
	/* An unavailable algorithm must not silently select a different digest. */
	struct crypto_shash *unknown = crypto_alloc_shash("unavailable-hash", 0, 0);
	if (!IS_ERR(unknown)) { crypto_free_shash(unknown); return 1; }
	return PTR_ERR(unknown) == -ENOENT ? 0 : 1;
}

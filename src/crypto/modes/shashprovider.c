/* SPDX-License-Identifier: GPL-2.0-only */
/* Fixed owned providers for the original Linux synchronous hash read path. */
#include <crypto/hash.h>

#define HASH_ADAPTER(name, member, prefix) \
	static int name##_init(struct shash_desc *desc) \
	{ prefix##_init(&desc->state.member); return 0; } \
	static int name##_update(struct shash_desc *desc, const u8 *data, unsigned size) \
	{ prefix##_update(&desc->state.member, data, size); return 0; } \
	static int name##_final(struct shash_desc *desc, u8 *out) \
	{ prefix##_final(&desc->state.member, out); return 0; }
HASH_ADAPTER(provider_sha1, sha1, sha1)
HASH_ADAPTER(provider_sha224, sha224, sha224)
HASH_ADAPTER(provider_sha256, sha256, sha256)
HASH_ADAPTER(provider_sha384, sha384, sha384)
HASH_ADAPTER(provider_sha512, sha512, sha512)
HASH_ADAPTER(provider_sm3, sm3, sm3)
#undef HASH_ADAPTER
static int provider_sha3_update(struct shash_desc *desc, const u8 *data, unsigned size)
{ sha3_update(&desc->state.sha3, data, size); return 0; }
static int provider_sha3_final(struct shash_desc *desc, u8 *out)
{ sha3_final(&desc->state.sha3, out); return 0; }

#define SHA3_INIT(bits) \
	static int provider_sha3_##bits##_init(struct shash_desc *desc) \
	{ sha3_##bits##_init(&desc->state.sha3); return 0; }
SHA3_INIT(224)
SHA3_INIT(256)
SHA3_INIT(384)
SHA3_INIT(512)
#undef SHA3_INIT

#define HASH_ALGORITHM(name, size, prefix, context, block) \
	{.init = prefix##_init, .update = prefix##_update, .final = prefix##_final, \
	 .descsize = sizeof(struct context), .digestsize = size, \
	 .base = {.cra_name = name, .cra_blocksize = block}}
#define SHA3_ALGORITHM(bits) \
	{.init = provider_sha3_##bits##_init, .update = provider_sha3_update, .final = provider_sha3_final, \
	 .descsize = sizeof(struct sha3_ctx), .digestsize = SHA3_##bits##_DIGEST_SIZE, \
	 .base = {.cra_name = "sha3-" #bits, .cra_blocksize = SHA3_##bits##_BLOCK_SIZE}}
static const struct shash_alg algorithms[] = {
	HASH_ALGORITHM("sha1", SHA1_DIGEST_SIZE, provider_sha1, sha1_ctx, SHA1_BLOCK_SIZE),
	HASH_ALGORITHM("sha224", SHA224_DIGEST_SIZE, provider_sha224, sha224_ctx, SHA224_BLOCK_SIZE),
	HASH_ALGORITHM("sha256", SHA256_DIGEST_SIZE, provider_sha256, sha256_ctx, SHA256_BLOCK_SIZE),
	HASH_ALGORITHM("sha384", SHA384_DIGEST_SIZE, provider_sha384, sha384_ctx, SHA384_BLOCK_SIZE),
	HASH_ALGORITHM("sha512", SHA512_DIGEST_SIZE, provider_sha512, sha512_ctx, SHA512_BLOCK_SIZE),
	HASH_ALGORITHM("sm3", SM3_DIGEST_SIZE, provider_sm3, sm3_ctx, SM3_BLOCK_SIZE),
	SHA3_ALGORITHM(224), SHA3_ALGORITHM(256), SHA3_ALGORITHM(384), SHA3_ALGORITHM(512),
};
#undef HASH_ALGORITHM
#undef SHA3_ALGORITHM

struct crypto_shash *crypto_alloc_shash(const char *name, unsigned type, unsigned mask)
{
	(void)type; (void)mask;
	const struct shash_alg *algorithm = NULL;
	for (size_t i = 0; i < ARRAY_SIZE(algorithms); ++i)
		if (!strcmp(name, algorithms[i].base.cra_name)) { algorithm = &algorithms[i]; break; }
	if (!strcmp(name, "streebog256")) algorithm = sevenzip_streebog_alg(STREEBOG256_DIGEST_SIZE);
	if (!strcmp(name, "streebog512")) algorithm = sevenzip_streebog_alg(STREEBOG512_DIGEST_SIZE);
	if (!algorithm) return ERR_PTR(-ENOENT);
	struct crypto_shash *hash = sevenzip_crypto_zalloc(sizeof(*hash));
	if (!hash) return ERR_PTR(-ENOMEM);
	hash->algorithm = *algorithm;
	/* Fixed-provider subset of shash_prepare_alg; per-allocation defaults
	 * avoid mutable global registration and preserve block-only buffering. */
	if (!hash->algorithm.finup) hash->algorithm.finup = shash_default_finup;
	if (!hash->algorithm.digest) hash->algorithm.digest = shash_default_digest;
	if (hash->algorithm.base.cra_flags & CRYPTO_AHASH_ALG_BLOCK_ONLY)
		hash->algorithm.descsize += hash->algorithm.base.cra_blocksize + 1;
	if (hash->algorithm.descsize > sizeof(union sevenzip_shash_state)) {
		sevenzip_crypto_free_sensitive(hash);
		return ERR_PTR(-EINVAL);
	}
	return hash;
}
void crypto_free_shash(struct crypto_shash *hash)
{
	if (!IS_ERR(hash)) sevenzip_crypto_free_sensitive(hash);
}
unsigned crypto_shash_digestsize(const struct crypto_shash *hash)
{ return hash->algorithm.digestsize; }
unsigned crypto_shash_descsize(const struct crypto_shash *hash)
{ return hash->algorithm.descsize; }

/* SPDX-License-Identifier: GPL-2.0-only */
/* Published Linux known answers cover math/encoding, digest truncation and the
 * owned public-key contract; corruptions cover observable verification errors. */
#include <crypto/sig.h>
#include <crypto/public_key.h>
#include <linux/oid_registry.h>

struct ecc_vector {
	const char *name, *key, *digest, *signature, *params;
	unsigned key_len, digest_len, signature_len, param_len, algo;
};
#include "fixtures/asn1/ecc-vectors.h"

static int check_vector(const struct ecc_vector *v)
{
	struct crypto_sig *tfm = crypto_alloc_sig(v->name, 0, 0);
	if (IS_ERR(tfm)) return 1;
	int result = 1;
	const char *step = "public key";
	u8 key[256], signature[160], digest[64];
	if (v->key_len + 8 + v->param_len > sizeof(key) ||
	    v->signature_len > sizeof(signature) || v->digest_len > sizeof(digest)) goto out;
	memcpy(key, v->key, v->key_len);
	/* Native u32 trailer is the original public_key.c provider contract. */
	u32 algo = v->algo, param_len = v->param_len;
	memcpy(key + v->key_len, &algo, sizeof(algo));
	memcpy(key + v->key_len + 4, &param_len, sizeof(param_len));
	memcpy(key + v->key_len + 8, v->params, v->param_len);
	if (crypto_sig_set_pubkey(tfm, key, v->key_len)) goto out;
	memset(key, 0, sizeof(key));
	step = "known answer after input-key wipe";
	if (crypto_sig_verify(tfm, v->signature, v->signature_len, v->digest, v->digest_len)) goto out;
	char algorithm[32];
	const char *encoding, *hash;
	if (!strcmp(v->name, "ecrdsa")) {
		strcpy(algorithm, "ecrdsa"); encoding = "raw";
		hash = v->digest_len == 32 ? "streebog256" : "streebog512";
	} else {
		const char *start = strchr(v->name, '(') + 1;
		size_t size = strlen(start) - 1;
		memcpy(algorithm, start, size); algorithm[size] = 0;
		encoding = !strncmp(v->name, "x962(", 5) ? "x962" : "p1363";
		hash = v->digest_len == 20 ? "sha1" : v->digest_len == 28 ? "sha224" :
			v->digest_len == 32 ? "sha256" : v->digest_len == 48 ? "sha384" : "sha512";
	}
	struct public_key public_key = {
		.key = (void *)v->key, .keylen = v->key_len, .pkey_algo = algorithm,
		.params = (void *)v->params, .paramlen = v->param_len, .algo = v->algo,
	};
	struct public_key_signature public_signature = {
		.s = (u8 *)v->signature, .s_size = v->signature_len,
		.m = (u8 *)v->digest, .m_size = v->digest_len,
		.pkey_algo = !strcmp(algorithm, "ecrdsa") ? "ecrdsa" : "ecdsa",
		.hash_algo = hash, .encoding = encoding,
	};
	step = "software public-key dispatch";
	if (public_key_verify_signature(&public_key, &public_signature)) goto out;
	step = "disallowed hash";
	public_signature.hash_algo = "md5";
	if (public_key_verify_signature(&public_key, &public_signature) != -EINVAL) goto out;
	public_signature.hash_algo = hash;
	step = "disallowed encoding";
	public_signature.encoding = "pkcs1";
	if (public_key_verify_signature(&public_key, &public_signature) != -EINVAL) goto out;
	step = "altered digest";
	memcpy(digest, v->digest, v->digest_len);
	digest[0] ^= 1;
	if (!crypto_sig_verify(tfm, v->signature, v->signature_len, digest, v->digest_len)) goto out;
	step = "altered signature";
	memcpy(signature, v->signature, v->signature_len);
	signature[v->signature_len - 1] ^= 1;
	if (!crypto_sig_verify(tfm, signature, v->signature_len, v->digest, v->digest_len)) goto out;
	step = "truncated signature";
	if (!crypto_sig_verify(tfm, v->signature, v->signature_len - 1, v->digest, v->digest_len)) goto out;
	if (!strncmp(v->name, "p1363(", 6) || !strcmp(v->name, "ecrdsa")) {
		step = "zero scalar signature";
		memset(signature, 0, sizeof(signature));
		if (!crypto_sig_verify(tfm, signature, v->signature_len, v->digest, v->digest_len)) goto out;
	}
	if (!strcmp(v->name, "ecrdsa")) {
		step = "wrong digest length";
		if (!crypto_sig_verify(tfm, v->signature, v->signature_len, v->digest, v->digest_len - 1)) goto out;
	}
	/* ECDSA's setter explicitly resets an old key before parsing a new one. */
	if (strcmp(v->name, "ecrdsa")) {
		step = "off-curve key and failed replacement";
		memset(key, 0, sizeof(key)); key[0] = 4;
		if (!crypto_sig_set_pubkey(tfm, key, v->key_len) ||
		    crypto_sig_verify(tfm, v->signature, v->signature_len, v->digest, v->digest_len) != -EINVAL)
			goto out;
		step = "compressed key rejection";
		memcpy(key, v->key, v->key_len); key[0] = 2;
		if (!crypto_sig_set_pubkey(tfm, key, v->key_len)) goto out;
		step = "key replacement recovery";
		if (crypto_sig_set_pubkey(tfm, v->key, v->key_len) ||
		    crypto_sig_verify(tfm, v->signature, v->signature_len, v->digest, v->digest_len)) goto out;
	}
	result = 0;
out:
	crypto_free_sig(tfm);
	if (result) fprintf(stderr, "%s/%u: %s failed\n", v->name, v->digest_len, step);
	return result;
}

int asymmetric_ecc_tests(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(ecc_vectors); ++i)
		if (check_vector(&ecc_vectors[i])) return 1;
	return 0;
}

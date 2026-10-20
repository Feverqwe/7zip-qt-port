/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Independent signatures exercise MPI conversion, modulus geometry and padding. */
#include <crypto/sig.h>
#include <crypto/public_key.h>
#include <stdio.h>
enum rsa_expected { RSA_VALID = 0, RSA_INVALID = -EINVAL,
	RSA_MALFORMED = -EBADMSG, RSA_REJECTED = -EKEYREJECTED };
struct rsa_vector {
	const char *name, *hash;
	const u8 *key;
	unsigned key_size, signature_size, digest_size;
	enum rsa_expected expected;
	const char *digest, *signature;
};
#include "fixtures/asn1/rsa-vectors.h"

static int check_rsa(const struct rsa_vector *vector)
{
	char algorithm[128];
	snprintf(algorithm, sizeof(algorithm), "pkcs1(rsa,%s)", vector->hash);
	struct crypto_sig *tfm = crypto_alloc_sig(algorithm, 0, 0);
	if (IS_ERR(tfm)) { fprintf(stderr, "%s allocation: %d\n", vector->name, PTR_ERR(tfm)); return 1; }
	int result = 1;
	int err = crypto_sig_set_pubkey(tfm, vector->key, vector->key_size);
	if (err) { fprintf(stderr, "%s key: %d\n", vector->name, err); goto out; }
	err = crypto_sig_verify(tfm, vector->signature, vector->signature_size,
		vector->digest, vector->digest_size);
	if (err != vector->expected) {
		fprintf(stderr, "%s verification: %d expected %d\n", vector->name, err, vector->expected);
		goto out;
	}
	struct public_key public_key = {
		.key = (void *)vector->key, .keylen = vector->key_size, .pkey_algo = "rsa",
	};
	struct public_key_signature signature = {
		.s = (u8 *)vector->signature, .s_size = vector->signature_size,
		.m = (u8 *)vector->digest, .m_size = vector->digest_size,
		.pkey_algo = "rsa", .hash_algo = vector->hash, .encoding = "pkcs1",
	};
	if (public_key_verify_signature(&public_key, &signature) != vector->expected) goto out;
	signature.pkey_algo = "ecdsa";
	if (public_key_verify_signature(&public_key, &signature) != -EKEYREJECTED) goto out;
	if (vector->expected == RSA_VALID) {
		if (crypto_sig_verify(tfm, vector->signature, vector->signature_size - 1,
			vector->digest, vector->digest_size) != -EINVAL ||
		    crypto_sig_verify(tfm, vector->signature, vector->signature_size,
			vector->digest, vector->digest_size - 1) != -EINVAL) goto out;
	}
	if (!strcmp(vector->name, "2048-sha256")) {
		/* A failed key replacement must not keep authenticating with an old key. */
		if (!crypto_sig_set_pubkey(tfm, vector->key, vector->key_size - 1) ||
		    crypto_sig_verify(tfm, vector->signature, vector->signature_size,
			vector->digest, vector->digest_size) != -EINVAL ||
		    crypto_sig_set_pubkey(tfm, vector->key, vector->key_size) ||
		    crypto_sig_verify(tfm, vector->signature, vector->signature_size,
			vector->digest, vector->digest_size)) goto out;
	}
	result = 0;
out:
	crypto_free_sig(tfm);
	return result;
}
int asymmetric_rsa_tests(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(rsa_vectors); ++i)
		if (check_rsa(&rsa_vectors[i])) return 1;
	return 0;
}

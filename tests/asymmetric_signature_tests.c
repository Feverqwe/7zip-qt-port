/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Known signatures, altered content and malformed lengths through Linux ML-DSA. */
#include <crypto/mldsa.h>
#include <crypto/sig.h>
#include <crypto/public_key.h>
#include <stdio.h>
#include "../third_party/linux/lib/crypto/tests/mldsa-testvecs.h"

static int check_mldsa(const struct mldsa_testvector *vector)
{
	int ret = mldsa_verify(vector->alg, vector->sig, vector->sig_len,
		vector->msg, vector->msg_len, vector->pk, vector->pk_len);
	if (ret) { fprintf(stderr, "ML-DSA %d known signature: %d\n", vector->alg, ret); return 1; }
	u8 changed_sig[MLDSA87_SIGNATURE_SIZE], changed_key[MLDSA87_PUBLIC_KEY_SIZE];
	memcpy(changed_sig, vector->sig, vector->sig_len);
	changed_sig[0] ^= 1;
	ret = mldsa_verify(vector->alg, changed_sig, vector->sig_len,
		vector->msg, vector->msg_len, vector->pk, vector->pk_len);
	if (ret != -EKEYREJECTED) { fprintf(stderr, "ML-DSA %d altered commitment: %d\n", vector->alg, ret); return 1; }
	memcpy(changed_sig, vector->sig, vector->sig_len);
	changed_sig[vector->sig_len - 1] = 255;
	ret = mldsa_verify(vector->alg, changed_sig, vector->sig_len,
		vector->msg, vector->msg_len, vector->pk, vector->pk_len);
	if (ret != -EBADMSG) { fprintf(stderr, "ML-DSA %d malformed hint: %d\n", vector->alg, ret); return 1; }
	memcpy(changed_key, vector->pk, vector->pk_len);
	changed_key[0] ^= 1;
	ret = mldsa_verify(vector->alg, vector->sig, vector->sig_len,
		vector->msg, vector->msg_len, changed_key, vector->pk_len);
	if (ret != -EKEYREJECTED) { fprintf(stderr, "ML-DSA %d altered key: %d\n", vector->alg, ret); return 1; }
	ret = mldsa_verify(vector->alg, vector->sig, vector->sig_len,
		(const u8 *)"foreign message", 15, vector->pk, vector->pk_len);
	if (ret != -EKEYREJECTED) { fprintf(stderr, "ML-DSA %d altered message: %d\n", vector->alg, ret); return 1; }
	if (mldsa_verify(vector->alg, vector->sig, vector->sig_len - 1,
		vector->msg, vector->msg_len, vector->pk, vector->pk_len) != -EBADMSG ||
	    mldsa_verify(vector->alg, vector->sig, vector->sig_len,
		vector->msg, vector->msg_len, vector->pk, vector->pk_len - 1) != -EBADMSG)
		return 1;
	return 0;
}
static int check_mldsa_provider(const struct mldsa_testvector *vector, const char *name)
{
	struct crypto_sig *tfm = crypto_alloc_sig(name, 0, 0);
	if (IS_ERR(tfm)) return 1;
	int result = 1;
	u8 input[MLDSA87_PUBLIC_KEY_SIZE];
	memcpy(input, vector->pk, vector->pk_len);
	if (crypto_sig_set_pubkey(tfm, input, vector->pk_len - 1) != -EINVAL ||
	    crypto_sig_verify(tfm, vector->sig, vector->sig_len, vector->msg, vector->msg_len) != -EINVAL ||
	    crypto_sig_set_pubkey(tfm, input, vector->pk_len)) goto out;
	/* A configured transform owns its key independently of the input buffer. */
	memset(input, 0, sizeof(input));
	if (crypto_sig_verify(tfm, vector->sig, vector->sig_len, vector->msg, vector->msg_len)) goto out;
	struct public_key public_key = {
		.key = (void *)vector->pk, .keylen = vector->pk_len, .pkey_algo = name,
	};
	struct public_key_signature signature = {
		.s = (u8 *)vector->sig, .s_size = vector->sig_len,
		.m = (u8 *)vector->msg, .m_size = vector->msg_len,
		.pkey_algo = name, .hash_algo = "none", .encoding = "raw",
	};
	if (public_key_verify_signature(&public_key, &signature)) goto out;
	signature.hash_algo = "sha256";
	if (public_key_verify_signature(&public_key, &signature) != -EINVAL) goto out;
	result = 0;
out:
	crypto_free_sig(tfm);
	if (result) fprintf(stderr, "%s provider ownership/key-state failure\n", name);
	return result;
}
int asymmetric_rsa_tests(void);
int asymmetric_ecc_tests(void);
int main(void)
{
	return check_mldsa(&mldsa44_testvector) || check_mldsa(&mldsa65_testvector) ||
		check_mldsa(&mldsa87_testvector) ||
		check_mldsa_provider(&mldsa44_testvector, "mldsa44") ||
		check_mldsa_provider(&mldsa65_testvector, "mldsa65") ||
		check_mldsa_provider(&mldsa87_testvector, "mldsa87") || asymmetric_rsa_tests() || asymmetric_ecc_tests();
}

// SPDX-License-Identifier: GPL-2.0-or-later
/* In-software asymmetric public-key crypto subtype
 *
 * See Documentation/crypto/asymmetric-keys.rst
 *
 * Copyright (C) 2012 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 */

#include "crypto/certificates/userspace.h"
#include <crypto/internal/sig.h>
#include <linux/keyctl.h>
#include <keys/asymmetric-subtype.h>
#include <crypto/public_key.h>

/*
 * Destroy a public key algorithm key.
 */
void public_key_free(struct public_key *key)
{
	if (key) {
		kfree_sensitive(key->key);
		kfree(key->params);
		kfree(key);
	}
}
EXPORT_SYMBOL_GPL(public_key_free);

/*
 * Given a public_key, and an encoding and hash_algo to be used for signing
 * and/or verification with that key, determine the name of the corresponding
 * akcipher algorithm.  Also check that encoding and hash_algo are allowed.
 */
static int
software_key_determine_akcipher(const struct public_key *pkey,
				const char *encoding, const char *hash_algo,
				char alg_name[CRYPTO_MAX_ALG_NAME], bool *sig,
				enum kernel_pkey_operation op)
{
	int n;

	*sig = true;

	if (!encoding)
		return -EINVAL;

	if (strcmp(pkey->pkey_algo, "rsa") == 0) {
		/*
		 * RSA signatures usually use EMSA-PKCS1-1_5 [RFC3447 sec 8.2].
		 */
		if (strcmp(encoding, "pkcs1") == 0) {
			*sig = op == kernel_pkey_sign ||
			       op == kernel_pkey_verify;
			if (!*sig) {
				/*
				 * For encrypt/decrypt, hash_algo is not used
				 * but allowed to be set for historic reasons.
				 */
				n = snprintf(alg_name, CRYPTO_MAX_ALG_NAME,
					     "pkcs1pad(%s)",
					     pkey->pkey_algo);
			} else {
				if (!hash_algo)
					hash_algo = "none";
				n = snprintf(alg_name, CRYPTO_MAX_ALG_NAME,
					     "pkcs1(%s,%s)",
					     pkey->pkey_algo, hash_algo);
			}
			return n >= CRYPTO_MAX_ALG_NAME ? -EINVAL : 0;
		}
		if (strcmp(encoding, "raw") != 0)
			return -EINVAL;
		/*
		 * Raw RSA cannot differentiate between different hash
		 * algorithms.
		 */
		if (hash_algo)
			return -EINVAL;
		*sig = false;
	} else if (strncmp(pkey->pkey_algo, "ecdsa", 5) == 0) {
		if (strcmp(encoding, "x962") != 0 &&
		    strcmp(encoding, "p1363") != 0)
			return -EINVAL;
		/*
		 * ECDSA signatures are taken over a raw hash, so they don't
		 * differentiate between different hash algorithms.  That means
		 * that the verifier should hard-code a specific hash algorithm.
		 * Unfortunately, in practice ECDSA is used with multiple SHAs,
		 * so we have to allow all of them and not just one.
		 */
		if (!hash_algo)
			return -EINVAL;
		if (strcmp(hash_algo, "sha1") != 0 &&
		    strcmp(hash_algo, "sha224") != 0 &&
		    strcmp(hash_algo, "sha256") != 0 &&
		    strcmp(hash_algo, "sha384") != 0 &&
		    strcmp(hash_algo, "sha512") != 0 &&
		    strcmp(hash_algo, "sha3-256") != 0 &&
		    strcmp(hash_algo, "sha3-384") != 0 &&
		    strcmp(hash_algo, "sha3-512") != 0)
			return -EINVAL;
		n = snprintf(alg_name, CRYPTO_MAX_ALG_NAME, "%s(%s)",
			     encoding, pkey->pkey_algo);
		return n >= CRYPTO_MAX_ALG_NAME ? -EINVAL : 0;
	} else if (strcmp(pkey->pkey_algo, "ecrdsa") == 0) {
		if (strcmp(encoding, "raw") != 0)
			return -EINVAL;
		if (!hash_algo)
			return -EINVAL;
		if (strcmp(hash_algo, "streebog256") != 0 &&
		    strcmp(hash_algo, "streebog512") != 0)
			return -EINVAL;
	} else if (strcmp(pkey->pkey_algo, "mldsa44") == 0 ||
		   strcmp(pkey->pkey_algo, "mldsa65") == 0 ||
		   strcmp(pkey->pkey_algo, "mldsa87") == 0) {
		if (strcmp(encoding, "raw") != 0)
			return -EINVAL;
		if (!hash_algo)
			return -EINVAL;
		if (strcmp(hash_algo, "none") != 0 &&
		    strcmp(hash_algo, "sha512") != 0)
			return -EINVAL;
	} else {
		/* Unknown public key algorithm */
		return -ENOPKG;
	}
	if (snprintf(alg_name, CRYPTO_MAX_ALG_NAME, "%s", pkey->pkey_algo) >= CRYPTO_MAX_ALG_NAME)
		return -EINVAL;
	return 0;
}

static u8 *pkey_pack_u32(u8 *dst, u32 val)
{
	memcpy(dst, &val, sizeof(val));
	return dst + sizeof(val);
}

/*
 * Verify a signature using a public key.
 */
int public_key_verify_signature(const struct public_key *pkey,
				const struct public_key_signature *sig)
{
	char alg_name[CRYPTO_MAX_ALG_NAME];
	struct crypto_sig *tfm;
	u8 *key, *ptr;
	bool issig;
	int ret;

	pr_devel("==>%s()\n", __func__);

	BUG_ON(!pkey);
	BUG_ON(!sig);
	BUG_ON(!sig->s);

	/*
	 * If the signature specifies a public key algorithm, it *must* match
	 * the key's actual public key algorithm.
	 *
	 * Small exception: ECDSA signatures don't specify the curve, but ECDSA
	 * keys do.  So the strings can mismatch slightly in that case:
	 * "ecdsa-nist-*" for the key, but "ecdsa" for the signature.
	 */
	if (sig->pkey_algo) {
		if (strcmp(pkey->pkey_algo, sig->pkey_algo) != 0 &&
		    (strncmp(pkey->pkey_algo, "ecdsa-", 6) != 0 ||
		     strcmp(sig->pkey_algo, "ecdsa") != 0))
			return -EKEYREJECTED;
	}

	ret = software_key_determine_akcipher(pkey, sig->encoding,
					      sig->hash_algo, alg_name,
					      &issig, kernel_pkey_verify);
	if (ret < 0)
		return ret;

	tfm = crypto_alloc_sig(alg_name, 0, 0);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);

	key = kmalloc(pkey->keylen + sizeof(u32) * 2 + pkey->paramlen,
		      GFP_KERNEL);
	if (!key) {
		ret = -ENOMEM;
		goto error_free_tfm;
	}

	memcpy(key, pkey->key, pkey->keylen);
	ptr = key + pkey->keylen;
	ptr = pkey_pack_u32(ptr, pkey->algo);
	ptr = pkey_pack_u32(ptr, pkey->paramlen);
	if (pkey->paramlen)
		memcpy(ptr, pkey->params, pkey->paramlen);

	if (pkey->key_is_private)
		ret = crypto_sig_set_privkey(tfm, key, pkey->keylen);
	else
		ret = crypto_sig_set_pubkey(tfm, key, pkey->keylen);
	if (ret)
		goto error_free_key;

	ret = crypto_sig_verify(tfm, sig->s, sig->s_size, sig->m, sig->m_size);

error_free_key:
	kfree_sensitive(key);
error_free_tfm:
	crypto_free_sig(tfm);
	pr_devel("<==%s() = %d\n", __func__, ret);
	if (WARN_ON_ONCE(ret > 0))
		ret = -EINVAL;
	return ret;
}
EXPORT_SYMBOL_GPL(public_key_verify_signature);

static int public_key_verify_signature_2(const struct key *key,
					 const struct public_key_signature *sig)
{
	const struct public_key *pk = key->payload.data[asym_crypto];
	return public_key_verify_signature(pk, sig);
}

/* Selected immutable subtype services for archive verification. */
const struct asymmetric_key_subtype public_key_subtype = {
	.name = "public_key",
	.name_len = sizeof("public_key") - 1,
	.verify_signature = public_key_verify_signature_2,
};

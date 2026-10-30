// SPDX-License-Identifier: GPL-2.0-or-later
/* Instantiate a public key crypto key from an X.509 Certificate
 *
 * Copyright (C) 2012 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 */

#include <crypto/hash.h>
#include <keys/system_keyring.h>
#include "crypto/certificates/x509_parser.h"

/*
 * Set up the signature parameters in an X.509 certificate.  This involves
 * digesting the signed data and extracting the signature.
 */
int x509_get_sig_params(struct x509_certificate *cert)
{
	struct public_key_signature *sig = cert->sig;
	struct crypto_shash *tfm;
	struct shash_desc *desc;
	size_t desc_size;
	int ret;

	pr_devel("==>%s()\n", __func__);

	/* Calculate a SHA256 hash of the TBS and check it against the
	 * blacklist.
	 */
	sha256(cert->tbs, cert->tbs_size, cert->sha256);
	ret = is_hash_blacklisted(cert->sha256, sizeof(cert->sha256),
				  BLACKLIST_HASH_X509_TBS);
	if (ret == -EKEYREJECTED) {
		pr_err("Cert %*phN is blacklisted\n",
		       (int)sizeof(cert->sha256), cert->sha256);
		cert->blacklisted = true;
		ret = 0;
	}

	sig->s = kmemdup(cert->raw_sig, cert->raw_sig_size, GFP_KERNEL);
	if (!sig->s)
		return -ENOMEM;

	sig->s_size = cert->raw_sig_size;

	if (sig->algo_takes_data) {
		/* The signature algorithm does whatever passes for hashing. */
		sig->m = (u8 *)cert->tbs;
		sig->m_size = cert->tbs_size;
		sig->m_free = false;
		goto out;
	}

	/* Allocate the hashing algorithm we're going to need and find out how
	 * big the hash operational data will be.
	 */
	tfm = crypto_alloc_shash(sig->hash_algo, 0, 0);
	if (IS_ERR(tfm)) {
		if (PTR_ERR(tfm) == -ENOENT) {
			cert->unsupported_sig = true;
			return 0;
		}
		return PTR_ERR(tfm);
	}

	desc_size = crypto_shash_descsize(tfm) + sizeof(*desc);
	sig->m_size = crypto_shash_digestsize(tfm);

	ret = -ENOMEM;
	sig->m = kmalloc(sig->m_size, GFP_KERNEL);
	if (!sig->m)
		goto error;
	sig->m_free = true;

	desc = kzalloc(desc_size, GFP_KERNEL);
	if (!desc)
		goto error;

	desc->tfm = tfm;

	ret = crypto_shash_digest(desc, cert->tbs, cert->tbs_size, sig->m);
	if (ret < 0)
		goto error_2;

error_2:
	kfree(desc);
error:
	crypto_free_shash(tfm);
out:
	pr_devel("<==%s() = %d\n", __func__, ret);
	return ret;
}

/*
 * Check for self-signedness in an X.509 cert and if found, check the signature
 * immediately if we can.
 */
int x509_check_for_self_signed(struct x509_certificate *cert)
{
	int ret = 0;

	pr_devel("==>%s()\n", __func__);

	if (cert->raw_subject_size != cert->raw_issuer_size ||
	    memcmp(cert->raw_subject, cert->raw_issuer,
		   cert->raw_issuer_size) != 0)
		goto not_self_signed;

	if (cert->sig->auth_ids[0] || cert->sig->auth_ids[1]) {
		/* If the AKID is present it may have one or two parts.  If
		 * both are supplied, both must match.
		 */
		bool a = asymmetric_key_id_same(cert->skid, cert->sig->auth_ids[1]);
		bool b = asymmetric_key_id_same(cert->id, cert->sig->auth_ids[0]);

		if (!a && !b)
			goto not_self_signed;

		ret = -EKEYREJECTED;
		if (((a && !b) || (b && !a)) &&
		    cert->sig->auth_ids[0] && cert->sig->auth_ids[1])
			goto out;
	}

	if (cert->unsupported_sig) {
		ret = 0;
		goto out;
	}

	ret = public_key_verify_signature(cert->pub, cert->sig);
	if (ret < 0) {
		if (ret == -ENOPKG) {
			cert->unsupported_sig = true;
			ret = 0;
		}
		goto out;
	}

	pr_devel("Cert Self-signature verified");
	cert->self_signed = true;

out:
	pr_devel("<==%s() = %d\n", __func__, ret);
	return ret;

not_self_signed:
	pr_devel("<==%s() = 0 [not]\n", __func__);
	return 0;
}

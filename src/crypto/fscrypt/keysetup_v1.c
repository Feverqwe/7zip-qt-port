// SPDX-License-Identifier: GPL-2.0
/*
 * Key setup for v1 encryption policies
 *
 * Copyright 2015, 2019 Google LLC
 */

/*
 * This file implements compatibility functions for the original encryption
 * policy version ("v1"), including:
 *
 * - Deriving per-file encryption keys using the AES-128-ECB based KDF
 *   (rather than the new method of using HKDF-SHA512)
 *
 * - Retrieving fscrypt master keys from process-subscribed keyrings
 *   (rather than the new method of using a filesystem-level keyring)
 *
 * - Handling policies with the DIRECT_KEY flag set using a master key table
 *   (rather than the new method of implementing DIRECT_KEY with per-mode keys
 *    managed alongside the master keys in the filesystem-level keyring)
 */

#include "crypto/fscrypt/fscrypt_private.h"
#include <crypto/aes.h>

/* Archive read subset: the original derivation/selection is retained below;
 * process keyrings and the shared direct-key table described above are omitted. */

/* An archive CI owns its direct transform instead of a global kernel cache. */
static int setup_v1_file_key_direct(struct fscrypt_inode_info *ci,
				    const u8 *raw_master_key)
{
	return fscrypt_set_per_file_enc_key(ci, raw_master_key);
}

/*
 * v1 policy, !DIRECT_KEY: derive the file's encryption key.
 *
 * The v1 key derivation function generates the derived key by encrypting the
 * master key with AES-128-ECB using the file's nonce as the AES key.  This
 * provides a unique derived key with sufficient entropy for each inode.
 * However, it's nonstandard, non-extensible, doesn't evenly distribute the
 * entropy from the master key, and is trivially reversible: an attacker who
 * compromises a derived key can "decrypt" it to get back to the master key,
 * then derive any other key.  For all new code, use HKDF instead.
 *
 * The master key must be at least as long as the derived key.  If the master
 * key is longer, then only the first ci->ci_mode->keysize bytes are used.
 */
static int setup_v1_file_key_derived(struct fscrypt_inode_info *ci,
				     const u8 *raw_master_key)
{
	const unsigned int derived_keysize = ci->ci_mode->keysize;
	u8 derived_key[FSCRYPT_MAX_RAW_KEY_SIZE];
	struct aes_enckey aes;
	int err;

	if (WARN_ON_ONCE(derived_keysize > FSCRYPT_MAX_RAW_KEY_SIZE ||
			 derived_keysize % AES_BLOCK_SIZE != 0))
		return -EINVAL;

	static_assert(FSCRYPT_FILE_NONCE_SIZE == AES_KEYSIZE_128);
	aes_prepareenckey(&aes, ci->ci_nonce, FSCRYPT_FILE_NONCE_SIZE);
	for (unsigned int i = 0; i < derived_keysize; i += AES_BLOCK_SIZE)
		aes_encrypt(&aes, &derived_key[i], &raw_master_key[i]);

	err = fscrypt_set_per_file_enc_key(ci, derived_key);

	memzero_explicit(derived_key, sizeof(derived_key));
	/* No need to zeroize 'aes', as its key is not secret. */
	return err;
}

int fscrypt_setup_v1_file_key(struct fscrypt_inode_info *ci,
			      const u8 *raw_master_key)
{
	if (ci->ci_policy.v1.flags & FSCRYPT_POLICY_FLAG_DIRECT_KEY)
		return setup_v1_file_key_direct(ci, raw_master_key);
	else
		return setup_v1_file_key_derived(ci, raw_master_key);
}

// SPDX-License-Identifier: GPL-2.0-only
/*
 * This contains encryption functions for per-file encryption.
 *
 * Copyright (C) 2015, Google, Inc.
 * Copyright (C) 2015, Motorola Mobility
 *
 * Written by Michael Halcrow, 2014.
 *
 * Filename encryption additions
 *	Uday Savagaonkar, 2014
 * Encryption policy handling additions
 *	Ildar Muslukhov, 2014
 * Add fscrypt_pullback_bio_page()
 *	Jaegeuk Kim, 2015.
 *
 * This has not yet undergone a rigorous security audit.
 *
 * The usage of AES-XTS should conform to recommendations in NIST
 * Special Publication 800-38E and IEEE P1619/D16.
 */

#include "fscrypt_private.h"

void fscrypt_generate_iv(union fscrypt_iv *iv, u64 index,
			 const struct fscrypt_inode_info *ci)
{
	u8 flags = fscrypt_policy_flags(&ci->ci_policy);

	memset(iv, 0, sizeof(*iv));

	if (flags & FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64) {
		WARN_ON_ONCE(index > U32_MAX);
		WARN_ON_ONCE(ci->ci_inode->i_ino > U32_MAX);
		index |= (u64)ci->ci_inode->i_ino << 32;
	} else if (flags & FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32) {
		WARN_ON_ONCE(index > U32_MAX);
		index = (u32)(ci->ci_hashed_ino + index);
	} else if (flags & FSCRYPT_POLICY_FLAG_DIRECT_KEY) {
		memcpy(iv->nonce, ci->ci_nonce, FSCRYPT_FILE_NONCE_SIZE);
	}
	iv->index = cpu_to_le64(index);
}

/* Encrypt or decrypt a single "data unit" of file contents. */
int fscrypt_crypt_data_unit(const struct fscrypt_inode_info *ci,
				   fscrypt_direction_t rw, u64 index,
				   struct page *src_page,
				   struct page *dest_page, unsigned int len,
				   unsigned int offs)
{
	struct crypto_sync_skcipher *tfm;
	union fscrypt_iv iv;
	struct scatterlist dst, src;
	int err;

	if (WARN_ON_ONCE(ci == NULL)) /* File hasn't been opened yet? */
		return -ENOKEY;
	tfm = ci->ci_enc_key.tfm;
	if (WARN_ON_ONCE(tfm == NULL)) /* Called on block-based filesystem? */
		return -ENOKEY;

	if (WARN_ON_ONCE(len <= 0))
		return -EINVAL;
	if (WARN_ON_ONCE(len % FSCRYPT_CONTENTS_ALIGNMENT != 0))
		return -EINVAL;

	fscrypt_generate_iv(&iv, index, ci);

	{
		struct skcipher_request *req = skcipher_request_alloc(tfm, GFP_KERNEL);
		if (!req) return -ENOMEM;
		skcipher_request_set_callback(req,
					      CRYPTO_TFM_REQ_MAY_BACKLOG |
						      CRYPTO_TFM_REQ_MAY_SLEEP,
					      NULL, NULL);
		sg_init_table(&dst, 1);
		sg_set_page(&dst, dest_page, len, offs);
		sg_init_table(&src, 1);
		sg_set_page(&src, src_page, len, offs);
		skcipher_request_set_crypt(req, &src, &dst, len, &iv);
		if (rw == FS_DECRYPT)
			err = crypto_skcipher_decrypt(req);
		else
			err = crypto_skcipher_encrypt(req);
		skcipher_request_free(req);
	}
	if (err)
		fscrypt_err(ci->ci_inode,
			    "%scryption failed for data unit %llu: %d",
			    (rw == FS_DECRYPT ? "De" : "En"), index, err);
	return err;
}

// SPDX-License-Identifier: GPL-2.0
/*
 * This contains functions for filename crypto management
 *
 * Copyright (C) 2015, Google, Inc.
 * Copyright (C) 2015, Motorola Mobility
 *
 * Written by Uday Savagaonkar, 2014.
 * Modified by Jaegeuk Kim, 2015.
 *
 * This has not yet undergone a rigorous security audit.
 */

#include "crypto/fscrypt/fscrypt_private.h"
#include <linux/base64.h>

#define FSCRYPT_FNAME_MIN_MSG_LEN 16

/*
 * struct fscrypt_nokey_name - identifier for directory entry when key is absent
 *
 * When userspace lists an encrypted directory without access to the key, the
 * filesystem must present a unique "no-key name" for each filename that allows
 * it to find the directory entry again if requested.  Naively, that would just
 * mean using the ciphertext filenames.  However, since the ciphertext filenames
 * can contain illegal characters ('\0' and '/'), they must be encoded in some
 * way.  We use base64url.  But that can cause names to exceed NAME_MAX (255
 * bytes), so we also need to use a strong hash to abbreviate long names.
 *
 * The filesystem may also need another kind of hash, the "dirhash", to quickly
 * find the directory entry.  Since filesystems normally compute the dirhash
 * over the on-disk filename (i.e. the ciphertext), it's not computable from
 * no-key names that abbreviate the ciphertext using the strong hash to fit in
 * NAME_MAX.  It's also not computable if it's a keyed hash taken over the
 * plaintext (but it may still be available in the on-disk directory entry);
 * casefolded directories use this type of dirhash.  At least in these cases,
 * each no-key name must include the name's dirhash too.
 *
 * To meet all these requirements, we base64url-encode the following
 * variable-length structure.  It contains the dirhash, or 0's if the filesystem
 * didn't provide one; up to 149 bytes of the ciphertext name; and for
 * ciphertexts longer than 149 bytes, also the SHA-256 of the remaining bytes.
 *
 * This ensures that each no-key name contains everything needed to find the
 * directory entry again, contains only legal characters, doesn't exceed
 * NAME_MAX, is unambiguous unless there's a SHA-256 collision, and that we only
 * take the performance hit of SHA-256 on very long filenames (which are rare).
 */
struct fscrypt_nokey_name {
	u32 dirhash[2];
	u8 bytes[149];
	u8 sha256[SHA256_DIGEST_SIZE];
}; /* 189 bytes => 252 bytes base64url-encoded, which is <= NAME_MAX (255) */

/*
 * Decoded size of max-size no-key name, i.e. a name that was abbreviated using
 * the strong hash and thus includes the 'sha256' field.  This isn't simply
 * sizeof(struct fscrypt_nokey_name), as the padding at the end isn't included.
 */
#define FSCRYPT_NOKEY_NAME_MAX	offsetofend(struct fscrypt_nokey_name, sha256)

/* Encoded size of max-size no-key name */
#define FSCRYPT_NOKEY_NAME_MAX_ENCODED \
		BASE64_CHARS(FSCRYPT_NOKEY_NAME_MAX)

static inline bool fscrypt_is_dot_dotdot(const struct qstr *str)
{
	return name_is_dot_dotdot(str->name, str->len);
}

static int fname_decrypt(const struct inode *inode,
			 const struct fscrypt_str *iname,
			 struct fscrypt_str *oname)
{
	const struct fscrypt_inode_info *ci = fscrypt_get_inode_info_raw(inode);
	struct crypto_sync_skcipher *tfm = ci->ci_enc_key.tfm;
	struct skcipher_request *req = skcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req) return -ENOMEM;
	union fscrypt_iv iv;
	struct scatterlist src_sg, dst_sg;
	int err;

	fscrypt_generate_iv(&iv, 0, ci);

	skcipher_request_set_callback(
		req, CRYPTO_TFM_REQ_MAY_BACKLOG | CRYPTO_TFM_REQ_MAY_SLEEP,
		NULL, NULL);
	sg_init_one(&src_sg, iname->name, iname->len);
	sg_init_one(&dst_sg, oname->name, oname->len);
	skcipher_request_set_crypt(req, &src_sg, &dst_sg, iname->len, &iv);
	err = crypto_skcipher_decrypt(req);
	skcipher_request_free(req);
	if (err) {
		fscrypt_err(inode, "Filename decryption failed: %d", err);
		return err;
	}

	oname->len = strnlen((const char *)oname->name, iname->len);
	return 0;
}

/**
 * fscrypt_fname_alloc_buffer() - allocate a buffer for presented filenames
 * @max_encrypted_len: maximum length of encrypted filenames the buffer will be
 *		       used to present
 * @crypto_str: (output) buffer to allocate
 *
 * Allocate a buffer that is large enough to hold any decrypted or encoded
 * filename (null-terminated), for the given maximum encrypted filename length.
 *
 * Return: 0 on success, -errno on failure
 */
int fscrypt_fname_alloc_buffer(u32 max_encrypted_len,
			       struct fscrypt_str *crypto_str)
{
	u32 max_presented_len = max_t(u32, FSCRYPT_NOKEY_NAME_MAX_ENCODED,
				      max_encrypted_len);

	crypto_str->name = kmalloc(max_presented_len + 1, GFP_NOFS);
	if (!crypto_str->name)
		return -ENOMEM;
	crypto_str->len = max_presented_len;
	return 0;
}
EXPORT_SYMBOL(fscrypt_fname_alloc_buffer);

/**
 * fscrypt_fname_free_buffer() - free a buffer for presented filenames
 * @crypto_str: the buffer to free
 *
 * Free a buffer that was allocated by fscrypt_fname_alloc_buffer().
 */
void fscrypt_fname_free_buffer(struct fscrypt_str *crypto_str)
{
	if (!crypto_str)
		return;
	kfree(crypto_str->name);
	crypto_str->name = NULL;
}
EXPORT_SYMBOL(fscrypt_fname_free_buffer);

/**
 * fscrypt_fname_disk_to_usr() - convert an encrypted filename to
 *				 user-presentable form
 * @inode: inode of the parent directory (for regular filenames)
 *	   or of the symlink (for symlink targets)
 * @hash: first part of the name's dirhash, if applicable.  This only needs to
 *	  be provided if the filename is located in an indexed directory whose
 *	  encryption key may be unavailable.  Not needed for symlink targets.
 * @minor_hash: second part of the name's dirhash, if applicable
 * @iname: encrypted filename to convert.  May also be "." or "..", which
 *	   aren't actually encrypted.
 * @oname: output buffer for the user-presentable filename.  The caller must
 *	   have allocated enough space for this, e.g. using
 *	   fscrypt_fname_alloc_buffer().
 *
 * If the key is available, we'll decrypt the disk name.  Otherwise, we'll
 * encode it for presentation in fscrypt_nokey_name format.
 * See struct fscrypt_nokey_name for details.
 *
 * Return: 0 on success, -errno on failure
 */
int fscrypt_fname_disk_to_usr(const struct inode *inode,
			      u32 hash, u32 minor_hash,
			      const struct fscrypt_str *iname,
			      struct fscrypt_str *oname)
{
	const struct qstr qname = FSTR_TO_QSTR(iname);
	struct fscrypt_nokey_name nokey_name;
	u32 size; /* size of the unencoded no-key name */

	if (fscrypt_is_dot_dotdot(&qname)) {
		oname->name[0] = '.';
		oname->name[iname->len - 1] = '.';
		oname->len = iname->len;
		return 0;
	}

	if (iname->len < FSCRYPT_FNAME_MIN_MSG_LEN)
		return -EUCLEAN;

	if (fscrypt_has_encryption_key(inode))
		return fname_decrypt(inode, iname, oname);

	/*
	 * Sanity check that struct fscrypt_nokey_name doesn't have padding
	 * between fields and that its encoded size never exceeds NAME_MAX.
	 */
	BUILD_BUG_ON(offsetofend(struct fscrypt_nokey_name, dirhash) !=
		     offsetof(struct fscrypt_nokey_name, bytes));
	BUILD_BUG_ON(offsetofend(struct fscrypt_nokey_name, bytes) !=
		     offsetof(struct fscrypt_nokey_name, sha256));
	BUILD_BUG_ON(FSCRYPT_NOKEY_NAME_MAX_ENCODED > NAME_MAX);

	nokey_name.dirhash[0] = hash;
	nokey_name.dirhash[1] = minor_hash;

	if (iname->len <= sizeof(nokey_name.bytes)) {
		memcpy(nokey_name.bytes, iname->name, iname->len);
		size = offsetof(struct fscrypt_nokey_name, bytes[iname->len]);
	} else {
		memcpy(nokey_name.bytes, iname->name, sizeof(nokey_name.bytes));
		/* Compute strong hash of remaining part of name. */
		sha256(&iname->name[sizeof(nokey_name.bytes)],
		       iname->len - sizeof(nokey_name.bytes),
		       nokey_name.sha256);
		size = FSCRYPT_NOKEY_NAME_MAX;
	}
	oname->len = base64_encode((const u8 *)&nokey_name, size,
				   (char *)oname->name, false, BASE64_URLSAFE);
	return 0;
}
EXPORT_SYMBOL(fscrypt_fname_disk_to_usr);

/**
 * fscrypt_fname_siphash() - calculate the SipHash of a filename
 * @dir: the parent directory
 * @name: the filename to calculate the SipHash of
 *
 * Given a plaintext filename @name and a directory @dir which uses SipHash as
 * its dirhash method and has had its fscrypt key set up, this function
 * calculates the SipHash of that name using the directory's secret dirhash key.
 *
 * Return: the SipHash of @name using the hash key of @dir
 */
u64 fscrypt_fname_siphash(const struct inode *dir, const struct qstr *name)
{
	const struct fscrypt_inode_info *ci = fscrypt_get_inode_info_raw(dir);

	WARN_ON_ONCE(!ci->ci_dirhash_key_initialized);

	return siphash(name->name, name->len, &ci->ci_dirhash_key);
}
EXPORT_SYMBOL_GPL(fscrypt_fname_siphash);

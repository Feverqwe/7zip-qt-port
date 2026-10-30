// SPDX-License-Identifier: GPL-2.0
/*
 * Verification of builtin signatures
 *
 * Copyright 2019 Google LLC
 */

#include "crypto/verity/fsverity_private.h"
#include "crypto/verity/signaturecompat.h"

/**
 * fsverity_verify_signature() - check a verity file's signature
 * @vi: the file's fsverity_info
 * @signature: the file's built-in signature
 * @sig_size: size of signature in bytes, or 0 if no signature
 *
 * If the file includes a signature of its fs-verity file digest, verify it
 * against the certificates in the fs-verity keyring. Note that signatures
 * are verified regardless of the state of the 'fsverity_require_signatures'
 * variable and the LSM subsystem relies on this behavior to help enforce
 * file integrity policies. Please discuss changes with the LSM list
 * (thank you!).
 *
 * Return: 0 on success (signature valid or not required); -errno on failure
 */
int fsverity_verify_signature(const struct fsverity_info *vi,
			      const u8 *signature, size_t sig_size)
{
	const struct inode *inode = vi->inode;
	const struct fsverity_hash_alg *hash_alg = vi->tree_params.hash_alg;
	struct fsverity_formatted_digest *d;
	int err;

	if (sig_size == 0) {
		if (inode->verity_require_signatures) {
			fsverity_err(inode,
				     "require_signatures=1, rejecting unsigned file!");
			return -EPERM;
		}
		return 0;
	}

	if (sevenzip_asymmetric_trust_count(inode->verity_trust) == 0) {
		/*
		 * The ".fs-verity" keyring is empty, due to builtin signatures
		 * being supported by the kernel but not actually being used.
		 * In this case, verify_pkcs7_signature() would always return an
		 * error, usually ENOKEY.  It could also be EBADMSG if the
		 * PKCS#7 is malformed, but that isn't very important to
		 * distinguish.  So, just skip to ENOKEY to avoid the attack
		 * surface of the PKCS#7 parser, which would otherwise be
		 * reachable by any task able to execute FS_IOC_ENABLE_VERITY.
		 */
		fsverity_err(inode,
			     "fs-verity keyring is empty, rejecting signed file!");
		return -ENOKEY;
	}

	d = kzalloc(sizeof(*d) + hash_alg->digest_size, GFP_KERNEL);
	if (!d)
		return -ENOMEM;
	memcpy(d->magic, "FSVerity", 8);
	d->digest_algorithm = cpu_to_le16(hash_alg - fsverity_hash_algs);
	d->digest_size = cpu_to_le16(hash_alg->digest_size);
	memcpy(d->digest, vi->file_digest, hash_alg->digest_size);

	err = sevenzip_asymmetric_trust_verify(inode->verity_trust,
				     d, sizeof(*d) + hash_alg->digest_size,
				     signature, sig_size);
	kfree(d);

	if (err) {
		if (err == -ENOKEY)
			fsverity_err(inode,
				     "File's signing cert isn't in the fs-verity keyring");
		else if (err == -EKEYREJECTED)
			fsverity_err(inode, "Incorrect file signature");
		else if (err == -EBADMSG)
			fsverity_err(inode, "Malformed file signature");
		else
			fsverity_err(inode, "Error %d verifying file signature",
				     err);
		return err;
	}

	err = security_inode_setintegrity(inode,
					  LSM_INT_FSVERITY_BUILTINSIG_VALID,
					  signature,
					  sig_size);

	if (err) {
		fsverity_err(inode, "Error %d exposing file signature to LSMs",
			     err);
		return err;
	}

	return 0;
}

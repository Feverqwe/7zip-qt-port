// SPDX-License-Identifier: GPL-2.0
/*
 * Filesystem-level keyring for fscrypt
 *
 * Copyright 2019 Google LLC
 */

/*
 * Archive subset of the raw-key preparation from FS_IOC_ADD_ENCRYPTION_KEY.
 * Kernel user claims, revocation and ioctl entry points are omitted.
 */

#include "fscrypt_private.h"

/* Raw immutable archive key preparation.  Kernel keyring claims/revocation
 * do not apply; the caller owns and wipes this record after inode setup. */
int fscrypt_prepare_master_key(struct fscrypt_master_key *mk, u32 type,
			       const u8 *descriptor, const u8 *raw, unsigned size)
{
	if (!mk || !raw || size < FSCRYPT_MIN_KEY_SIZE || size > FSCRYPT_MAX_RAW_KEY_SIZE)
		return -EINVAL;
	if (type != FSCRYPT_KEY_SPEC_TYPE_DESCRIPTOR && type != FSCRYPT_KEY_SPEC_TYPE_IDENTIFIER)
		return -EINVAL;
	if (type == FSCRYPT_KEY_SPEC_TYPE_DESCRIPTOR && !descriptor) return -EINVAL;
	memset(mk, 0, sizeof(*mk));
	mk->mk_spec.type = type;
	mk->mk_secret.size = size;
	memcpy(mk->mk_secret.bytes, raw, size);
	if (type == FSCRYPT_KEY_SPEC_TYPE_DESCRIPTOR) {
		memcpy(mk->mk_spec.u.descriptor, descriptor, FSCRYPT_KEY_DESCRIPTOR_SIZE);
	} else {
		/* Original raw-key branch of add_master_key(): HKDF and identifier. */
		fscrypt_init_hkdf(&mk->mk_secret.hkdf, mk->mk_secret.bytes, size);
		memzero_explicit(mk->mk_secret.bytes, size);
		fscrypt_hkdf_expand(&mk->mk_secret.hkdf,
			HKDF_CONTEXT_KEY_IDENTIFIER_FOR_RAW_KEY, NULL, 0,
			mk->mk_spec.u.identifier, FSCRYPT_KEY_IDENTIFIER_SIZE);
		/* Prepare before publication, avoiding a shared mutable lazy cache. */
		fscrypt_derive_siphash_key(mk, HKDF_CONTEXT_INODE_HASH_KEY, NULL, 0,
					   &mk->mk_ino_hash_key);
		mk->mk_ino_hash_key_initialized = true;
	}
	mk->mk_present = true;
	return 0;
}

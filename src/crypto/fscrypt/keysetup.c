// SPDX-License-Identifier: GPL-2.0
/*
 * Key setup facility for FS encryption support.
 *
 * Copyright (C) 2015, Google, Inc.
 *
 * Originally written by Michael Halcrow, Ildar Muslukhov, and Uday Savagaonkar.
 * Heavily modified since then.
 */

#include "crypto/fscrypt/fscrypt_private.h"

struct fscrypt_mode fscrypt_modes[] = {
	{0},
	{
		.friendly_name = "AES-256-XTS",
		.cipher_str = "xts(aes)",
		.keysize = 64,
		.security_strength = 32,
		.ivsize = 16,
		.blk_crypto_mode = BLK_ENCRYPTION_MODE_AES_256_XTS,
	},
	{0},
	{0},
	{
		.friendly_name = "AES-256-CBC-CTS",
		.cipher_str = "cts(cbc(aes))",
		.keysize = 32,
		.security_strength = 32,
		.ivsize = 16,
	},
	{
		.friendly_name = "AES-128-CBC-ESSIV",
		.cipher_str = "essiv(cbc(aes),sha256)",
		.keysize = 16,
		.security_strength = 16,
		.ivsize = 16,
		.blk_crypto_mode = BLK_ENCRYPTION_MODE_AES_128_CBC_ESSIV,
	},
	{
		.friendly_name = "AES-128-CBC-CTS",
		.cipher_str = "cts(cbc(aes))",
		.keysize = 16,
		.security_strength = 16,
		.ivsize = 16,
	},
	{
		.friendly_name = "SM4-XTS",
		.cipher_str = "xts(sm4)",
		.keysize = 32,
		.security_strength = 16,
		.ivsize = 16,
		.blk_crypto_mode = BLK_ENCRYPTION_MODE_SM4_XTS,
	},
	{
		.friendly_name = "SM4-CBC-CTS",
		.cipher_str = "cts(cbc(sm4))",
		.keysize = 16,
		.security_strength = 16,
		.ivsize = 16,
	},
	{
		.friendly_name = "Adiantum",
		.cipher_str = "adiantum(xchacha12,aes)",
		.keysize = 32,
		.security_strength = 32,
		.ivsize = 32,
		.blk_crypto_mode = BLK_ENCRYPTION_MODE_ADIANTUM,
	},
	{
		.friendly_name = "AES-256-HCTR2",
		.cipher_str = "hctr2(aes)",
		.keysize = 32,
		.security_strength = 32,
		.ivsize = 32,
	},
};

static struct fscrypt_mode *
select_encryption_mode(const union fscrypt_policy *policy,
		       const struct inode *inode)
{
	if (S_ISREG(inode->i_mode))
		return &fscrypt_modes[fscrypt_policy_contents_mode(policy)];

	if (S_ISDIR(inode->i_mode) || S_ISLNK(inode->i_mode))
		return &fscrypt_modes[fscrypt_policy_fnames_mode(policy)];

	WARN_ONCE(1, "fscrypt: filesystem tried to load encryption info for inode %llu, which is not encryptable (file type %d)\n",
		  inode->i_ino, (inode->i_mode & S_IFMT));
	return ERR_PTR(-EINVAL);
}

/* Create a symmetric cipher object for the given encryption mode and key */
static struct crypto_sync_skcipher *
fscrypt_allocate_skcipher(struct fscrypt_mode *mode, const u8 *raw_key,
			  const struct inode *inode)
{
	struct crypto_sync_skcipher *tfm;
	int err;

	tfm = crypto_alloc_sync_skcipher(mode->cipher_str, 0,
					 FSCRYPT_CRYPTOAPI_MASK);
	if (IS_ERR(tfm)) {
		if (PTR_ERR(tfm) == -ENOENT) {
			fscrypt_warn(inode,
				     "Missing crypto API support for %s (API name: \"%s\")",
				     mode->friendly_name, mode->cipher_str);
			return ERR_PTR(-ENOPKG);
		}
		fscrypt_err(inode, "Error allocating '%s' transform: %ld",
			    mode->cipher_str, PTR_ERR(tfm));
		return tfm;
	}
	if (WARN_ON_ONCE(crypto_sync_skcipher_ivsize(tfm) != mode->ivsize)) {
		err = -EINVAL;
		goto err_free_tfm;
	}
	crypto_sync_skcipher_set_flags(tfm, CRYPTO_TFM_REQ_FORBID_WEAK_KEYS);
	err = crypto_sync_skcipher_setkey(tfm, raw_key, mode->keysize);
	if (err)
		goto err_free_tfm;

	return tfm;

err_free_tfm:
	crypto_free_sync_skcipher(tfm);
	return ERR_PTR(err);
}

/*
 * Prepare the crypto transform object or blk-crypto key in @prep_key, given the
 * raw key, encryption mode (@ci->ci_mode), predicate indicating which style of
 * key is needed (fscrypt_using_inline_encryption(ci)), IV generation method
 * (@ci->ci_policy.flags), and data unit size (@ci->ci_data_unit_bits).
 */
int fscrypt_prepare_key(struct fscrypt_prepared_key *prep_key,
			const u8 *raw_key, const struct fscrypt_inode_info *ci)
{
	struct crypto_sync_skcipher *tfm;

	tfm = fscrypt_allocate_skcipher(ci->ci_mode, raw_key, ci->ci_inode);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);
	prep_key->tfm = tfm;
	return 0;
}

/* Destroy a crypto transform object and/or blk-crypto key. */
void fscrypt_destroy_prepared_key(struct super_block *sb,
				  struct fscrypt_prepared_key *prep_key)
{
	crypto_free_sync_skcipher(prep_key->tfm);
	(void)sb; /* No kernel inline-crypto key is owned by an archive inode. */
	memzero_explicit(prep_key, sizeof(*prep_key));
}

/* Given a per-file encryption key, set up the file's crypto transform object */
int fscrypt_set_per_file_enc_key(struct fscrypt_inode_info *ci,
				 const u8 *raw_key)
{
	ci->ci_owns_key = true;
	return fscrypt_prepare_key(&ci->ci_enc_key, raw_key, ci);
}

static int setup_per_mode_enc_key(struct fscrypt_inode_info *ci,
				  struct fscrypt_master_key *mk,
				  u8 hkdf_context, bool include_fs_uuid)
{
	const struct inode *inode = ci->ci_inode;
	const struct super_block *sb = inode->i_sb;
	struct fscrypt_mode *mode = ci->ci_mode;
	const u8 mode_num = mode - fscrypt_modes;
	struct fscrypt_prepared_key *prep_key = &ci->ci_enc_key;
	u8 raw_mode_key[FSCRYPT_MAX_RAW_KEY_SIZE];
	u8 hkdf_info[sizeof(mode_num) + sizeof(sb->s_uuid)];
	unsigned int hkdf_infolen = 0;
	int err;

	/* Archive inodes own their transform; no global kernel transform cache. */
	ci->ci_owns_key = true;

	if (mk->mk_secret.is_hw_wrapped && S_ISREG(inode->i_mode)) {
		err = -EOPNOTSUPP; /* A wrapped hardware key cannot be unwrapped here. */
	} else {
		static_assert(sizeof(mode_num) == 1);
		static_assert(sizeof(sb->s_uuid) == 16);
		static_assert(sizeof(hkdf_info) == 17);
		hkdf_info[hkdf_infolen++] = mode_num;
		if (include_fs_uuid) {
			memcpy(&hkdf_info[hkdf_infolen], &sb->s_uuid,
			       sizeof(sb->s_uuid));
			hkdf_infolen += sizeof(sb->s_uuid);
		}
		fscrypt_hkdf_expand(&mk->mk_secret.hkdf, hkdf_context,
				    hkdf_info, hkdf_infolen, raw_mode_key,
				    mode->keysize);
		err = fscrypt_prepare_key(prep_key, raw_mode_key, ci);
		memzero_explicit(raw_mode_key, sizeof(raw_mode_key));
	}
	return err;
}

/*
 * Derive a SipHash key from the given fscrypt master key and the given
 * application-specific information string.
 *
 * Note that the KDF produces a byte array, but the SipHash APIs expect the key
 * as a pair of 64-bit words.  Therefore, on big endian CPUs we have to do an
 * endianness swap in order to get the same results as on little endian CPUs.
 */
void fscrypt_derive_siphash_key(const struct fscrypt_master_key *mk,
				       u8 context, const u8 *info,
				       unsigned int infolen, siphash_key_t *key)
{
	fscrypt_hkdf_expand(&mk->mk_secret.hkdf, context, info, infolen,
			    (u8 *)key, sizeof(*key));
	BUILD_BUG_ON(sizeof(*key) != 16);
	BUILD_BUG_ON(ARRAY_SIZE(key->key) != 2);
	le64_to_cpus(&key->key[0]);
	le64_to_cpus(&key->key[1]);
}

void fscrypt_derive_dirhash_key(struct fscrypt_inode_info *ci,
				const struct fscrypt_master_key *mk)
{
	fscrypt_derive_siphash_key(mk, HKDF_CONTEXT_DIRHASH_KEY,
				   ci->ci_nonce, FSCRYPT_FILE_NONCE_SIZE,
				   &ci->ci_dirhash_key);
	ci->ci_dirhash_key_initialized = true;
}

void fscrypt_hash_inode_number(struct fscrypt_inode_info *ci,
			       const struct fscrypt_master_key *mk)
{
	WARN_ON_ONCE(ci->ci_inode->i_ino == 0);
	WARN_ON_ONCE(!mk->mk_ino_hash_key_initialized);

	ci->ci_hashed_ino = (u32)siphash_1u64(ci->ci_inode->i_ino,
					      &mk->mk_ino_hash_key);
}

static int fscrypt_setup_iv_ino_lblk_32_key(struct fscrypt_inode_info *ci,
					    struct fscrypt_master_key *mk)
{
	int err;

	err = setup_per_mode_enc_key(ci, mk, HKDF_CONTEXT_IV_INO_LBLK_32_KEY,
				     true);
	if (err)
		return err;

	/* The immutable archive key owner prepares the inode hash key eagerly. */

	/*
	 * New inodes may not have an inode number assigned yet.
	 * Hashing their inode number is delayed until later.
	 */
	if (ci->ci_inode->i_ino)
		fscrypt_hash_inode_number(ci, mk);
	return 0;
}

static int fscrypt_setup_v2_file_key(struct fscrypt_inode_info *ci,
				     struct fscrypt_master_key *mk,
				     bool need_dirhash_key)
{
	int err;

	if (mk->mk_secret.is_hw_wrapped &&
	    !(ci->ci_policy.v2.flags & (FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64 |
					FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32))) {
		fscrypt_warn(ci->ci_inode,
			     "Hardware-wrapped keys are only supported with IV_INO_LBLK policies");
		return -EINVAL;
	}

	if (ci->ci_policy.v2.flags & FSCRYPT_POLICY_FLAG_DIRECT_KEY) {
		/*
		 * DIRECT_KEY: instead of deriving per-file encryption keys, the
		 * per-file nonce will be included in all the IVs.  But unlike
		 * v1 policies, for v2 policies in this case we don't encrypt
		 * with the master key directly but rather derive a per-mode
		 * encryption key.  This ensures that the master key is
		 * consistently used only for HKDF, avoiding key reuse issues.
		 */
		err = setup_per_mode_enc_key(ci, mk, HKDF_CONTEXT_DIRECT_KEY,
					     false);
	} else if (ci->ci_policy.v2.flags &
		   FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64) {
		/*
		 * IV_INO_LBLK_64: encryption keys are derived from (master_key,
		 * mode_num, filesystem_uuid), and inode number is included in
		 * the IVs.  This format is optimized for use with inline
		 * encryption hardware compliant with the UFS standard.
		 */
		err = setup_per_mode_enc_key(
			ci, mk, HKDF_CONTEXT_IV_INO_LBLK_64_KEY, true);
	} else if (ci->ci_policy.v2.flags &
		   FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32) {
		err = fscrypt_setup_iv_ino_lblk_32_key(ci, mk);
	} else {
		u8 derived_key[FSCRYPT_MAX_RAW_KEY_SIZE];

		fscrypt_hkdf_expand(&mk->mk_secret.hkdf,
				    HKDF_CONTEXT_PER_FILE_ENC_KEY,
				    ci->ci_nonce, FSCRYPT_FILE_NONCE_SIZE,
				    derived_key, ci->ci_mode->keysize);
		err = fscrypt_set_per_file_enc_key(ci, derived_key);
		memzero_explicit(derived_key, sizeof(derived_key));
	}
	if (err)
		return err;

	/* Derive a secret dirhash key for directories that need it. */
	if (need_dirhash_key)
		fscrypt_derive_dirhash_key(ci, mk);

	return 0;
}

/*
 * Check whether the size of the given master key (@mk) is appropriate for the
 * encryption settings which a particular file will use (@ci).
 *
 * If the file uses a v1 encryption policy, then the master key must be at least
 * as long as the derived key, as this is a requirement of the v1 KDF.
 *
 * Otherwise, the KDF can accept any size key, so we enforce a slightly looser
 * requirement: we require that the size of the master key be at least the
 * maximum security strength of any algorithm whose key will be derived from it
 * (but in practice we only need to consider @ci->ci_mode, since any other
 * possible subkeys such as DIRHASH and INODE_HASH will never increase the
 * required key size over @ci->ci_mode).  This allows AES-256-XTS keys to be
 * derived from a 256-bit master key, which is cryptographically sufficient,
 * rather than requiring a 512-bit master key which is unnecessarily long.  (We
 * still allow 512-bit master keys if the user chooses to use them, though.)
 */
static bool fscrypt_valid_master_key_size(const struct fscrypt_master_key *mk,
					  const struct fscrypt_inode_info *ci)
{
	unsigned int min_keysize;

	if (ci->ci_policy.version == FSCRYPT_POLICY_V1)
		min_keysize = ci->ci_mode->keysize;
	else
		min_keysize = ci->ci_mode->security_strength;

	if (mk->mk_secret.size < min_keysize) {
		fscrypt_warn(NULL,
			     "key with %s %*phN is too short (got %u bytes, need %u+ bytes)",
			     master_key_spec_type(&mk->mk_spec),
			     master_key_spec_len(&mk->mk_spec),
			     (u8 *)&mk->mk_spec.u,
			     mk->mk_secret.size, min_keysize);
		return false;
	}
	return true;
}

static int setup_file_encryption_key(struct fscrypt_inode_info *ci,
				     bool need_dirhash_key,
				     struct fscrypt_master_key **mk_ret)
{
	struct super_block *sb = ci->ci_inode->i_sb;
	struct fscrypt_key_specifier mk_spec;
	struct fscrypt_master_key *mk;
	int err;

	err = fscrypt_policy_to_key_spec(&ci->ci_policy, &mk_spec);
	if (err)
		return err;

	mk = fscrypt_find_master_key(sb, &mk_spec);
	if (!mk)
		return -ENOKEY; /* Host keys are explicit, not process keyring entries. */

	if (!mk->mk_present) {
		/* FS_IOC_REMOVE_ENCRYPTION_KEY has been executed on this key */
		err = -ENOKEY;
		goto out_release_key;
	}

	if (!fscrypt_valid_master_key_size(mk, ci)) {
		err = -ENOKEY;
		goto out_release_key;
	}

	switch (ci->ci_policy.version) {
	case FSCRYPT_POLICY_V1:
		if (WARN_ON_ONCE(mk->mk_secret.is_hw_wrapped)) {
			/*
			 * This should never happen, as adding a v1 policy key
			 * that is hardware-wrapped isn't allowed.
			 */
			err = -EINVAL;
			goto out_release_key;
		}
		err = fscrypt_setup_v1_file_key(ci, mk->mk_secret.bytes);
		break;
	case FSCRYPT_POLICY_V2:
		err = fscrypt_setup_v2_file_key(ci, mk, need_dirhash_key);
		break;
	default:
		WARN_ON_ONCE(1);
		err = -EINVAL;
		break;
	}
	if (err)
		goto out_release_key;

	*mk_ret = mk;
	return 0;

out_release_key:
	return err;
}

/* Archive CI lifetime owns the prepared transform, without VFS key links. */
static void put_crypt_info(struct fscrypt_inode_info *ci)
{
	if (!ci) return;
	if (ci->ci_owns_key)
		fscrypt_destroy_prepared_key(ci->ci_inode->i_sb, &ci->ci_enc_key);
	memzero_explicit(ci, sizeof(*ci));
	free(ci);
}

static int
fscrypt_setup_encryption_info(struct inode *inode,
			      const union fscrypt_policy *policy,
			      const u8 nonce[FSCRYPT_FILE_NONCE_SIZE],
			      bool need_dirhash_key)
{
	struct fscrypt_inode_info *crypt_info;
	struct fscrypt_mode *mode;
	struct fscrypt_master_key *mk = NULL;
	int res;

	crypt_info = calloc(1, sizeof(*crypt_info));
	if (!crypt_info)
		return -ENOMEM;

	crypt_info->ci_inode = inode;
	crypt_info->ci_policy = *policy;
	memcpy(crypt_info->ci_nonce, nonce, FSCRYPT_FILE_NONCE_SIZE);

	mode = select_encryption_mode(&crypt_info->ci_policy, inode);
	if (IS_ERR(mode)) {
		res = PTR_ERR(mode);
		goto out;
	}
	WARN_ON_ONCE(mode->ivsize > FSCRYPT_MAX_IV_SIZE);
	crypt_info->ci_mode = mode;

	crypt_info->ci_data_unit_bits =
		fscrypt_policy_du_bits(&crypt_info->ci_policy, inode);

	res = setup_file_encryption_key(crypt_info, need_dirhash_key, &mk);
	if (res)
		goto out;

	/* The private archive inode is initialized before its owner publishes it. */
	if (!inode->i_crypt_info) {
		inode->i_crypt_info = crypt_info;
		crypt_info = NULL;
	}
	res = 0;
out:
	put_crypt_info(crypt_info);
	return res;
}

/**
 * fscrypt_get_encryption_info() - set up an inode's encryption key
 * @inode: the inode to set up the key for.  Must be encrypted.
 * @allow_unsupported: if %true, treat an unsupported encryption policy (or
 *		       unrecognized encryption context) the same way as the key
 *		       being unavailable, instead of returning an error.  Use
 *		       %false unless the operation being performed is needed in
 *		       order for files (or directories) to be deleted.
 *
 * Set up the inode's encryption key, if it hasn't already been done.
 *
 * Note: unless the key setup was already done, this isn't %GFP_NOFS-safe.  So
 * generally this shouldn't be called from within a filesystem transaction.
 *
 * Return: 0 if the key is now set up, *or* if it couldn't be set up because the
 *	   needed master key is absent.  (Use fscrypt_has_encryption_key() to
 *	   distinguish these cases.)  Also can return another -errno code.
 */
int fscrypt_get_encryption_info(struct inode *inode, bool allow_unsupported)
{
	int res;
	union fscrypt_context ctx;
	union fscrypt_policy policy;

	if (fscrypt_has_encryption_key(inode))
		return 0;

	res = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx));
	if (res < 0) {
		if (res == -ERANGE && allow_unsupported)
			return 0;
		fscrypt_warn(inode, "Error %d getting encryption context", res);
		return res;
	}

	res = fscrypt_policy_from_context(&policy, &ctx, res);
	if (res) {
		if (allow_unsupported)
			return 0;
		fscrypt_warn(inode,
			     "Unrecognized or corrupt encryption context");
		return res;
	}

	if (!fscrypt_supported_policy(&policy, inode)) {
		if (allow_unsupported)
			return 0;
		return -EINVAL;
	}

	res = fscrypt_setup_encryption_info(inode, &policy,
					    fscrypt_context_nonce(&ctx),
					    IS_CASEFOLDED(inode) &&
					    S_ISDIR(inode->i_mode));

	if (res == -ENOPKG && allow_unsupported) /* Algorithm unavailable? */
		res = 0;
	if (res == -ENOKEY)
		res = 0;
	return res;
}

/**
 * fscrypt_put_encryption_info() - free most of an inode's fscrypt data
 * @inode: an inode being evicted
 *
 * Free the inode's fscrypt_inode_info.  Filesystems must call this when the
 * inode is being evicted.  An RCU grace period need not have elapsed yet.
 */
void fscrypt_put_encryption_info(struct inode *inode)
{
	/*
	 * Ideally we'd start with a lightweight IS_ENCRYPTED() check here
	 * before proceeding to retrieve and check the pointer.  However, during
	 * inode creation, the fscrypt_inode_info is set before S_ENCRYPTED.  If
	 * an error occurs, it needs to be cleaned up regardless.
	 */
	struct fscrypt_inode_info **ci_addr = fscrypt_inode_info_addr(inode);

	put_crypt_info(*ci_addr);
	*ci_addr = NULL;
}

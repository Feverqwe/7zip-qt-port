// SPDX-License-Identifier: GPL-2.0
/*
 * Encryption policy functions for per-file encryption support.
 *
 * Copyright (C) 2015, Google, Inc.
 * Copyright (C) 2015, Motorola Mobility.
 *
 * Originally written by Michael Halcrow, 2015.
 * Modified by Jaegeuk Kim, 2015.
 * Modified by Eric Biggers, 2019 for v2 policy support.
 */

#include "crypto/fscrypt/fscrypt_private.h"

bool fscrypt_policies_equal(const union fscrypt_policy *policy1,
			    const union fscrypt_policy *policy2)
{
	if (policy1->version != policy2->version)
		return false;

	return !memcmp(policy1, policy2, fscrypt_policy_size(policy1));
}

int fscrypt_policy_to_key_spec(const union fscrypt_policy *policy,
			       struct fscrypt_key_specifier *key_spec)
{
	switch (policy->version) {
	case FSCRYPT_POLICY_V1:
		key_spec->type = FSCRYPT_KEY_SPEC_TYPE_DESCRIPTOR;
		memcpy(key_spec->u.descriptor, policy->v1.master_key_descriptor,
		       FSCRYPT_KEY_DESCRIPTOR_SIZE);
		return 0;
	case FSCRYPT_POLICY_V2:
		key_spec->type = FSCRYPT_KEY_SPEC_TYPE_IDENTIFIER;
		memcpy(key_spec->u.identifier, policy->v2.master_key_identifier,
		       FSCRYPT_KEY_IDENTIFIER_SIZE);
		return 0;
	default:
		WARN_ON_ONCE(1);
		return -EINVAL;
	}
}

/*
 * Return %true if the given combination of encryption modes is supported for v1
 * (and later) encryption policies.
 *
 * Do *not* add anything new here, since v1 encryption policies are deprecated.
 * New combinations of modes should go in fscrypt_valid_enc_modes_v2() only.
 */
static bool fscrypt_valid_enc_modes_v1(u32 contents_mode, u32 filenames_mode)
{
	if (contents_mode == FSCRYPT_MODE_AES_256_XTS &&
	    filenames_mode == FSCRYPT_MODE_AES_256_CTS)
		return true;

	if (contents_mode == FSCRYPT_MODE_AES_128_CBC &&
	    filenames_mode == FSCRYPT_MODE_AES_128_CTS)
		return true;

	if (contents_mode == FSCRYPT_MODE_ADIANTUM &&
	    filenames_mode == FSCRYPT_MODE_ADIANTUM)
		return true;

	return false;
}

static bool fscrypt_valid_enc_modes_v2(u32 contents_mode, u32 filenames_mode)
{
	if (contents_mode == FSCRYPT_MODE_AES_256_XTS &&
	    filenames_mode == FSCRYPT_MODE_AES_256_HCTR2)
		return true;

	if (contents_mode == FSCRYPT_MODE_SM4_XTS &&
	    filenames_mode == FSCRYPT_MODE_SM4_CTS)
		return true;

	return fscrypt_valid_enc_modes_v1(contents_mode, filenames_mode);
}

static bool supported_direct_key_modes(const struct inode *inode,
				       u32 contents_mode, u32 filenames_mode)
{
	const struct fscrypt_mode *mode;

	if (contents_mode != filenames_mode) {
		fscrypt_warn(inode,
			     "Direct key flag not allowed with different contents and filenames modes");
		return false;
	}
	mode = &fscrypt_modes[contents_mode];

	if (mode->ivsize < offsetofend(union fscrypt_iv, nonce)) {
		fscrypt_warn(inode, "Direct key flag not allowed with %s",
			     mode->friendly_name);
		return false;
	}
	return true;
}

static bool supported_iv_ino_lblk_policy(const struct fscrypt_policy_v2 *policy,
					 const struct inode *inode)
{
	const char *type = (policy->flags & FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64)
				? "IV_INO_LBLK_64" : "IV_INO_LBLK_32";
	struct super_block *sb = inode->i_sb;

	/*
	 * IV_INO_LBLK_* exist only because of hardware limitations, and
	 * currently the only known use case for them involves AES-256-XTS.
	 * That's also all we test currently.  For these reasons, for now only
	 * allow AES-256-XTS here.  This can be relaxed later if a use case for
	 * IV_INO_LBLK_* with other encryption modes arises.
	 */
	if (policy->contents_encryption_mode != FSCRYPT_MODE_AES_256_XTS) {
		fscrypt_warn(inode,
			     "Can't use %s policy with contents mode other than AES-256-XTS",
			     type);
		return false;
	}

	/*
	 * It's unsafe to include inode numbers in the IVs if the filesystem can
	 * potentially renumber inodes, e.g. via filesystem shrinking.
	 */
	if (!sb->s_cop->has_stable_inodes ||
	    !sb->s_cop->has_stable_inodes(sb)) {
		fscrypt_warn(inode,
			     "Can't use %s policy on filesystem '%s' because it doesn't have stable inode numbers",
			     type, sb->s_id);
		return false;
	}

	/*
	 * IV_INO_LBLK_64 and IV_INO_LBLK_32 both require that inode numbers fit
	 * in 32 bits.  In principle, IV_INO_LBLK_32 could support longer inode
	 * numbers because it hashes the inode number; however, currently the
	 * inode number is gotten from inode::i_ino which is 'unsigned long'.
	 * So for now the implementation limit is 32 bits.
	 */
	if (!sb->s_cop->has_32bit_inodes) {
		fscrypt_warn(inode,
			     "Can't use %s policy on filesystem '%s' because its inode numbers are too long",
			     type, sb->s_id);
		return false;
	}

	/*
	 * IV_INO_LBLK_64 and IV_INO_LBLK_32 both require that file data unit
	 * indices fit in 32 bits.
	 */
	if (fscrypt_max_file_dun_bits(sb,
			fscrypt_policy_v2_du_bits(policy, inode)) > 32) {
		fscrypt_warn(inode,
			     "Can't use %s policy on filesystem '%s' because its maximum file size is too large",
			     type, sb->s_id);
		return false;
	}

	/*
	 * IV_INO_LBLK_32 isn't compatible with inline encryption when
	 * s_blocksize != PAGE_SIZE.  In that case the DUN can wrap around in
	 * the middle of a page, but sometimes fscrypt_mergeable_bio() is called
	 * only for the first block per page.  Since IV_INO_LBLK_32 exists only
	 * to support inline encryption hardware that is limited to 32-bit DUNs,
	 * just disallow IV_INO_LBLK_32 with s_blocksize != PAGE_SIZE entirely.
	 */
	if ((policy->flags & FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32) &&
	    sb->s_blocksize != sb->s_page_size) {
		fscrypt_warn(inode,
			     "Can't use %s policy on filesystem '%s' with block size != PAGE_SIZE",
			     type, sb->s_id);
		return false;
	}

	return true;
}

static bool fscrypt_supported_v1_policy(const struct fscrypt_policy_v1 *policy,
					const struct inode *inode)
{
	if (!fscrypt_valid_enc_modes_v1(policy->contents_encryption_mode,
				     policy->filenames_encryption_mode)) {
		fscrypt_warn(inode,
			     "Unsupported encryption modes (contents %d, filenames %d)",
			     policy->contents_encryption_mode,
			     policy->filenames_encryption_mode);
		return false;
	}

	if (policy->flags & ~(FSCRYPT_POLICY_FLAGS_PAD_MASK |
			      FSCRYPT_POLICY_FLAG_DIRECT_KEY)) {
		fscrypt_warn(inode, "Unsupported encryption flags (0x%02x)",
			     policy->flags);
		return false;
	}

	if ((policy->flags & FSCRYPT_POLICY_FLAG_DIRECT_KEY) &&
	    !supported_direct_key_modes(inode, policy->contents_encryption_mode,
					policy->filenames_encryption_mode))
		return false;

	if (IS_CASEFOLDED(inode)) {
		/* With v1, there's no way to derive dirhash keys. */
		fscrypt_warn(inode,
			     "v1 policies can't be used on casefolded directories");
		return false;
	}

	return true;
}

static bool fscrypt_supported_v2_policy(const struct fscrypt_policy_v2 *policy,
					const struct inode *inode)
{
	int count = 0;

	if (!fscrypt_valid_enc_modes_v2(policy->contents_encryption_mode,
				     policy->filenames_encryption_mode)) {
		fscrypt_warn(inode,
			     "Unsupported encryption modes (contents %d, filenames %d)",
			     policy->contents_encryption_mode,
			     policy->filenames_encryption_mode);
		return false;
	}

	if (policy->flags & ~(FSCRYPT_POLICY_FLAGS_PAD_MASK |
			      FSCRYPT_POLICY_FLAG_DIRECT_KEY |
			      FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64 |
			      FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32)) {
		fscrypt_warn(inode, "Unsupported encryption flags (0x%02x)",
			     policy->flags);
		return false;
	}

	count += !!(policy->flags & FSCRYPT_POLICY_FLAG_DIRECT_KEY);
	count += !!(policy->flags & FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64);
	count += !!(policy->flags & FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32);
	if (count > 1) {
		fscrypt_warn(inode, "Mutually exclusive encryption flags (0x%02x)",
			     policy->flags);
		return false;
	}

	if (policy->log2_data_unit_size) {
		if (!inode->i_sb->s_cop->supports_subblock_data_units) {
			fscrypt_warn(inode,
				     "Filesystem does not support configuring crypto data unit size");
			return false;
		}
		if (policy->log2_data_unit_size > inode->i_blkbits ||
		    policy->log2_data_unit_size < SECTOR_SHIFT /* 9 */) {
			fscrypt_warn(inode,
				     "Unsupported log2_data_unit_size in encryption policy: %d",
				     policy->log2_data_unit_size);
			return false;
		}
		if (policy->log2_data_unit_size != inode->i_blkbits &&
		    (policy->flags & FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32)) {
			/*
			 * Not safe to enable yet, as we need to ensure that DUN
			 * wraparound can only occur on a FS block boundary.
			 */
			fscrypt_warn(inode,
				     "Sub-block data units not yet supported with IV_INO_LBLK_32");
			return false;
		}
	}

	if ((policy->flags & FSCRYPT_POLICY_FLAG_DIRECT_KEY) &&
	    !supported_direct_key_modes(inode, policy->contents_encryption_mode,
					policy->filenames_encryption_mode))
		return false;

	if ((policy->flags & (FSCRYPT_POLICY_FLAG_IV_INO_LBLK_64 |
			      FSCRYPT_POLICY_FLAG_IV_INO_LBLK_32)) &&
	    !supported_iv_ino_lblk_policy(policy, inode))
		return false;

	if (memchr_inv(policy->__reserved, 0, sizeof(policy->__reserved))) {
		fscrypt_warn(inode, "Reserved bits set in encryption policy");
		return false;
	}

	return true;
}

/**
 * fscrypt_supported_policy() - check whether an encryption policy is supported
 * @policy_u: the encryption policy
 * @inode: the inode on which the policy will be used
 *
 * Given an encryption policy, check whether all its encryption modes and other
 * settings are supported by this kernel on the given inode.  (But we don't
 * currently don't check for crypto API support here, so attempting to use an
 * algorithm not configured into the crypto API will still fail later.)
 *
 * Return: %true if supported, else %false
 */
bool fscrypt_supported_policy(const union fscrypt_policy *policy_u,
			      const struct inode *inode)
{
	switch (policy_u->version) {
	case FSCRYPT_POLICY_V1:
		return fscrypt_supported_v1_policy(&policy_u->v1, inode);
	case FSCRYPT_POLICY_V2:
		return fscrypt_supported_v2_policy(&policy_u->v2, inode);
	}
	return false;
}

/**
 * fscrypt_policy_from_context() - convert an fscrypt_context to
 *				   an fscrypt_policy
 * @policy_u: output policy
 * @ctx_u: input context
 * @ctx_size: size of input context in bytes
 *
 * Given an fscrypt_context, build the corresponding fscrypt_policy.
 *
 * Return: 0 on success, or -EINVAL if the fscrypt_context has an unrecognized
 * version number or size.
 *
 * This does *not* validate the settings within the policy itself, e.g. the
 * modes, flags, and reserved bits.  Use fscrypt_supported_policy() for that.
 */
int fscrypt_policy_from_context(union fscrypt_policy *policy_u,
				const union fscrypt_context *ctx_u,
				int ctx_size)
{
	memset(policy_u, 0, sizeof(*policy_u));

	if (!fscrypt_context_is_valid(ctx_u, ctx_size))
		return -EINVAL;

	switch (ctx_u->version) {
	case FSCRYPT_CONTEXT_V1: {
		const struct fscrypt_context_v1 *ctx = &ctx_u->v1;
		struct fscrypt_policy_v1 *policy = &policy_u->v1;

		policy->version = FSCRYPT_POLICY_V1;
		policy->contents_encryption_mode =
			ctx->contents_encryption_mode;
		policy->filenames_encryption_mode =
			ctx->filenames_encryption_mode;
		policy->flags = ctx->flags;
		memcpy(policy->master_key_descriptor,
		       ctx->master_key_descriptor,
		       sizeof(policy->master_key_descriptor));
		return 0;
	}
	case FSCRYPT_CONTEXT_V2: {
		const struct fscrypt_context_v2 *ctx = &ctx_u->v2;
		struct fscrypt_policy_v2 *policy = &policy_u->v2;

		policy->version = FSCRYPT_POLICY_V2;
		policy->contents_encryption_mode =
			ctx->contents_encryption_mode;
		policy->filenames_encryption_mode =
			ctx->filenames_encryption_mode;
		policy->flags = ctx->flags;
		policy->log2_data_unit_size = ctx->log2_data_unit_size;
		memcpy(policy->__reserved, ctx->__reserved,
		       sizeof(policy->__reserved));
		memcpy(policy->master_key_identifier,
		       ctx->master_key_identifier,
		       sizeof(policy->master_key_identifier));
		return 0;
	}
	}
	/* unreachable */
	return -EINVAL;
}

/* Retrieve an inode's encryption policy */
static int fscrypt_get_policy(struct inode *inode, union fscrypt_policy *policy)
{
	const struct fscrypt_inode_info *ci;
	union fscrypt_context ctx;
	int ret;

	ci = fscrypt_get_inode_info(inode);
	if (ci) {
		/* key available, use the cached policy */
		*policy = ci->ci_policy;
		return 0;
	}

	if (!IS_ENCRYPTED(inode))
		return -ENODATA;

	ret = inode->i_sb->s_cop->get_context(inode, &ctx, sizeof(ctx));
	if (ret < 0)
		return (ret == -ERANGE) ? -EINVAL : ret;

	return fscrypt_policy_from_context(policy, &ctx, ret);
}


/**
 * fscrypt_has_permitted_context() - is a file's encryption policy permitted
 *				     within its directory?
 *
 * @parent: inode for parent directory
 * @child: inode for file being looked up, opened, or linked into @parent
 *
 * Filesystems must call this before permitting access to an inode in a
 * situation where the parent directory is encrypted (either before allowing
 * ->lookup() to succeed, or for a regular file before allowing it to be opened)
 * and before any operation that involves linking an inode into an encrypted
 * directory, including link, rename, and cross rename.  It enforces the
 * constraint that within a given encrypted directory tree, all files use the
 * same encryption policy.  The pre-access check is needed to detect potentially
 * malicious offline violations of this constraint, while the link and rename
 * checks are needed to prevent online violations of this constraint.
 *
 * Return: 1 if permitted, 0 if forbidden.
 */
int fscrypt_has_permitted_context(struct inode *parent, struct inode *child)
{
	union fscrypt_policy parent_policy, child_policy;
	int err, err1, err2;

	/* No restrictions on file types which are never encrypted */
	if (!S_ISREG(child->i_mode) && !S_ISDIR(child->i_mode) &&
	    !S_ISLNK(child->i_mode))
		return 1;

	/* No restrictions if the parent directory is unencrypted */
	if (!IS_ENCRYPTED(parent))
		return 1;

	/* Encrypted directories must not contain unencrypted files */
	if (!IS_ENCRYPTED(child))
		return 0;

	/*
	 * Both parent and child are encrypted, so verify they use the same
	 * encryption policy.  Compare the cached policies if the keys are
	 * available, otherwise retrieve and compare the fscrypt_contexts.
	 *
	 * Note that the fscrypt_context retrieval will be required frequently
	 * when accessing an encrypted directory tree without the key.
	 * Performance-wise this is not a big deal because we already don't
	 * really optimize for file access without the key (to the extent that
	 * such access is even possible), given that any attempted access
	 * already causes a fscrypt_context retrieval and keyring search.
	 *
	 * In any case, if an unexpected error occurs, fall back to "forbidden".
	 */

	err = fscrypt_get_encryption_info(parent, true);
	if (err)
		return 0;
	err = fscrypt_get_encryption_info(child, true);
	if (err)
		return 0;

	err1 = fscrypt_get_policy(parent, &parent_policy);
	err2 = fscrypt_get_policy(child, &child_policy);

	/*
	 * Allow the case where the parent and child both have an unrecognized
	 * encryption policy, so that files with an unrecognized encryption
	 * policy can be deleted.
	 */
	if (err1 == -EINVAL && err2 == -EINVAL)
		return 1;

	if (err1 || err2)
		return 0;

	return fscrypt_policies_equal(&parent_policy, &child_policy);
}

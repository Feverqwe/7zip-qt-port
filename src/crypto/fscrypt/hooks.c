// SPDX-License-Identifier: GPL-2.0-only
/*
 * fs/crypto/hooks.c
 *
 * Encryption hooks for higher-level filesystem operations.
 */

#include "crypto/fscrypt/fscrypt_private.h"

/**
 * fscrypt_get_symlink() - get the target of an encrypted symlink
 * @inode: the symlink inode
 * @caddr: the on-disk contents of the symlink
 * @max_size: size of @caddr buffer
 * @done: if successful, will be set up to free the returned target if needed
 *
 * If the symlink's encryption key is available, we decrypt its target.
 * Otherwise, we encode its target for presentation.
 *
 * This may sleep, so the filesystem must have dropped out of RCU mode already.
 *
 * Return: the presentable symlink target or an ERR_PTR()
 */
const char *fscrypt_get_symlink(struct inode *inode, const void *caddr,
				unsigned int max_size,
				struct delayed_call *done)
{
	const struct fscrypt_symlink_data *sd;
	struct fscrypt_str cstr, pstr;
	int err;

	/* This is for encrypted symlinks only */
	if (WARN_ON_ONCE(!IS_ENCRYPTED(inode)))
		return ERR_PTR(-EINVAL);

	/*
	 * Try to set up the symlink's encryption key, but we can continue
	 * regardless of whether the key is available or not.
	 */
	err = fscrypt_get_encryption_info(inode, false);
	if (err)
		return ERR_PTR(err);

	/*
	 * For historical reasons, encrypted symlink targets are prefixed with
	 * the ciphertext length, even though this is redundant with i_size.
	 */

	if (max_size < sizeof(*sd) + 1)
		return ERR_PTR(-EUCLEAN);
	sd = caddr;
	cstr.name = (unsigned char *)sd->encrypted_path;
	cstr.len = get_unaligned_le16(&sd->len);

	if (cstr.len == 0)
		return ERR_PTR(-EUCLEAN);

	if (cstr.len + sizeof(*sd) > max_size)
		return ERR_PTR(-EUCLEAN);

	err = fscrypt_fname_alloc_buffer(cstr.len, &pstr);
	if (err)
		return ERR_PTR(err);

	err = fscrypt_fname_disk_to_usr(inode, 0, 0, &cstr, &pstr);
	if (err)
		goto err_kfree;

	err = -EUCLEAN;
	if (pstr.name[0] == '\0')
		goto err_kfree;

	pstr.name[pstr.len] = '\0';

	/* Archive callers own every result; there is no shared VFS cache. */
	set_delayed_call(done, kfree_link, pstr.name);

	return (const char *)pstr.name;

err_kfree:
	kfree(pstr.name);
	return ERR_PTR(err);
}
EXPORT_SYMBOL_GPL(fscrypt_get_symlink);

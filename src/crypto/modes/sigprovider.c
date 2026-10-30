/* SPDX-License-Identifier: GPL-2.0-only */
/* Fixed owned provider lookup for the original signature read paths. */
#include <crypto/internal/sig.h>
struct crypto_sig *crypto_alloc_sig(const char *name, u32 type, u32 mask)
{
	(void)type; (void)mask;
	size_t size = strlen(name);
	if (!strncmp(name, "ecdsa-", 6)) return sevenzip_ecdsa_alloc(name);
	if (!strcmp(name, "ecrdsa") || !strcmp(name, "ecrdsa-generic"))
		return sevenzip_ecrdsa_alloc();
	const char *prefix = !strncmp(name, "x962(", 5) ? "x962(" :
		!strncmp(name, "p1363(", 6) ? "p1363(" : NULL;
	if (prefix && size < CRYPTO_MAX_ALG_NAME && name[size - 1] == ')') {
		size_t offset = strlen(prefix);
		char child[CRYPTO_MAX_ALG_NAME];
		memcpy(child, name + offset, size - offset - 1);
		child[size - offset - 1] = 0;
		return offset == 5 ? sevenzip_ecdsa_x962_alloc(child) :
			sevenzip_ecdsa_p1363_alloc(child);
	}
	if (!strncmp(name, "mldsa", 5)) return sevenzip_mldsa_alloc(name);
	if (size >= 12 && size < CRYPTO_MAX_ALG_NAME &&
	    !strncmp(name, "pkcs1(rsa,", 10) && name[size - 1] == ')') {
		char hash[CRYPTO_MAX_ALG_NAME];
		memcpy(hash, name + 10, size - 11);
		hash[size - 11] = 0;
		return sevenzip_rsassa_pkcs1_alloc(hash);
	}
	return ERR_PTR(-ENOENT);
}

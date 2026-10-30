/* SPDX-License-Identifier: GPL-2.0-only */
/* Fixed owned public-key cipher providers for synchronous memory requests. */
#include <crypto/internal/akcipher.h>
const struct akcipher_alg *crypto_akcipher_alg(struct crypto_akcipher *tfm) { return tfm->algorithm; }
unsigned crypto_akcipher_reqsize(struct crypto_akcipher *tfm) { return tfm->reqsize; }
struct crypto_akcipher *crypto_alloc_akcipher(const char *name, u32 type, u32 mask)
{
	(void)type; (void)mask;
	if (strcmp(name, "rsa")) return ERR_PTR(-ENOENT);
	const struct akcipher_alg *algorithm = sevenzip_rsa_alg();
	struct crypto_akcipher *tfm = sevenzip_crypto_zalloc(sizeof(*tfm));
	if (!tfm) return ERR_PTR(-ENOMEM);
	tfm->algorithm = algorithm;
	tfm->ctx = sevenzip_crypto_zalloc(algorithm->base.cra_ctxsize);
	if (!tfm->ctx) { sevenzip_crypto_free_sensitive(tfm); return ERR_PTR(-ENOMEM); }
	if (algorithm->init) {
		int err = algorithm->init(tfm);
		if (err) {
			sevenzip_crypto_free_sensitive(tfm->ctx);
			sevenzip_crypto_free_sensitive(tfm);
			return ERR_PTR(err);
		}
	}
	return tfm;
}
void crypto_free_akcipher(struct crypto_akcipher *tfm)
{
	if (!tfm || IS_ERR(tfm)) return;
	if (tfm->algorithm->exit) tfm->algorithm->exit(tfm);
	sevenzip_crypto_free_sensitive(tfm->ctx);
	sevenzip_crypto_free_sensitive(tfm);
}

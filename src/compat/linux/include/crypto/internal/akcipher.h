/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Public Key Encryption: owned context/spawn adaptation.
 * Copyright (c) 2015, Intel Corporation
 * Authors: Tadeusz Struk <tadeusz.struk@intel.com> */
#ifndef SEVENZIP_CRYPTO_INTERNAL_AKCIPHER_H
#define SEVENZIP_CRYPTO_INTERNAL_AKCIPHER_H
#include <crypto/akcipher.h>
struct crypto_akcipher {
    const struct akcipher_alg *algorithm;
    void *ctx;
    unsigned reqsize;
};
struct crypto_akcipher_spawn {
    const char *name;
};
static inline void *akcipher_tfm_ctx(struct crypto_akcipher *tfm) { return tfm->ctx; }
static inline struct crypto_akcipher *crypto_spawn_akcipher(struct crypto_akcipher_spawn *spawn) {
    return crypto_alloc_akcipher(spawn->name, 0, 0);
}
const struct akcipher_alg *sevenzip_rsa_alg(void);
#endif

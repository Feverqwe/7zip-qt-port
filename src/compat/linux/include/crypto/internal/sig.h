/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Public Key Signature Algorithm: owned context/instance adaptation.
 * Copyright (c) 2023 Herbert Xu <herbert@gondor.apana.org.au> */
#ifndef SEVENZIP_CRYPTO_INTERNAL_SIG_H
#define SEVENZIP_CRYPTO_INTERNAL_SIG_H
#include <crypto/sig.h>
struct sig_instance {
    struct sig_alg alg;
    void *ctx;
};
struct crypto_sig {
    struct sig_instance *instance;
    void *ctx;
    bool initialized;
};
static inline void *crypto_sig_ctx(struct crypto_sig *tfm) { return tfm->ctx; }
static inline struct sig_instance *sig_alg_instance(struct crypto_sig *tfm) {
    return tfm->instance;
}
static inline void *sig_instance_ctx(struct sig_instance *instance) { return instance->ctx; }
struct crypto_sig *sevenzip_sig_alloc(const struct sig_alg *, size_t);
int sevenzip_sig_finish_init(struct crypto_sig *);
struct crypto_sig *sevenzip_rsassa_pkcs1_alloc(const char *);
struct crypto_sig *sevenzip_mldsa_alloc(const char *);
struct crypto_sig *sevenzip_ecdsa_alloc(const char *);
struct crypto_sig *sevenzip_ecdsa_x962_alloc(const char *);
struct crypto_sig *sevenzip_ecdsa_p1363_alloc(const char *);
struct crypto_sig *sevenzip_ecrdsa_alloc(void);
#endif

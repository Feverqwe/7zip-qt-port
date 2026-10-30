/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Generic public-key signature interface with owned process contexts.
 * Copyright (c) 2023 Herbert Xu <herbert@gondor.apana.org.au> */
#ifndef SEVENZIP_CRYPTO_SIG_H
#define SEVENZIP_CRYPTO_SIG_H
#include "crypto/modes/pkeycompat.h"
struct crypto_sig;
struct sig_alg {
    int (*sign)(struct crypto_sig *, const void *, unsigned, void *, unsigned);
    int (*verify)(struct crypto_sig *, const void *, unsigned, const void *, unsigned);
    int (*set_pub_key)(struct crypto_sig *, const void *, unsigned);
    int (*set_priv_key)(struct crypto_sig *, const void *, unsigned);
    unsigned (*key_size)(struct crypto_sig *);
    unsigned (*digest_size)(struct crypto_sig *);
    unsigned (*max_size)(struct crypto_sig *);
    int (*init)(struct crypto_sig *);
    void (*exit)(struct crypto_sig *);
    struct crypto_alg base;
};
struct crypto_sig *crypto_alloc_sig(const char *, u32, u32);
void crypto_free_sig(struct crypto_sig *);
struct sig_alg *crypto_sig_alg(struct crypto_sig *);
static inline int crypto_sig_set_pubkey(struct crypto_sig *tfm, const void *key, unsigned size) {
    return crypto_sig_alg(tfm)->set_pub_key(tfm, key, size);
}
static inline int crypto_sig_set_privkey(struct crypto_sig *tfm, const void *key, unsigned size) {
    return crypto_sig_alg(tfm)->set_priv_key(tfm, key, size);
}
static inline int crypto_sig_verify(struct crypto_sig *tfm, const void *sig, unsigned size,
                                    const void *message, unsigned bytes) {
    return crypto_sig_alg(tfm)->verify(tfm, sig, size, message, bytes);
}
static inline unsigned crypto_sig_keysize(struct crypto_sig *tfm) {
    return crypto_sig_alg(tfm)->key_size(tfm);
}
static inline unsigned crypto_sig_digestsize(struct crypto_sig *tfm) {
    return crypto_sig_alg(tfm)->digest_size(tfm);
}
static inline unsigned crypto_sig_maxsize(struct crypto_sig *tfm) {
    return crypto_sig_alg(tfm)->max_size(tfm);
}
#endif

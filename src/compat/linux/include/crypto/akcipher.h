/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Public Key Encryption: selected synchronous interface and owned context.
 * Copyright (c) 2015, Intel Corporation
 * Authors: Tadeusz Struk <tadeusz.struk@intel.com> */
#ifndef SEVENZIP_CRYPTO_AKCIPHER_H
#define SEVENZIP_CRYPTO_AKCIPHER_H
#include "crypto/modes/pkeycompat.h"
struct crypto_akcipher;
struct akcipher_request {
    struct crypto_akcipher *tfm;
    struct scatterlist *src, *dst;
    unsigned src_len, dst_len;
    struct {
        u32 flags;
        crypto_completion_t complete;
        void *data;
    } base;
};
struct akcipher_alg {
    int (*encrypt)(struct akcipher_request *);
    int (*decrypt)(struct akcipher_request *);
    int (*set_pub_key)(struct crypto_akcipher *, const void *, unsigned);
    int (*set_priv_key)(struct crypto_akcipher *, const void *, unsigned);
    unsigned (*max_size)(struct crypto_akcipher *);
    int (*init)(struct crypto_akcipher *);
    void (*exit)(struct crypto_akcipher *);
    struct crypto_alg base;
};
struct crypto_akcipher *crypto_alloc_akcipher(const char *, u32, u32);
void crypto_free_akcipher(struct crypto_akcipher *);
const struct akcipher_alg *crypto_akcipher_alg(struct crypto_akcipher *);
static inline struct crypto_akcipher *crypto_akcipher_reqtfm(struct akcipher_request *req) {
    return req->tfm;
}
unsigned crypto_akcipher_reqsize(struct crypto_akcipher *);
static inline void akcipher_request_set_tfm(struct akcipher_request *req,
                                            struct crypto_akcipher *tfm) {
    req->tfm = tfm;
}
static inline void akcipher_request_set_crypt(struct akcipher_request *req, struct scatterlist *src,
                                              struct scatterlist *dst, unsigned src_len,
                                              unsigned dst_len) {
    req->src = src;
    req->dst = dst;
    req->src_len = src_len;
    req->dst_len = dst_len;
}
static inline void akcipher_request_set_callback(struct akcipher_request *req, u32 flags,
                                                 crypto_completion_t complete, void *data) {
    req->base.flags = flags;
    req->base.complete = complete;
    req->base.data = data;
}
static inline int crypto_akcipher_encrypt(struct akcipher_request *req) {
    return crypto_akcipher_alg(req->tfm)->encrypt(req);
}
static inline int crypto_akcipher_decrypt(struct akcipher_request *req) {
    return crypto_akcipher_alg(req->tfm)->decrypt(req);
}
static inline int crypto_akcipher_set_pub_key(struct crypto_akcipher *tfm, const void *key,
                                              unsigned size) {
    return crypto_akcipher_alg(tfm)->set_pub_key(tfm, key, size);
}
static inline int crypto_akcipher_set_priv_key(struct crypto_akcipher *tfm, const void *key,
                                               unsigned size) {
    return crypto_akcipher_alg(tfm)->set_priv_key(tfm, key, size);
}
static inline unsigned crypto_akcipher_maxsize(struct crypto_akcipher *tfm) {
    return crypto_akcipher_alg(tfm)->max_size(tfm);
}
int crypto_akcipher_sync_encrypt(struct crypto_akcipher *, const void *, unsigned, void *,
                                 unsigned);
int crypto_akcipher_sync_decrypt(struct crypto_akcipher *, const void *, unsigned, void *,
                                 unsigned);
#endif

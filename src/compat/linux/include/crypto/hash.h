/* SPDX-License-Identifier: GPL-2.0-only */
/* Owned synchronous hash contexts for the original Linux certificate readers. */
#ifndef SEVENZIP_CRYPTO_HASH_H
#define SEVENZIP_CRYPTO_HASH_H
#include "crypto/modes/skciphercompat.h"
#include <crypto/sha1.h>
#include <crypto/sha3.h>
#include <crypto/sm3.h>
#include <crypto/streebog.h>

#define CRYPTO_AHASH_ALG_BLOCK_ONLY 0x01000000
#define CRYPTO_AHASH_ALG_FINAL_NONZERO 0x02000000
#define CRYPTO_AHASH_ALG_FINUP_MAX 0x04000000
#define CRYPTO_TFM_NEED_KEY 0x00000001
#define THIS_MODULE NULL

union sevenzip_shash_state {
    struct sha1_ctx sha1;
    struct sha224_ctx sha224;
    struct sha256_ctx sha256;
    struct sha384_ctx sha384;
    struct sha512_ctx sha512;
    struct sha3_ctx sha3;
    struct sm3_ctx sm3;
    struct {
        struct streebog_state state;
        u8 buffer[STREEBOG_BLOCK_SIZE + 1];
    } streebog;
};
struct shash_desc {
    struct crypto_shash *tfm;
    union sevenzip_shash_state state;
};
/* Selected synchronous callbacks and provider metadata from shash_alg.
 * Context storage is bounded for the fixed providers; no VLA is required. */
struct shash_alg {
    int (*init)(struct shash_desc *);
    int (*update)(struct shash_desc *, const u8 *, unsigned);
    int (*final)(struct shash_desc *, u8 *);
    int (*finup)(struct shash_desc *, const u8 *, unsigned, u8 *);
    int (*digest)(struct shash_desc *, const u8 *, unsigned, u8 *);
    unsigned descsize, digestsize;
    struct {
        const char *cra_name, *cra_driver_name;
        u32 cra_flags;
        unsigned cra_blocksize;
        const void *cra_module;
    } base;
};
struct crypto_shash {
    struct shash_alg algorithm;
    u32 flags;
};
#define SHASH_DESC_ON_STACK(name, tfm)                                                             \
    struct shash_desc name##_storage;                                                              \
    struct shash_desc *name = &name##_storage
static inline void *shash_desc_ctx(struct shash_desc *desc) { return &desc->state; }
static inline struct shash_alg *crypto_shash_alg(struct crypto_shash *tfm) {
    return &tfm->algorithm;
}
static inline unsigned crypto_shash_blocksize(struct crypto_shash *tfm) {
    return tfm->algorithm.base.cra_blocksize;
}
static inline u32 crypto_shash_get_flags(struct crypto_shash *tfm) { return tfm->flags; }
unsigned crypto_shash_descsize(const struct crypto_shash *);
int crypto_shash_init(struct shash_desc *);
int crypto_shash_finup(struct shash_desc *, const u8 *, unsigned, u8 *);
static inline int crypto_shash_update(struct shash_desc *desc, const u8 *data, unsigned len) {
    return crypto_shash_finup(desc, data, len, NULL);
}
static inline int crypto_shash_final(struct shash_desc *desc, u8 *out) {
    return crypto_shash_finup(desc, NULL, 0, out);
}
int crypto_shash_digest(struct shash_desc *, const u8 *, unsigned, u8 *);
int shash_default_finup(struct shash_desc *, const u8 *, unsigned, u8 *);
int shash_default_digest(struct shash_desc *, const u8 *, unsigned, u8 *);
const struct shash_alg *sevenzip_streebog_alg(unsigned);
#endif

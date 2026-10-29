/* SPDX-License-Identifier: GPL-2.0-only */
/* Synchronous, owned-buffer Crypto API services for archive fscrypt reads.
 * Requests present a single bounded memory segment; no kernel pages, DMA or
 * asynchronous hardware provider is involved. Algorithm bodies stay in their
 * original Linux source modules. */
#ifndef SEVENZIP_SKCIPHER_COMPAT_H
#define SEVENZIP_SKCIPHER_COMPAT_H
#include <crypto/aes.h>
#include <crypto/sha2.h>
#include <crypto/utils.h>
#include <stdlib.h>
#ifndef ENOKEY
#define ENOKEY 126
#endif

#define CRYPTO_TFM_REQ_MASK 0x000fff00
#define CRYPTO_TFM_REQ_FORBID_WEAK_KEYS 0x00000100
#define CRYPTO_TFM_REQ_MAY_SLEEP 0x00000200
#define CRYPTO_TFM_REQ_MAY_BACKLOG 0x00000400
#define GFP_KERNEL 0
#define SG_MITER_ATOMIC 1
#define SG_MITER_TO_SG 2
#define SG_MITER_FROM_SG 4
#define SG_MITER_LOCAL 8
#define HASH_MAX_DIGESTSIZE 64
#define MAX_CIPHER_BLOCKSIZE 16
#define ALIGN(n, a) (((n) + (a) - 1) & ~((size_t)(a) - 1))
#define rounddown(n, a) ((n) - (n) % (a))
#define PTR_ALIGN(p, a) ((void *)ALIGN((uintptr_t)(p), (a)))
#define min_t(t, a, b) ((t)(a) < (t)(b) ? (t)(a) : (t)(b))
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#define offsetofend(t, member) (offsetof(t, member) + sizeof(((t *)0)->member))
#define BUILD_BUG_ON(e)                                                                            \
    do {                                                                                           \
        _Static_assert(!(e), #e);                                                                  \
    } while (0)
#define ERR_PTR(e) ((void *)(intptr_t)(e))
#define PTR_ERR(p) ((int)(intptr_t)(p))
#define IS_ERR(p) ((uintptr_t)(p) >= (uintptr_t)-4095)

/* A synthetic page is a bounded span of ordinary process memory. */
#define SEVENZIP_CRYPTO_PAGE_SIZE 4096
struct page {
    u8 *data;
};
struct scatterlist {
    u8 *data;
    union {
        size_t len, length;
    };
    size_t offset;
    struct page page;
};
static inline void sg_init_one(struct scatterlist *sg, void *data, size_t len) {
    sg->data = data;
    sg->len = len;
    sg->offset = 0;
    sg->page.data = data;
}
static inline void sg_init_table(struct scatterlist *sg, unsigned count) {
    memset(sg, 0, count * sizeof(*sg));
}
static inline void sg_set_page(struct scatterlist *sg, struct page *page, unsigned len,
                               unsigned offset) {
    sg->data = page->data + offset;
    sg->len = len;
    sg->offset = offset;
    sg->page = *page;
}
static inline struct scatterlist *scatterwalk_ffwd(struct scatterlist *out, struct scatterlist *in,
                                                   size_t offset) {
    sg_init_one(out, in->data + offset, in->len - offset);
    return out;
}
static inline void scatterwalk_map_and_copy(void *buffer, struct scatterlist *sg, size_t offset,
                                            size_t len, bool write) {
    if (len) {
        if (write)
            memcpy(sg->data + offset, buffer, len);
        else
            memcpy(buffer, sg->data + offset, len);
    }
}
struct sg_page_iter {
    struct scatterlist *sg;
    unsigned long sg_pgoffset;
    unsigned __nents, __pg_advance;
};
struct sg_mapping_iter {
    struct page *page;
    void *addr;
    size_t length, consumed;
    struct sg_page_iter piter;
    size_t __offset, __remaining;
    unsigned __flags;
    struct page mapped_page;
};
static inline unsigned sg_nents(const struct scatterlist *sg) {
    (void)sg;
    return 1;
}
static inline int sg_nents_for_len(const struct scatterlist *sg, unsigned len) {
    return len > sg->len ? -EINVAL : len ? 1 : 0;
}
void sg_miter_start(struct sg_mapping_iter *, struct scatterlist *, unsigned, unsigned);
bool sg_miter_next(struct sg_mapping_iter *);
void sg_miter_stop(struct sg_mapping_iter *);
static inline struct page *sg_page(struct scatterlist *sg) { return &sg->page; }
static inline u8 *kmap_local_page(struct page *page) { return page->data; }
static inline void kunmap_local(const void *p) { (void)p; }
static inline void flush_dcache_page(struct page *p) { (void)p; }
static inline void memcpy_from_sglist(void *out, struct scatterlist *sg, size_t off, size_t len) {
    scatterwalk_map_and_copy(out, sg, off, len, false);
}
static inline void memcpy_to_sglist(struct scatterlist *sg, size_t off, const void *in,
                                    size_t len) {
    scatterwalk_map_and_copy((void *)in, sg, off, len, true);
}
struct scatter_walk {
    u8 *addr;
    size_t remaining;
};
static inline void scatterwalk_start(struct scatter_walk *walk, struct scatterlist *sg) {
    walk->addr = sg->data;
    walk->remaining = sg->len;
}
static inline unsigned scatterwalk_next(struct scatter_walk *walk, unsigned len) {
    size_t page_left =
        SEVENZIP_CRYPTO_PAGE_SIZE - ((uintptr_t)walk->addr & (SEVENZIP_CRYPTO_PAGE_SIZE - 1));
    return (unsigned)min(len, min(walk->remaining, page_left));
}
static inline void scatterwalk_done_src(struct scatter_walk *walk, unsigned len) {
    walk->addr += len;
    walk->remaining -= len;
}

struct crypto_wait {
    int status;
};
static inline void crypto_init_wait(struct crypto_wait *wait) { wait->status = 0; }
static inline void crypto_req_done(void *data, int status) {
    ((struct crypto_wait *)data)->status = status;
}
static inline int crypto_wait_req(int status, struct crypto_wait *wait) {
    (void)wait;
    return status;
}
void *sevenzip_crypto_zalloc(size_t);
void sevenzip_crypto_free_sensitive(void *);
#define kzalloc(size, flags) sevenzip_crypto_zalloc(size)
#define kfree_sensitive(p) sevenzip_crypto_free_sensitive(p)
#define WARN_ON(e) crypto_warn_on(e)

struct skcipher_request;
struct crypto_skcipher {
    void *ctx;
    size_t ctxsize, reqsize;
    unsigned ivsize;
    unsigned blocksize;
    u32 flags;
    bool key_ready;
    int (*setkey)(struct crypto_skcipher *, const u8 *, unsigned);
    int (*encrypt)(struct skcipher_request *);
    int (*decrypt)(struct skcipher_request *);
    void (*exit)(struct crypto_skcipher *);
};
struct crypto_tfm {
    void *ctx;
};
struct crypto_cipher {
    struct crypto_tfm base;
    size_t ctxsize;
    u32 flags;
    int (*setkey)(struct crypto_tfm *, const u8 *, unsigned);
    void (*encrypt)(struct crypto_tfm *, u8 *, const u8 *);
    void (*decrypt)(struct crypto_tfm *, u8 *, const u8 *);
};
struct crypto_lskcipher {
    struct crypto_lskcipher *child;
    struct crypto_cipher *block;
};
#define CRYPTO_LSKCIPHER_FLAG_FINAL 0x00000002
struct crypto_shash;
struct skcipher_request {
    struct {
        u32 flags;
        void (*complete)(void *, int);
        void *data;
    } base;
    struct crypto_skcipher *tfm;
    struct scatterlist *src, *dst;
    unsigned cryptlen;
    u8 *iv;
};
typedef void (*crypto_completion_t)(void *, int);
struct skcipher_walk {
    struct {
        struct {
            void *addr;
        } virt;
    } src, dst;
    unsigned nbytes, total;
    struct skcipher_request *req;
    u8 *input, *output;
};
int skcipher_walk_virt(struct skcipher_walk *, struct skcipher_request *, bool);
int skcipher_walk_done(struct skcipher_walk *, int);
struct crypto_skcipher *sevenzip_skcipher_create(size_t ctxsize);
struct crypto_skcipher *sevenzip_hctr2_alloc(void);
struct crypto_skcipher *sevenzip_aes_alloc(const char *);
struct crypto_skcipher *sevenzip_essiv_alloc(void);
struct crypto_skcipher *sevenzip_cts_alloc(const char *);
struct crypto_skcipher *sevenzip_cbc_alloc(const char *);
struct crypto_skcipher *sevenzip_ecb_alloc(const char *);
struct crypto_skcipher *sevenzip_xts_alloc(const char *, const char *);
struct crypto_skcipher *sevenzip_chacha_alloc(const char *);
struct crypto_skcipher *sevenzip_adiantum_alloc(const char *);
struct crypto_cipher *sevenzip_cipher_create(size_t);
struct crypto_cipher *sevenzip_sm4_alloc(void);
struct crypto_skcipher *crypto_alloc_skcipher(const char *, unsigned, unsigned);
void crypto_free_skcipher(struct crypto_skcipher *);
struct crypto_cipher *crypto_alloc_cipher(const char *, unsigned, unsigned);
void crypto_free_cipher(struct crypto_cipher *);
struct crypto_shash *crypto_alloc_shash(const char *, unsigned, unsigned);
void crypto_free_shash(struct crypto_shash *);
unsigned crypto_shash_digestsize(const struct crypto_shash *);
int crypto_shash_tfm_digest(struct crypto_shash *, const u8 *, unsigned, u8 *);
struct skcipher_request *skcipher_request_alloc(struct crypto_skcipher *, unsigned);
void skcipher_request_free(struct skcipher_request *);
int crypto_skcipher_encrypt(struct skcipher_request *);
int crypto_skcipher_decrypt(struct skcipher_request *);
static inline void *crypto_skcipher_ctx(struct crypto_skcipher *tfm) { return tfm->ctx; }
static inline void *skcipher_request_ctx(struct skcipher_request *req) { return req + 1; }
static inline struct crypto_skcipher *crypto_skcipher_reqtfm(struct skcipher_request *req) {
    return req->tfm;
}
static inline u32 skcipher_request_flags(const struct skcipher_request *req) {
    return req->base.flags;
}
static inline size_t crypto_skcipher_reqsize(const struct crypto_skcipher *tfm) {
    return tfm->reqsize;
}
static inline unsigned crypto_skcipher_blocksize(const struct crypto_skcipher *tfm) {
    return tfm->blocksize;
}
static inline unsigned crypto_skcipher_alignmask(const struct crypto_skcipher *tfm) {
    (void)tfm;
    return 0;
}
/* Generic contexts contain pointers and u64 words, without SIMD kernel state. */
static inline size_t crypto_tfm_ctx_alignment(void) { return _Alignof(u64); }
static inline void crypto_skcipher_set_reqsize(struct crypto_skcipher *tfm, size_t size) {
    tfm->reqsize = size;
}
static inline u32 crypto_skcipher_get_flags(const struct crypto_skcipher *tfm) {
    return tfm->flags;
}
static inline void crypto_skcipher_clear_flags(struct crypto_skcipher *tfm, u32 flags) {
    tfm->flags &= ~flags;
}
static inline void crypto_skcipher_set_flags(struct crypto_skcipher *tfm, u32 flags) {
    tfm->flags |= flags;
}
static inline int crypto_skcipher_setkey(struct crypto_skcipher *tfm, const u8 *key, unsigned len) {
    tfm->key_ready = false;
    int err = tfm->setkey(tfm, key, len);
    tfm->key_ready = err == 0;
    return err;
}
static inline void crypto_cipher_clear_flags(struct crypto_cipher *tfm, u32 flags) {
    tfm->flags &= ~flags;
}
static inline void crypto_cipher_set_flags(struct crypto_cipher *tfm, u32 flags) {
    tfm->flags |= flags;
}
static inline void *crypto_tfm_ctx(struct crypto_tfm *tfm) { return tfm->ctx; }
static inline unsigned crypto_cipher_blocksize(const struct crypto_cipher *tfm) {
    (void)tfm;
    return 16;
}
static inline struct crypto_tfm *crypto_cipher_tfm(struct crypto_cipher *tfm) { return &tfm->base; }
static inline int crypto_cipher_setkey(struct crypto_cipher *tfm, const u8 *key, unsigned len) {
    return tfm->setkey(&tfm->base, key, len);
}
static inline void crypto_cipher_encrypt_one(struct crypto_cipher *tfm, u8 *dst, const u8 *src) {
    tfm->encrypt(&tfm->base, dst, src);
}
static inline void crypto_cipher_decrypt_one(struct crypto_cipher *tfm, u8 *dst, const u8 *src) {
    tfm->decrypt(&tfm->base, dst, src);
}
static inline unsigned crypto_lskcipher_blocksize(const struct crypto_lskcipher *tfm) {
    (void)tfm;
    return 16;
}
static inline void *crypto_lskcipher_ctx(struct crypto_lskcipher *tfm) { return &tfm->child; }
static inline int crypto_lskcipher_encrypt(struct crypto_lskcipher *tfm, const u8 *src, u8 *dst,
                                           unsigned len, u8 *iv) {
    (void)iv;
    if (len != 16)
        return -EINVAL;
    crypto_cipher_encrypt_one(tfm->block, dst, src);
    return 0;
}
static inline int crypto_lskcipher_decrypt(struct crypto_lskcipher *tfm, const u8 *src, u8 *dst,
                                           unsigned len, u8 *iv) {
    (void)iv;
    if (len != 16)
        return -EINVAL;
    crypto_cipher_decrypt_one(tfm->block, dst, src);
    return 0;
}
static inline void skcipher_request_set_tfm(struct skcipher_request *req,
                                            struct crypto_skcipher *tfm) {
    req->tfm = tfm;
}
static inline void skcipher_request_set_crypt(struct skcipher_request *req, struct scatterlist *src,
                                              struct scatterlist *dst, unsigned len, void *iv) {
    req->src = src;
    req->dst = dst;
    req->cryptlen = len;
    req->iv = iv;
}
static inline void skcipher_request_set_callback(struct skcipher_request *req, u32 flags,
                                                 void (*complete)(void *, int), void *data) {
    req->base.flags = flags;
    req->base.complete = complete;
    req->base.data = data;
}
static inline void skcipher_request_complete(struct skcipher_request *req, int err) {
    if (req->base.complete)
        req->base.complete(req->base.data, err);
}
#endif

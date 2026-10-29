// SPDX-License-Identifier: MIT
#ifndef SEVENZIP_SQUASHFS_COMPAT_H
#define SEVENZIP_SQUASHFS_COMPAT_H
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef uint16_t __le16;
typedef uint32_t __le32;
typedef uint64_t __le64;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;
typedef int64_t loff_t;
typedef uint64_t pgoff_t;
#define PAGE_SIZE 65536
#define PAGE_SHIFT 16
#define GFP_KERNEL 0
#define CONFIG_SQUASHFS_LZ4 1
#define CONFIG_SQUASHFS_LZO 1
#define CONFIG_SQUASHFS_XZ 1
#define CONFIG_SQUASHFS_ZLIB 1
#define CONFIG_SQUASHFS_ZSTD 1
#define CONFIG_SQUASHFS_XATTR 1
#define likely(x) (x)
#define unlikely(x) (x)
#define ERROR(...) ((void)0)
#define TRACE(...) ((void)0)
#define WARN_ON_ONCE(x) (x)
#define panic(...) abort()
#define max_t(t, a, b) ((t)(a) > (t)(b) ? (t)(a) : (t)(b))
#define min_t(t, a, b) ((t)(a) < (t)(b) ? (t)(a) : (t)(b))
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#define kmalloc(n, flags) malloc(n)
#define kcalloc(n, s, flags) calloc(n, s)
#define kmalloc_obj(obj) malloc(sizeof(obj))
#define kzalloc_obj(obj) calloc(1, sizeof(obj))
#define kzalloc_objs(obj, n) calloc(n, sizeof(obj))
#define kfree(p) free(p)
#define vmalloc(n) malloc(n)
#define vfree(p) free(p)
#define ERR_PTR(n) ((void *)(intptr_t)(n))
#define PTR_ERR(p) ((int)(intptr_t)(p))
#define IS_ERR(p) ((uintptr_t)(p) >= (uintptr_t)-4095)
static inline uint32_t le32_to_cpu(__le32 v) {
    const uint8_t *b = (const uint8_t *)&v;
    return (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}
static inline uint16_t le16_to_cpu(__le16 v) {
    const uint8_t *b = (const uint8_t *)&v;
    return (uint16_t)(b[0] | (uint16_t)b[1] << 8);
}
static inline uint64_t le64_to_cpu(__le64 v) {
    const uint8_t *b = (const uint8_t *)&v;
    uint64_t value = 0;
    unsigned i;
    for (i = 0; i < 8; ++i)
        value |= (uint64_t)b[i] << (8 * i);
    return value;
}
static inline int sevenzip_squashfs_ffs(int n) {
    unsigned value = (unsigned)n;
    int bit = 1;
    if (!value)
        return 0;
    while (!(value & 1)) {
        value >>= 1;
        ++bit;
    }
    return bit;
}
#define ffs sevenzip_squashfs_ffs
struct squashfs_sb_info;
struct squashfs_page_actor;
struct bio;
struct super_block {
    struct squashfs_sb_info *s_fs_info;
};
struct squashfs_decompressor_thread_ops {
    void *(*create)(struct squashfs_sb_info *, void *);
    void (*destroy)(struct squashfs_sb_info *);
    int (*decompress)(struct squashfs_sb_info *, struct bio *, int, int,
                      struct squashfs_page_actor *);
    int (*max_decompressors)(void);
};
struct mutex {
    int unused;
};
#define mutex_init(p) ((void)(p))
#define mutex_lock(p) ((void)(p))
#define mutex_unlock(p) ((void)(p))
struct bio_vec {
    const void *data;
    unsigned int bv_len;
};
struct bio {
    struct bio_vec segment;
};
struct bvec_iter_all {
    struct bio_vec bv;
    int consumed;
};
static inline struct bio_vec *bvec_init_iter_all(struct bvec_iter_all *iter) { return &iter->bv; }
static inline int bio_next_segment(struct bio *bio, struct bvec_iter_all *iter) {
    if (iter->consumed)
        return 0;
    iter->bv = bio->segment;
    iter->consumed = 1;
    return 1;
}
static inline void *bvec_virt(struct bio_vec *segment) { return (void *)segment->data; }
static inline void bio_free_pages(struct bio *bio) { free((void *)bio->segment.data); }
#define bio_uninit(bio) ((void)(bio))
int squashfs_bio_read(struct super_block *, u64, int, struct bio **, int *);
int lzo1x_decompress_safe(const unsigned char *, size_t, unsigned char *, size_t *);
#include "../../include/linux/lzo.h"
void *sevenzip_squashfs_external_init(struct squashfs_sb_info *, void *);
void sevenzip_squashfs_external_free(void *);
#endif

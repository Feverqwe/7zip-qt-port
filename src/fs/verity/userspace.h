/* SPDX-License-Identifier: GPL-2.0-only */
/* Immutable archive services for the transferred Linux fs-verity readers. */
#ifndef SEVENZIP_FSVERITY_USERSPACE_H
#define SEVENZIP_FSVERITY_USERSPACE_H
#include "../../../third_party/7zip/C/Sha256.h"
#include "../../../third_party/7zip/C/Sha512.h"
#include "../../include/linux/string.h"
#include "../ext4/ext4.h"
#include "../ext4/ext4_extents.h"
/* Windows SAL defines this annotation name, also used by the Linux disk ABI. */
#ifdef __reserved
#undef __reserved
#endif
#include "../../include/uapi/linux/fsverity.h"
#define PAGE_SHIFT 16
#define PAGE_SIZE ((size_t)1 << PAGE_SHIFT)
#define PAGE_MASK (~((u64)PAGE_SIZE - 1))
#define PAGE_ALIGN(n) (((n) + PAGE_SIZE - 1) & PAGE_MASK)
#define KM_MAX_IDX 16
#define GFP_KERNEL 0
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define round_up(n, a) (((n) + (a) - 1) & ~((u64)(a) - 1))
#define round_down(n, a) ((n) & ~((u64)(a) - 1))
#define is_power_of_2(n) ((n) && !((n) & ((n) - 1)))
#define IS_ALIGNED(n, a) (!((n) & ((a) - 1)))
#define WARN_ON_ONCE(n) (n)
#define static_assert(expr) _Static_assert((expr), #expr)
#undef kfree
#define kfree(ptr) free((void *)(ptr))
#define smp_rmb() ((void)0)
#define smp_wmb() ((void)0)
#define kmem_cache_zalloc(cache, flags) calloc(1, sizeof(struct fsverity_info))
#define kmem_cache_free(cache, ptr) free(ptr)
#define kzalloc(size, flags) calloc(1, size)
#define kvcalloc(count, size, flags) calloc(count, size)
#define kvfree free
#define BUG() abort()
#define ERR_PTR(n) ((void *)(intptr_t)(n))
#define IS_ERR(p) ((uintptr_t)(p) >= (uintptr_t)-4095)
#define PTR_ERR(p) ((int)(intptr_t)(p))
#define ERR_CAST(p) ((void *)(p))
#define fsverity_err(inode, ...) ((void)0)
#define fsverity_warn(inode, ...) ((void)0)
#define trace_fsverity_verify_data_block(...) ((void)0)
#define trace_fsverity_verify_merkle_block(...) ((void)0)
#define trace_fsverity_merkle_hit(...) ((void)0)
typedef u64 pgoff_t;
/* Preserve Linux 64-bit file/tree indices on LLP64 hosts too. */
typedef u64 fsverity_index_t;
#define FSVERITY_INDEX_MAX UINT64_MAX
enum hash_algo { HASH_ALGO_SHA256 = 4, HASH_ALGO_SHA512 = 6 };
struct sha256_ctx {
    CSha256 state;
};
struct sha512_ctx {
    CSha512 state;
};
static inline void sha256_init(struct sha256_ctx *p) { Sha256_Init(&p->state); }
static inline void sha256_update(struct sha256_ctx *p, const void *data, size_t size) {
    Sha256_Update(&p->state, data, size);
}
static inline void sha256_final(struct sha256_ctx *p, u8 *out) { Sha256_Final(&p->state, out); }
static inline void sha512_init(struct sha512_ctx *p) { Sha512_Init(&p->state, SHA512_DIGEST_SIZE); }
static inline void sha512_update(struct sha512_ctx *p, const void *data, size_t size) {
    Sha512_Update(&p->state, data, size);
}
static inline void sha512_final(struct sha512_ctx *p, u8 *out) {
    Sha512_Final(&p->state, out, SHA512_DIGEST_SIZE);
}
static inline void sha256(const void *data, size_t size, u8 *out) {
    struct sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, size);
    sha256_final(&ctx, out);
}
static inline void sha512(const void *data, size_t size, u8 *out) {
    struct sha512_ctx ctx;
    sha512_init(&ctx);
    sha512_update(&ctx, data, size);
    sha512_final(&ctx, out);
}
static inline bool sha256_finup_2x_is_optimized(void) { return false; }
static inline void sha256_finup_2x(const struct sha256_ctx *initial, const void *a, const void *b,
                                   size_t size, u8 *out_a, u8 *out_b) {
    struct sha256_ctx ctx;
    if (initial)
        ctx = *initial;
    else
        sha256_init(&ctx);
    sha256_update(&ctx, a, size);
    sha256_final(&ctx, out_a);
    if (initial)
        ctx = *initial;
    else
        sha256_init(&ctx);
    sha256_update(&ctx, b, size);
    sha256_final(&ctx, out_b);
}
static inline void *kmemdup(const void *data, size_t size, int flags) {
    void *result = malloc(size);
    if (result)
        memcpy(result, data, size);
    return result;
}
static inline u64 le64_to_cpu(__le64 value) {
    return (u64)ext_load_le32(&value) | (u64)ext_load_le32((const char *)&value + 4) << 32;
}
static inline unsigned int ilog2(u64 n) {
    unsigned int result = 0;
    while (n >>= 1)
        ++result;
    return result;
}
static inline void *page_address(const void *page) { return (void *)page; }
static inline const void *zero_page(void) {
    static const u8 zero[PAGE_SIZE] = {0};
    return zero;
}
#define ZERO_PAGE(n) zero_page()
static inline bool test_bit(u64 bit, const unsigned long *map) {
    return (map[bit / (sizeof(long) * 8)] >> (bit % (sizeof(long) * 8))) & 1;
}
static inline void clear_bit(u64 bit, unsigned long *map) {
    map[bit / (sizeof(long) * 8)] &= ~(1UL << (bit % (sizeof(long) * 8)));
}
static inline void set_bit(u64 bit, unsigned long *map) {
    map[bit / (sizeof(long) * 8)] |= 1UL << (bit % (sizeof(long) * 8));
}
#define BITS_TO_LONGS(n) (((n) + sizeof(long) * 8 - 1) / (sizeof(long) * 8))
struct page {
    u8 *data;
    u64 index;
    size_t base_offset;
    bool checked;
    unsigned int references;
    u64 verified_blocks, age;
};
#define folio page
#define PageChecked(p) ((p)->checked)
static inline bool fsverity_needs_verified_bitmap(void) { return false; }
#define SetPageChecked(p) ((p)->checked = true)
#define kmap_local_page(p) ((p)->data)
#define kmap_local_folio(p, offset) ((p)->data + ((offset) - (p)->base_offset))
#define kunmap_local(p) ((void)0)
static inline void verity_put_page(struct page *page) {
    if (--page->references == 0)
        free(page);
}
#define put_page verity_put_page
#define folio_put verity_put_page
#define folio_file_page(p, index) (p)
#define folio_test_locked(p) true
#define folio_test_uptodate(p) false
static inline bool fsverity_page_block_verified(const struct page *page, u64 index, unsigned bits) {
    return (page->verified_blocks >> (index & ((1U << bits) - 1))) & 1;
}
static inline void fsverity_mark_page_block_verified(struct page *page, u64 index, unsigned bits) {
    page->verified_blocks |= (u64)1 << (index & ((1U << bits) - 1));
}
struct page *read_mapping_folio(struct inode *, pgoff_t, void *);
static inline size_t memcpy_from_file_folio(void *output, struct page *page, u64 pos, size_t size) {
    size_t offset = pos & (PAGE_SIZE - 1);
    size_t amount = min_t(size_t, size, PAGE_SIZE - offset);
    memcpy(output, page->data + offset, amount);
    return amount;
}
#include "../../include/linux/fsverity.h"
struct page *generic_read_merkle_tree_page(struct inode *, pgoff_t);
#define EXT4_LBLK_TO_B(inode, block) ((u64)(block) << (inode)->i_blkbits)
#define i_blocksize(inode) ((inode)->i_sb->s_blocksize)
#define EXT4_INODE_EXTENTS 19
#define EXT_MAX_BLOCKS 0xFFFFFFFFU
struct ext4_ext_path *ext4_find_extent(struct inode *, u32, void *, int);
#define ext4_free_ext_path free
#endif

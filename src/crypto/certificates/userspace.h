/* SPDX-License-Identifier: GPL-2.0-only */
/* Owned allocation and scalar services for Linux asymmetric-key readers. */
#ifndef SEVENZIP_ASYMMETRIC_USERSPACE_H
#define SEVENZIP_ASYMMETRIC_USERSPACE_H
#include "crypto/modes/skciphercompat.h"
#include <linux/asn1.h>
#include <linux/asn1_decoder.h>
#ifndef ENOPKG
#define ENOPKG 65
#endif
#ifndef EKEYREJECTED
#define EKEYREJECTED 129
#endif
#ifndef ENOTSUPP
#define ENOTSUPP 524
#endif
#define ERR_CAST(ptr) ((void *)(ptr))
typedef int64_t time64_t;
time64_t mktime64(unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
#define pr_debug(...) ((void)0)
#define pr_devel(...) ((void)0)
#define pr_warn(...) ((void)0)
#define pr_err(...) ((void)0)
#define printk(...) ((void)0)
#define MODULE_AUTHOR(value)
#define pr_notice_once(...) ((void)0)
#define might_sleep() ((void)0)
#define umax(a, b) max(a, b)
int match_string(const char *const *, size_t, const char *);
static inline bool test_bit(unsigned bit, const unsigned long *set) {
    return (*set & (1UL << bit)) != 0;
}
static inline bool __test_and_set_bit(unsigned bit, unsigned long *set) {
    bool old = test_bit(bit, set);
    *set |= 1UL << bit;
    return old;
}
#define kmalloc(size, flags) sevenzip_crypto_zalloc(size)
#define kfree(ptr) sevenzip_crypto_free_sensitive(ptr)
#define kzalloc_obj(type) sevenzip_crypto_zalloc(sizeof(type))
static inline bool asymmetric_check_add_overflow(size_t a, size_t b, size_t *result) {
    *result = a + b;
    return *result < a;
}
#define check_add_overflow asymmetric_check_add_overflow
static inline void *kmemdup(const void *input, size_t size, int flags) {
    (void)flags;
    void *out = sevenzip_crypto_zalloc(size);
    if (out && size)
        memcpy(out, input, size);
    return out;
}
#endif

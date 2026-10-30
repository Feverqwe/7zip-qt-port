/* SPDX-License-Identifier: GPL-2.0-only */
/* C11 compiler/allocation services for original Linux ECC read routines. */
#ifndef SEVENZIP_ECC_READ_H
#define SEVENZIP_ECC_READ_H
#include "crypto/modes/pkeycompat.h"
#define DIV_ROUND_UP_POW2(n, d) DIV_ROUND_UP(n, d)
#define kmalloc_obj(obj) kmalloc(sizeof(obj), GFP_KERNEL)
#define pr_err_ratelimited(...) ((void)0)
#define be64_to_cpu(value) get_unaligned_be64(&(value))
/* Select Linux's existing generic carry/borrow path. These services replace
 * compiler overflow intrinsics, including their unsigned-long-long pointer
 * assumption, with the same unsigned 64-bit operations on all host ABIs. */
static inline bool ecc_add_overflow(u64 a, u64 b, u64 *out) {
    return check_add_overflow(a, b, out);
}
static inline bool ecc_sub_overflow(u64 a, u64 b, u64 *out) {
    *out = a - b;
    return a < b;
}
#endif

/* SPDX-License-Identifier: GPL-2.0-only */
/* Owned allocation and preemptible-process services for original Linux MPI. */
#ifndef SEVENZIP_MPI_USERSPACE_H
#define SEVENZIP_MPI_USERSPACE_H
#include "../../../crypto/skciphercompat.h"
#include <linux/count_zeros.h>
typedef unsigned long ulong;
#define kmalloc(size, flags) sevenzip_crypto_zalloc(size)
#define kmalloc_obj(object) sevenzip_crypto_zalloc(sizeof(object))
#define kzalloc_obj(object) sevenzip_crypto_zalloc(sizeof(object))
#define kfree(ptr) sevenzip_crypto_free_sensitive(ptr)
#define printk(...) ((void)0)
#define pr_info(...) ((void)0)
#define BUG_ON(value)                                                                              \
    do {                                                                                           \
        if (value)                                                                                 \
            abort();                                                                               \
    } while (0)
#define cond_resched() ((void)0)
#define swap(a, b)                                                                                 \
    do {                                                                                           \
        mpi_ptr_t temporary = (a);                                                                 \
        (a) = (b);                                                                                 \
        (b) = temporary;                                                                           \
    } while (0)
static inline void *sevenzip_mpi_zalloc_array(size_t size, size_t count) {
    if (size && count > SIZE_MAX / size)
        return NULL;
    return sevenzip_crypto_zalloc(size * count);
}
#define kzalloc_objs(object, count) sevenzip_mpi_zalloc_array(sizeof(object), count)
#endif

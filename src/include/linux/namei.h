/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SEVENZIP_LINUX_NAMEI_H
#define SEVENZIP_LINUX_NAMEI_H
#include <stddef.h>

static inline void nd_terminate_link(void *name, size_t len, size_t maxlen) {
    ((char *)name)[min(len, maxlen)] = '\0';
}
#endif

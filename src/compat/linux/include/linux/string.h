/* SPDX-License-Identifier: GPL-2.0 */
/* Selected Linux string service; libc supplies the standard routines. */
#ifndef SEVENZIP_LINUX_STRING_H
#define SEVENZIP_LINUX_STRING_H
#include <stddef.h>

void *memchr_inv(const void *s, int c, size_t n);

#endif

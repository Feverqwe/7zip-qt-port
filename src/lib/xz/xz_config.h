// SPDX-License-Identifier: 0BSD
// Host services for Linux's unchanged XZ Embedded MicroLZMA decoder.
#ifndef SEVENZIP_XZ_CONFIG_H
#define SEVENZIP_XZ_CONFIG_H
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "../../../third_party/linux/include/linux/xz.h"
#define XZ_DEC_SINGLE
#define XZ_DEC_MICROLZMA
#define min(a, b) ((a) < (b) ? (a) : (b))
#define kmalloc_obj(obj) malloc(sizeof(obj))
#define kfree free
#define vmalloc malloc
#define vfree free
#define memzero(p, n) memset(p, 0, n)
#define memeq(a, b, n) (memcmp(a, b, n) == 0)
#ifndef __always_inline
#ifdef _MSC_VER
#define __always_inline __forceinline
#else
#define __always_inline inline __attribute__((always_inline))
#endif
#endif
#define fallthrough ((void)0)
#endif

// SPDX-License-Identifier: MIT
// Portable types/memory services for the original NTFS3 read decoder.
#ifndef SEVENZIP_NTFS_COMPAT_H
#define SEVENZIP_NTFS_COMPAT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
typedef uint8_t u8;
typedef uint16_t u16;
#ifdef _MSC_VER
typedef intptr_t ssize_t;
#else
#include <sys/types.h>
#endif
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define PtrOffset(base, p) ((size_t)((const u8 *)(p) - (const u8 *)(base)))
ssize_t decompress_lznt(const void *, size_t, void *, size_t);
#endif

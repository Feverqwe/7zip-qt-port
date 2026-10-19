/* SPDX-License-Identifier: GPL-2.0-only */
/* Shared portable ABI for the transferred Linux NLS routines. */
#ifndef SEVENZIP_NLS_TYPES_H
#define SEVENZIP_NLS_TYPES_H
#include <stdint.h>
typedef uint32_t unicode_t;
/* Linux NLS wchar_t is 16-bit, independently of the host C library. */
typedef uint16_t fat_wchar_t;
#define NLS_MAX_CHARSET_SIZE 6
#define MAX_WCHAR_T 0xffff
struct nls_table {
    const char *charset;
    int (*uni2char)(fat_wchar_t, unsigned char *, int);
    int (*char2uni)(const unsigned char *, int, fat_wchar_t *);
    const unsigned char *charset2lower, *charset2upper;
    const char *alias;
};
enum utf16_endian { UTF16_HOST_ENDIAN, UTF16_LITTLE_ENDIAN, UTF16_BIG_ENDIAN };
int utf32_to_utf8(unicode_t, uint8_t *, int);
int utf16s_to_utf8s(const fat_wchar_t *, int, enum utf16_endian, uint8_t *, int);
#endif

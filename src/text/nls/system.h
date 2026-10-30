/* SPDX-License-Identifier: GPL-2.0-only */
/* Platform charset services for the transferred read-only filesystem code. */
#ifndef SEVENZIP_NLS_SYSTEM_H
#define SEVENZIP_NLS_SYSTEM_H
#include "text/nls/types.h"

struct nls_system_charset {
    const char *iconv_name;
    const char *iconv_alias;
    unsigned windows_codepage;
};
int nls_system_available(const struct nls_system_charset *);
int nls_system_encode(const struct nls_system_charset *, fat_wchar_t, unsigned char *, int);
int nls_system_decode(const struct nls_system_charset *, const unsigned char *, int, fat_wchar_t *);
unsigned char fat_cp437_lower(unsigned char);
#endif

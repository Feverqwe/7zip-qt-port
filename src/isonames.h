// SPDX-License-Identifier: GPL-2.0-only
#ifndef SEVENZIP_ISO_NAMES_H
#define SEVENZIP_ISO_NAMES_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
int sevenzip_iso_charset(const char *);
int sevenzip_iso_visible(unsigned char, int, int);
int sevenzip_iso_name(const unsigned char *, size_t, const unsigned char *, size_t, unsigned char,
                      int, char, int, char[1024]);
#ifdef __cplusplus
}
#endif
#endif

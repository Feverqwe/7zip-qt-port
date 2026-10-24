/* SPDX-License-Identifier: MIT */
#ifndef SEVENZIP_CRYPTO_ABI_H
#define SEVENZIP_CRYPTO_ABI_H
/* Native 7-Zip uses /Gr on x86; the library adapters use the C ABI. */
#ifdef _MSC_VER
#define SEVENZIP_CRYPTO_CALL __cdecl
#else
#define SEVENZIP_CRYPTO_CALL
#endif
#endif

/* SPDX-License-Identifier: MIT */
#ifndef SEVENZIP_OPENSSL_HASH_H
#define SEVENZIP_OPENSSL_HASH_H
#include "cryptoabi.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void SEVENZIP_CRYPTO_CALL sevenzip_sha1_blocks(void *state, const unsigned char *data,
                                               size_t blocks);
void SEVENZIP_CRYPTO_CALL sevenzip_sha256_blocks(void *state, const unsigned char *data,
                                                 size_t blocks);
void SEVENZIP_CRYPTO_CALL sevenzip_sha512_blocks(void *state, const unsigned char *data,
                                                 size_t blocks);
void SEVENZIP_CRYPTO_CALL sevenzip_md5_blocks(void *state, const unsigned char *data,
                                              size_t blocks);
#ifdef __cplusplus
}
#endif
#endif

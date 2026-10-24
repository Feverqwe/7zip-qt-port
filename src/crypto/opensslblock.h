/* SPDX-License-Identifier: MIT */
#ifndef SEVENZIP_OPENSSL_BLOCK_H
#define SEVENZIP_OPENSSL_BLOCK_H
#include "cryptoabi.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
void SEVENZIP_CRYPTO_CALL sevenzip_aes_attach(void *storage, void *context);
int SEVENZIP_CRYPTO_CALL sevenzip_aes_blocks(void *storage, const unsigned char *input,
                                             unsigned char *output, size_t blocks);
int SEVENZIP_CRYPTO_CALL sevenzip_aes_failed(const void *storage);
#ifdef __cplusplus
}
#endif
#endif

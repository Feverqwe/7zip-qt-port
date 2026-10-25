/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SEVENZIP_SDK_AES_BLOCKS_H
#define SEVENZIP_SDK_AES_BLOCKS_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void sevenzip_gpl_aes_init(void);
void sevenzip_gpl_aes_prepare(uint32_t *enc, uint32_t *inv, const uint8_t *key, size_t size);
void sevenzip_gpl_aes_block(const uint32_t *keys, unsigned rounds, uint8_t out[16],
                            const uint8_t in[16], int decrypt);
#ifdef __cplusplus
}
#endif
#endif

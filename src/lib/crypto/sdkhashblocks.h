/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef SEVENZIP_SDK_HASH_BLOCKS_H
#define SEVENZIP_SDK_HASH_BLOCKS_H
#include <stddef.h>
#include <stdint.h>

/* Full-block compression only. Linux retains buffering, length and padding. */
void sevenzip_gpl_sha512_blocks(uint64_t state[8], const uint8_t *data, size_t blocks);
void sevenzip_gpl_keccakf(uint64_t state[25]);
#endif

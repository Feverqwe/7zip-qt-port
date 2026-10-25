/* SPDX-License-Identifier: GPL-2.0-only */
/* Adapt Linux's prepared words and block buffers to the existing SDK AES API. */
#include "sdkaesblocks.h"
#include "../../../third_party/7zip/C/Aes.h"
#include "userspace.h"

void sevenzip_gpl_aes_prepare(uint32_t *enc, uint32_t *inv, const uint8_t *key, size_t size) {
    MY_ALIGN(16) UInt32 prepared[AES_NUM_IVMRK_WORDS - 4] = {0};
    MY_ALIGN(16) Byte raw[32];
    const unsigned rounds = 6 + (unsigned)size / 4;
    const size_t bytes = (rounds + 1) * AES_BLOCK_SIZE;
    /* Linux validates the size first. Preserve input overlapping prepared keys. */
    memcpy(raw, key, size);
    sevenzip_gpl_aes_init();
    Aes_SetKey_Enc(prepared, raw, (unsigned)size);
    memcpy(enc, prepared + 4, bytes);
    if (inv) {
        Aes_SetKey_Dec(prepared, raw, (unsigned)size);
        /* Linux stores equivalent-inverse round keys in reverse order. */
        for (unsigned i = 0; i <= rounds; ++i)
            memcpy(inv + 4 * i, prepared + 4 + 4 * (rounds - i), AES_BLOCK_SIZE);
    }
    memzero_explicit(prepared, sizeof(prepared));
    memzero_explicit(raw, sizeof(raw));
}

void sevenzip_gpl_aes_block(const uint32_t *keys, unsigned rounds, uint8_t out[16],
                            const uint8_t in[16], int decrypt) {
    MY_ALIGN(16) UInt32 context[AES_NUM_IVMRK_WORDS] = {0};
    MY_ALIGN(16) Byte block[AES_BLOCK_SIZE];
    sevenzip_gpl_aes_init();
    context[4] = rounds / 2;
    if (decrypt) {
        for (unsigned i = 0; i <= rounds; ++i)
            memcpy(context + 8 + 4 * i, keys + 4 * (rounds - i), AES_BLOCK_SIZE);
    } else {
        memcpy(context + 8, keys, (rounds + 1) * AES_BLOCK_SIZE);
    }
    memcpy(block, in, sizeof(block));
    /* One CBC block with a zero IV is the block operation; Linux owns modes. */
    if (decrypt)
        g_AesCbc_Decode(context, block, 1);
    else
        g_AesCbc_Encode(context, block, 1);
    memcpy(out, block, sizeof(block));
    memzero_explicit(block, sizeof(block));
    memzero_explicit(context, sizeof(context));
}

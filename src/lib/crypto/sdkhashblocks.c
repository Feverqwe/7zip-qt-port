/* SPDX-License-Identifier: GPL-2.0-only */
/* Adapt Linux chaining words to the existing portable 7-Zip hash APIs. */
#include "sdkhashblocks.h"
#include "../../../third_party/7zip/C/Sha3.h"
#include "../../../third_party/7zip/C/Sha512.h"
#include "userspace.h"

void sevenzip_gpl_sha512_blocks(uint64_t state[8], const uint8_t *data, size_t blocks) {
    CSha512 context;
    Sha512_Init(&context, SHA512_DIGEST_SIZE);
    memcpy(context.state, state, sizeof(context.state));
    Sha512_Update(&context, data, blocks * SHA512_BLOCK_SIZE);
    memcpy(state, context.state, sizeof(context.state));
    memzero_explicit(&context, sizeof(context));
}

void sevenzip_gpl_keccakf(uint64_t state[25]) {
    static const Byte zero[SHA3_BLOCK_SIZE_FROM_DIGEST_SIZE(32)] = {0};
    CSha3 context;
    Sha3_Init(&context);
    context.blockSize = (UInt32)sizeof(zero);
    memcpy(context.state, state, sizeof(context.state));
    /* A complete zero rate block leaves the imported lanes unchanged before
       the SDK's permutation. Linux retains domain padding and SHAKE output. */
    Sha3_Update(&context, zero, sizeof(zero));
    memcpy(state, context.state, sizeof(context.state));
    memzero_explicit(&context, sizeof(context));
}

/* SPDX-License-Identifier: MIT */
/* The SDK's copyable hash state and no-failure block interface cannot use an
 * opaque, allocated EVP context. Limit the deprecated API to this adapter;
 * CMake probes it, and unsupported builds retain the upstream primitives.
 * Only full blocks are processed. SDK buffering, padding and finalization stay
 * in the original modules. No OpenSSL algorithm bodies are copied here. */
#include "opensslhash.h"
#ifdef SEVENZIP_OPENSSL_MD5
#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/crypto.h>
#include <openssl/md5.h>
#include <stdint.h>
#include <string.h>

void SEVENZIP_CRYPTO_CALL sevenzip_md5_blocks(void *state, const unsigned char *data,
                                              size_t blocks) {
    if (!blocks)
        return;
    MD5_CTX context;
    uint32_t words[4];
    _Static_assert(sizeof(MD5_LONG) == sizeof(uint32_t), "MD5 state word size");
    MD5_Init(&context);
    memcpy(words, state, sizeof(words));
    context.A = words[0];
    context.B = words[1];
    context.C = words[2];
    context.D = words[3];
    while (blocks) {
        const size_t count = blocks > SIZE_MAX / MD5_CBLOCK ? SIZE_MAX / MD5_CBLOCK : blocks;
        MD5_Update(&context, data, count * MD5_CBLOCK);
        data += count * MD5_CBLOCK;
        blocks -= count;
    }
    words[0] = context.A;
    words[1] = context.B;
    words[2] = context.C;
    words[3] = context.D;
    memcpy(state, words, sizeof(words));
    OPENSSL_cleanse(words, sizeof(words));
    OPENSSL_cleanse(&context, sizeof(context));
}
#endif
#ifdef SEVENZIP_OPENSSL_HASHES
#ifndef OPENSSL_SUPPRESS_DEPRECATED
#define OPENSSL_SUPPRESS_DEPRECATED
#endif
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <string.h>

void SEVENZIP_CRYPTO_CALL sevenzip_sha1_blocks(void *state, const unsigned char *data,
                                               size_t blocks) {
    if (!blocks)
        return;
    SHA_CTX context;
    uint32_t words[5];
    _Static_assert(sizeof(SHA_LONG) == sizeof(uint32_t), "SHA-1 state word size");
    SHA1_Init(&context);
    memcpy(words, state, sizeof(words));
    context.h0 = words[0];
    context.h1 = words[1];
    context.h2 = words[2];
    context.h3 = words[3];
    context.h4 = words[4];
    while (blocks) {
        const size_t count = blocks > SIZE_MAX / SHA_CBLOCK ? SIZE_MAX / SHA_CBLOCK : blocks;
        SHA1_Update(&context, data, count * SHA_CBLOCK);
        data += count * SHA_CBLOCK;
        blocks -= count;
    }
    words[0] = context.h0;
    words[1] = context.h1;
    words[2] = context.h2;
    words[3] = context.h3;
    words[4] = context.h4;
    memcpy(state, words, sizeof(words));
    OPENSSL_cleanse(words, sizeof(words));
    OPENSSL_cleanse(&context, sizeof(context));
}

void SEVENZIP_CRYPTO_CALL sevenzip_sha256_blocks(void *state, const unsigned char *data,
                                                 size_t blocks) {
    if (!blocks)
        return;
    SHA256_CTX context;
    _Static_assert(sizeof(context.h) == 32, "SHA-256 state size");
    SHA256_Init(&context);
    memcpy(context.h, state, sizeof(context.h));
    while (blocks) {
        const size_t count = blocks > SIZE_MAX / SHA256_CBLOCK ? SIZE_MAX / SHA256_CBLOCK : blocks;
        SHA256_Update(&context, data, count * SHA256_CBLOCK);
        data += count * SHA256_CBLOCK;
        blocks -= count;
    }
    memcpy(state, context.h, sizeof(context.h));
    OPENSSL_cleanse(&context, sizeof(context));
}

void SEVENZIP_CRYPTO_CALL sevenzip_sha512_blocks(void *state, const unsigned char *data,
                                                 size_t blocks) {
    if (!blocks)
        return;
    SHA512_CTX context;
    _Static_assert(sizeof(context.h) == 64, "SHA-512 state size");
    SHA512_Init(&context);
    memcpy(context.h, state, sizeof(context.h));
    while (blocks) {
        const size_t count = blocks > SIZE_MAX / SHA512_CBLOCK ? SIZE_MAX / SHA512_CBLOCK : blocks;
        SHA512_Update(&context, data, count * SHA512_CBLOCK);
        data += count * SHA512_CBLOCK;
        blocks -= count;
    }
    memcpy(state, context.h, sizeof(context.h));
    OPENSSL_cleanse(&context, sizeof(context));
}
#endif

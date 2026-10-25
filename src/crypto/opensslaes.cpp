// SPDX-License-Identifier: MIT
#include "opensslaes.h"
#include <algorithm>
#include <cstring>

#ifdef SEVENZIP_OPENSSL_CRYPTO
#include <memory>
#include <openssl/evp.h>
#endif

SevenzipOpenSslAesCbc::~SevenzipOpenSslAesCbc() { clear(); }

void SevenzipOpenSslAesCbc::clear() {
#ifdef SEVENZIP_OPENSSL_CRYPTO
    EVP_CIPHER_CTX_free(static_cast<EVP_CIPHER_CTX *>(context_));
#endif
    context_ = nullptr;
}

bool SevenzipOpenSslAesCbc::prepare(const unsigned char *key, unsigned key_size, bool encrypt) {
    clear();
#ifdef SEVENZIP_OPENSSL_CRYPTO
    const char *name;
    switch (key_size) {
    case 16:
        name = "AES-128-CBC";
        break;
    case 24:
        name = "AES-192-CBC";
        break;
    case 32:
        name = "AES-256-CBC";
        break;
    default:
        return false;
    }
    if (!key)
        return false;
    std::unique_ptr<EVP_CIPHER, decltype(&EVP_CIPHER_free)> cipher(
        EVP_CIPHER_fetch(nullptr, name, nullptr), EVP_CIPHER_free);
    if (!cipher)
        return false;
    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> context(EVP_CIPHER_CTX_new(),
                                                                            EVP_CIPHER_CTX_free);
    const unsigned char zero_iv[16] = {};
    if (!context ||
        EVP_CipherInit_ex2(context.get(), cipher.get(), key, zero_iv, encrypt ? 1 : 0, nullptr) !=
            1 ||
        EVP_CIPHER_CTX_set_padding(context.get(), 0) != 1)
        return false;
    context_ = context.release();
    return true;
#else
    (void)key;
    (void)key_size;
    (void)encrypt;
    return false;
#endif
}

std::size_t SevenzipOpenSslAesCbc::crypt(unsigned char *data, std::size_t size,
                                         const unsigned char iv[16], unsigned char next_iv[16]) {
#ifdef SEVENZIP_OPENSSL_CRYPTO
    if (!context_ || !size || size % 16 || !data || !iv || !next_iv)
        return 0;
    auto *context = static_cast<EVP_CIPHER_CTX *>(context_);
    if (EVP_CipherInit_ex2(context, nullptr, nullptr, iv, -1, nullptr) != 1) {
        clear();
        return 0;
    }
    // Stage a bounded chunk: an EVP error must never publish partially
    // transformed bytes as successfully filtered (ICompressFilter has no error result).
    constexpr std::size_t batch_size = 4096;
    unsigned char output[batch_size + 16];
    unsigned char updated_iv[16];
    std::size_t done = 0;
    while (done < size) {
        const auto count = static_cast<int>(std::min(batch_size, size - done));
        int produced = 0;
        if (EVP_CipherUpdate(context, output, &produced, data + done, count) != 1 ||
            produced != count ||
            EVP_CIPHER_CTX_get_updated_iv(context, updated_iv, sizeof(updated_iv)) != 1) {
            clear();
            break;
        }
        std::memcpy(data + done, output, static_cast<std::size_t>(produced));
        std::memcpy(next_iv, updated_iv, sizeof(updated_iv));
        done += static_cast<std::size_t>(produced);
    }
    OPENSSL_cleanse(output, sizeof(output));
    OPENSSL_cleanse(updated_iv, sizeof(updated_iv));
    return done;
#else
    (void)data;
    (void)size;
    (void)iv;
    (void)next_iv;
    return 0;
#endif
}

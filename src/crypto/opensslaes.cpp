// SPDX-License-Identifier: MIT
#include "opensslaes.h"
#include "opensslblock.h"
#include <algorithm>
#include <climits>
#include <cstring>

#include <memory>
#include <openssl/evp.h>

SevenzipOpenSslAes::~SevenzipOpenSslAes() { clear(); }

void SevenzipOpenSslAes::clear() {
    EVP_CIPHER_CTX_free(static_cast<EVP_CIPHER_CTX *>(context_));
    context_ = nullptr;
}

bool SevenzipOpenSslAes::prepare(const unsigned char *key, unsigned key_size, bool encrypt,
                                 void *counter_state) {
    clear();
    const char *name;
    switch (key_size) {
    case 16:
        name = counter_state ? "AES-128-ECB" : "AES-128-CBC";
        break;
    case 24:
        name = counter_state ? "AES-192-ECB" : "AES-192-CBC";
        break;
    case 32:
        name = counter_state ? "AES-256-ECB" : "AES-256-CBC";
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
    if (counter_state)
        sevenzip_aes_attach(counter_state, context_);
    return true;
}

bool SevenzipOpenSslAes::crypt(unsigned char *data, std::size_t size, const unsigned char iv[16],
                               unsigned char next_iv[16]) {
    if (!context_ || size % 16 || !data || !iv || !next_iv)
        return false;
    auto *context = static_cast<EVP_CIPHER_CTX *>(context_);
    if (EVP_CipherInit_ex2(context, nullptr, nullptr, iv, -1, nullptr) != 1) {
        clear();
        return false;
    }
    constexpr std::size_t batch_size = INT_MAX & ~std::size_t(15);
    std::size_t done = 0;
    while (done < size) {
        const auto count = static_cast<int>(std::min(batch_size, size - done));
        int produced = 0;
        if (EVP_CipherUpdate(context, data + done, &produced, data + done, count) != 1 ||
            produced != count) {
            clear();
            return false;
        }
        done += static_cast<std::size_t>(produced);
    }
    if (EVP_CIPHER_CTX_get_updated_iv(context, next_iv, 16) != 1) {
        clear();
        return false;
    }
    return true;
}

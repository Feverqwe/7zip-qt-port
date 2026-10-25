// SPDX-License-Identifier: MIT
#ifndef SEVENZIP_OPENSSL_AES_H
#define SEVENZIP_OPENSSL_AES_H

#include <cstddef>

// A failed library operation leaves the unprocessed suffix and its input IV
// available to the original cipher. No OpenSSL types cross this boundary.
class SevenzipOpenSslAesCbc {
    void *context_ = nullptr;
    void clear();

  public:
    SevenzipOpenSslAesCbc() = default;
    ~SevenzipOpenSslAesCbc();
    SevenzipOpenSslAesCbc(const SevenzipOpenSslAesCbc &) = delete;
    SevenzipOpenSslAesCbc &operator=(const SevenzipOpenSslAesCbc &) = delete;
    bool prepare(const unsigned char *key, unsigned key_size, bool encrypt);
    std::size_t crypt(unsigned char *data, std::size_t size, const unsigned char iv[16],
                      unsigned char next_iv[16]);
};

#endif

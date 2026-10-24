// SPDX-License-Identifier: MIT
#ifndef SEVENZIP_OPENSSL_AES_H
#define SEVENZIP_OPENSSL_AES_H

#include <cstddef>

// No OpenSSL types cross this boundary. Failure aborts the enclosing filter;
// callers must not publish the current output buffer after a failed call.
class SevenzipOpenSslAes {
    void *context_ = nullptr;
    void clear();

  public:
    SevenzipOpenSslAes() = default;
    ~SevenzipOpenSslAes();
    SevenzipOpenSslAes(const SevenzipOpenSslAes &) = delete;
    SevenzipOpenSslAes &operator=(const SevenzipOpenSslAes &) = delete;
    bool prepare(const unsigned char *key, unsigned key_size, bool encrypt,
                 void *counter_state = nullptr);
    bool crypt(unsigned char *data, std::size_t size, const unsigned char iv[16],
               unsigned char next_iv[16]);
};

#endif

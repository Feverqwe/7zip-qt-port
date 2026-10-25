// SPDX-License-Identifier: MIT
#ifndef SEVENZIP_OPENSSL_KDF_H
#define SEVENZIP_OPENSSL_KDF_H
#include "cryptoabi.h"

#include <cstddef>
#include <cstdint>

// False asks the caller to use its original provider, including when a system
// OpenSSL provider does not support the legacy archive parameters.
bool SEVENZIP_CRYPTO_CALL sevenzip_try_pbkdf2_sha1(const unsigned char *password,
                                                   std::size_t password_size,
                                                   const unsigned char *salt, std::size_t salt_size,
                                                   std::uint32_t iterations, unsigned char *key,
                                                   std::size_t key_size);

#endif

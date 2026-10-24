// SPDX-License-Identifier: MIT
#include "opensslkdf.h"

#ifdef SEVENZIP_OPENSSL_CRYPTO
#include <memory>
#include <openssl/core_names.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#endif

bool SEVENZIP_CRYPTO_CALL sevenzip_try_pbkdf2_sha1(const unsigned char *password,
                                                   std::size_t password_size,
                                                   const unsigned char *salt, std::size_t salt_size,
                                                   std::uint32_t iterations, unsigned char *key,
                                                   std::size_t key_size) {
#ifdef SEVENZIP_OPENSSL_CRYPTO
    if (!key_size)
        return true;
    if (!iterations || (!password && password_size) || (!salt && salt_size) || !key)
        return false;
    std::unique_ptr<EVP_KDF, decltype(&EVP_KDF_free)> algorithm(
        EVP_KDF_fetch(nullptr, "PBKDF2", nullptr), EVP_KDF_free);
    if (!algorithm)
        return false;
    std::unique_ptr<EVP_KDF_CTX, decltype(&EVP_KDF_CTX_free)> context(
        EVP_KDF_CTX_new(algorithm.get()), EVP_KDF_CTX_free);
    if (!context)
        return false;
    std::uint64_t count = iterations;
    int pkcs5 = 1; // Legacy ZIP uses short salts and passwords, with a fixed iteration count.
    char digest[] = "SHA1";
    OSSL_PARAM parameters[] = {
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_PASSWORD,
                                          const_cast<unsigned char *>(password), password_size),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, const_cast<unsigned char *>(salt),
                                          salt_size),
        OSSL_PARAM_construct_uint64(OSSL_KDF_PARAM_ITER, &count),
        OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, digest, 0),
        OSSL_PARAM_construct_int(OSSL_KDF_PARAM_PKCS5, &pkcs5),
        OSSL_PARAM_construct_end()};
    return EVP_KDF_derive(context.get(), key, key_size, parameters) == 1;
#else
    (void)password;
    (void)password_size;
    (void)salt;
    (void)salt_size;
    (void)iterations;
    (void)key;
    (void)key_size;
    return false;
#endif
}

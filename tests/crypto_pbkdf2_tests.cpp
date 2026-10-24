// SPDX-License-Identifier: MIT
// Expected bytes generated independently with Python hashlib.pbkdf2_hmac.
#include "7zip/Crypto/Pbkdf2HmacSha1.h"
#include "crypto/opensslkdf.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#ifdef SEVENZIP_TEST_OPENSSL
#include <openssl/evp.h>
#endif

namespace {
std::string hex(const std::vector<Byte> &bytes) {
    const char digits[] = "0123456789abcdef";
    std::string result;
    for (Byte value : bytes) {
        result += digits[value >> 4];
        result += digits[value & 15];
    }
    return result;
}

bool check(const std::string &password, const std::string &salt, UInt32 iterations,
           const char *expected, bool require_provider) {
    const auto *p = reinterpret_cast<const Byte *>(password.data());
    const auto *s = reinterpret_cast<const Byte *>(salt.data());
    std::vector<Byte> result(std::char_traits<char>::length(expected) / 2, 0xa5);
    NCrypto::NSha1::Pbkdf2Hmac(p, password.size(), s, salt.size(), iterations, result.data(),
                               result.size());
    if (hex(result) != expected) {
        std::fputs("PBKDF2 produced incorrect key bytes\n", stderr);
        return false;
    }
    if (require_provider && (!sevenzip_try_pbkdf2_sha1(p, password.size(), s, salt.size(),
                                                       iterations, result.data(), result.size()) ||
                             hex(result) != expected)) {
        std::fputs("Library provider could not derive a supported legacy key\n", stderr);
        return false;
    }
    return true;
}

bool vectors(bool require_provider) {
    // Partial final blocks, embedded NUL bytes and empty secrets must survive
    // a provider change; string APIs and provider strength gates can break them.
    std::string long_password(100, '\0');
    std::string long_salt(40, '\0');
    for (std::size_t i = 0; i < long_password.size(); ++i)
        long_password[i] = static_cast<char>(i);
    for (std::size_t i = 0; i < long_salt.size(); ++i)
        long_salt[i] = static_cast<char>(i);
    return check("password", "salt", 1, "0c60c80f961f0e71f3a9b524af6012062fe037a6",
                 require_provider) &&
           check("password", "salt", 4096, "4b007901b765489abead49d926f721d065a429c12e463f6c4c",
                 require_provider) &&
           check(std::string("pass\0word", 9), std::string("sa\0lt", 5), 4096,
                 "56fa6aa75548099dcc37d7f03425e0c3", require_provider) &&
           check("", "", 1000,
                 "6e40910ac02ec89cebb9d898b13a09d1cd7adf6f8cc08cc473302c8973aa2e19a383ca"
                 "dd85471d4ebd5efcf34307f89b006ed5293174b4c612fa9ffe2a1b455a5c49",
                 require_provider) &&
           check(long_password, long_salt, 1000,
                 "1d3333f21c3519a30aa4b8d6e81da16f062f926a0231b71f3f220414e476eeb151a1"
                 "888f9f14902968f0b79751adae86fa16a988b73619d15f742ff824097a50b692",
                 require_provider);
}
} // namespace

int main() {
#ifdef SEVENZIP_TEST_OPENSSL
    if (!vectors(true))
        return 1;
    // Model an unavailable algorithm in a system provider configuration. The
    // archive consumer must still produce the complete original key.
    if (EVP_set_default_properties(nullptr, "provider=sevenzip_missing") != 1)
        return 1;
    const bool fallback_ok = vectors(false);
    if (EVP_set_default_properties(nullptr, nullptr) != 1 || !fallback_ok)
        return 1;
#else
    if (!vectors(false))
        return 1;
#endif
    return 0;
}

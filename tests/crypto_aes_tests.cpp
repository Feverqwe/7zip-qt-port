#include "CPP/7zip/Crypto/MyAes.h"
#include "crypto/opensslaes.h"
#include <array>
#include <cstdio>

#ifdef SEVENZIP_TEST_OPENSSL
#include <openssl/evp.h>
#endif

namespace {
using Bytes = std::array<Byte, 8240>;

bool exercise(unsigned key_size, bool require_provider) {
    Byte key[32], replacement[32], iv[16];
    for (unsigned i = 0; i < 32; ++i) {
        key[i] = Byte(i * 7);
        replacement[i] = Byte(255 - i * 3);
    }
    for (unsigned i = 0; i < 16; ++i)
        iv[i] = Byte(i * 13);
    alignas(64) Bytes input, expected, actual;
    for (std::size_t i = 0; i < input.size(); ++i)
        input[i] = Byte(i * 31 + i / 16);
    expected = actual = input;
    CMyComPtr2_Create<ICompressFilter, NCrypto::CAesCbcEncoder> upstream;
    CMyComPtr2_Create<ICompressFilter, NCrypto::CAesCbcEncoder> selected;
    if (!upstream->SetFunctions(1) || !selected->SetFunctions(0))
        return false;
    // IV-before-key is allowed by the source contract.
    if (upstream->SetInitVector(iv, sizeof(iv)) != S_OK ||
        selected->SetInitVector(iv, sizeof(iv)) != S_OK ||
        upstream->SetKey(key, key_size) != S_OK || selected->SetKey(key, key_size) != S_OK ||
        upstream->Init() != S_OK || selected->Init() != S_OK)
        return false;
    if (selected->Filter(actual.data(), 0) != 0 || selected->Filter(actual.data(), 15) != 16 ||
        actual != input)
        return false;
    if (upstream->Filter(expected.data(), UInt32(expected.size())) != expected.size())
        return false;
    // Multiple calls, staging boundaries and a non-full final input request.
    if (selected->Filter(actual.data(), 17) != 16 || actual[16] != input[16] ||
        selected->Filter(actual.data() + 16, 4096) != 4096 ||
        selected->Filter(actual.data() + 4112, UInt32(actual.size() - 4112)) !=
            actual.size() - 4112 ||
        actual != expected)
        return false;

    if (require_provider) {
        SevenzipOpenSslAesCbc cipher;
        Byte next_iv[16];
        actual = input;
        if (!cipher.prepare(key, key_size, true) ||
            cipher.crypt(actual.data(), actual.size(), iv, next_iv) != actual.size() ||
            actual != expected)
            return false;
        if (!cipher.prepare(key, key_size, false) ||
            cipher.crypt(actual.data(), actual.size(), iv, next_iv) != actual.size() ||
            actual != input)
            return false;
    }

    actual = expected;
    CMyComPtr2_Create<ICompressFilter, NCrypto::CAesCbcDecoder> decoder;
    if (decoder->SetKey(key, key_size) != S_OK || decoder->SetInitVector(iv, sizeof(iv)) != S_OK ||
        decoder->Init() != S_OK ||
        decoder->Filter(actual.data(), UInt32(actual.size())) != actual.size() || actual != input)
        return false;

    // Reset, rekey without resetting CBC chaining, and changes of the source
    // implementation selector must share the same IV at every boundary.
    expected = actual = input;
    if (upstream->Init() != S_OK || selected->Init() != S_OK)
        return false;
    for (std::size_t offset = 0; offset < actual.size(); offset += 16) {
        if (offset == 16 && !selected->SetFunctions(1))
            return false;
        if (offset == 48 && !selected->SetFunctions(0))
            return false;
        if (offset == 96 && (upstream->SetKey(replacement, key_size) != S_OK ||
                             selected->SetKey(replacement, key_size) != S_OK))
            return false;
        if (upstream->Filter(expected.data() + offset, 16) != 16 ||
            selected->Filter(actual.data() + offset, 16) != 16)
            return false;
    }
    return actual == expected;
}

bool all_keys(bool require_provider) {
    for (unsigned size : {16U, 24U, 32U})
        if (!exercise(size, require_provider)) {
            std::fprintf(stderr, "AES-CBC behavior differs for a %u-byte key\n", size);
            return false;
        }
    return true;
}
} // namespace

int main() {
#ifdef SEVENZIP_TEST_OPENSSL
    if (!all_keys(true))
        return 1;
    if (EVP_set_default_properties(nullptr, "provider=sevenzip_missing") != 1)
        return 1;
    const bool fallback_ok = all_keys(false);
    if (EVP_set_default_properties(nullptr, nullptr) != 1 || !fallback_ok)
        return 1;
#else
    if (!all_keys(false))
        return 1;
#endif
    return 0;
}

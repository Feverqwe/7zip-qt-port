#include "7zip/Crypto/MyAes.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <vector>

namespace {
std::vector<Byte> bytes(const char *hex) {
    std::vector<Byte> result;
    for (; *hex; hex += 2)
        result.push_back(Byte(std::stoul(std::string(hex, 2), nullptr, 16)));
    return result;
}

// NIST SP 800-38A CBC vectors, also checked using the independent system CLI.
bool cbc(const char *key_hex, const char *expected_hex) {
    auto key = bytes(key_hex);
    auto iv = bytes("000102030405060708090a0b0c0d0e0f");
    auto input = bytes("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51"
                       "30c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710");
    auto expected = bytes(expected_hex), actual = input;
    CMyComPtr2_Create<ICompressFilter, NCrypto::CAesCbcEncoder> encoder;
    if (!encoder->SetFunctions(0) || encoder->SetInitVector(iv.data(), 16) != S_OK ||
        encoder->SetKey(key.data(), UInt32(key.size())) != S_OK || encoder->Init() != S_OK)
        return false;
    if (encoder->Filter(actual.data(), 0) != 0 || encoder->Filter(actual.data(), 15) != 16 ||
        actual != input || encoder->Filter(actual.data(), 17) != 16 ||
        encoder->Filter(actual.data() + 16, 48) != 48 || actual != expected)
        return false;
    // Init resets the IV without re-deriving a key.
    actual = input;
    if (encoder->Init() != S_OK || encoder->Filter(actual.data(), 64) != 64 || actual != expected)
        return false;
    CMyComPtr2_Create<ICompressFilter, NCrypto::CAesCbcDecoder> decoder;
    if (decoder->SetKey(key.data(), UInt32(key.size())) != S_OK ||
        decoder->SetInitVector(iv.data(), 16) != S_OK || decoder->Init() != S_OK ||
        decoder->Filter(actual.data(), 32) != 32 || decoder->Filter(actual.data() + 32, 32) != 32 ||
        actual != input)
        return false;
    // Rekeying keeps the current CBC chain, including across larger requests.
    std::reverse(key.begin(), key.end());
    CMyComPtr2_Create<ICompressFilter, NCrypto::CAesCbcEncoder> fresh;
    if (encoder->SetKey(key.data(), UInt32(key.size())) != S_OK ||
        fresh->SetKey(key.data(), UInt32(key.size())) != S_OK ||
        fresh->SetInitVector(expected.data() + expected.size() - 16, 16) != S_OK ||
        fresh->Init() != S_OK)
        return false;
    actual.resize(8240);
    for (std::size_t i = 0; i < actual.size(); ++i)
        actual[i] = input[i % input.size()];
    const auto large_input = actual;
    expected = actual;
    if (fresh->Filter(expected.data(), UInt32(expected.size())) != expected.size() ||
        encoder->Filter(actual.data(), 17) != 16 || actual[16] != large_input[16] ||
        encoder->Filter(actual.data() + 16, 4096) != 4096 ||
        encoder->Filter(actual.data() + 4112, UInt32(actual.size() - 4112)) !=
            actual.size() - 4112 ||
        actual != expected)
        return false;
    return decoder->SetKey(key.data(), UInt32(key.size())) == S_OK &&
           decoder->Filter(actual.data(), UInt32(actual.size())) == actual.size() &&
           actual == large_input;
}

bool counter_wrap_and_partial_calls() {
    auto key = bytes("2b7e151628aed2a6abf7158809cf4f3c");
    auto iv = bytes("feffffffffffffff1122334455667788");
    // AES-ECB reference over the four little-endian pre-incremented counters.
    auto expected = bytes("2b8995f4ccb7e1751baf6cc8b635e45b1c03da48b2bc996417806546c0417980"
                          "e9265860aa7347551875287fa18f1968e26da2425452cf84791cb98a580ff132");
    alignas(64) std::array<Byte, 64> actual{};
    CMyComPtr2_Create<ICompressFilter, NCrypto::CAesCtrCoder> coder;
    if (coder->SetInitVector(iv.data(), 16) != S_OK || coder->SetKey(key.data(), 16) != S_OK ||
        coder->Init() != S_OK)
        return false;
    std::size_t offset = 0;
    for (UInt32 request : {1U, 15U, 17U, 31U}) {
        while (request) {
            const UInt32 done = coder.Interface()->Filter(actual.data() + offset, request);
            if (!done || done > request)
                return false;
            request -= done;
            offset += done;
        }
    }
    return std::equal(actual.begin(), actual.end(), expected.begin());
}

} // namespace

int main() {
    if (!cbc("2b7e151628aed2a6abf7158809cf4f3c",
             "7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b273"
             "bed6b8e3c1743b7116e69e222295163ff1caa1681fac09120eca307586e1a7") ||
        !cbc("8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b",
             "4f021db243bc633d7178183a9fa071e8b4d9ada9ad7dedf4e5e738763f69145a5"
             "71b242012fb7ae07fa9baac3df102e008b0e27988598881d920a9e64f5615cd") ||
        !cbc("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
             "f58c4c04d6e5f1ba779eabfb5f7bfbd69cfc4e967edb808d679f777bc6702c7d"
             "39f23369a9d9bacfa530e26304231461b2eb05e2c39be9fcda6c19078c6a9d1b") ||
        !counter_wrap_and_partial_calls()) {
        std::fputs("AES archive-mode, streaming contract differs\n", stderr);
        return 1;
    }
    return 0;
}

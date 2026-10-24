// Expected abc digests independently computed with OpenSSL's reference CLI.
#include "7zip/Crypto/Pbkdf2HmacSha1.h"
#include "Md5.h"
#include "Sha1.h"
#include "Sha256.h"
#include "Sha512.h"
#include <cstdio>
#include <string>
#include <vector>

#ifdef SEVENZIP_TEST_OPENSSL
#include <openssl/evp.h>
#endif

namespace {
template <class Context, auto Init, auto Update, auto Final, auto Select> struct Hash {
    Context State;
    unsigned Size;
    Hash(unsigned size, unsigned algorithm) : Size(size) {
        Init(&State);
        Select(&State, algorithm);
    }
    void update(const Byte *p, std::size_t n) { Update(&State, p, n); }
    std::vector<Byte> final() {
        std::vector<Byte> bytes(Size);
        Final(&State, bytes.data());
        return bytes;
    }
    bool select(unsigned algorithm) { return Select(&State, algorithm) != 0; }
};
using Hash1 = Hash<CSha1, Sha1_Init, Sha1_Update, Sha1_Final, Sha1_SetFunction>;
using Hash256 = Hash<CSha256, Sha256_Init, Sha256_Update, Sha256_Final, Sha256_SetFunction>;
struct Hash512 {
    CSha512 State;
    unsigned Size;
    Hash512(unsigned size, unsigned algorithm) : Size(size) {
        Sha512_Init(&State, Size);
        Sha512_SetFunction(&State, algorithm);
    }
    void update(const Byte *p, std::size_t n) { Sha512_Update(&State, p, n); }
    std::vector<Byte> final() {
        std::vector<Byte> bytes(Size);
        Sha512_Final(&State, bytes.data(), Size);
        return bytes;
    }
    bool select(unsigned algorithm) { return Sha512_SetFunction(&State, algorithm) != 0; }
};

std::string hex(const std::vector<Byte> &bytes) {
    const char digits[] = "0123456789abcdef";
    std::string result;
    for (Byte b : bytes) {
        result += digits[b >> 4];
        result += digits[b & 15];
    }
    return result;
}

template <class H> bool exercise(unsigned size, const char *abc_digest) {
    std::vector<Byte> payload(8193);
    for (std::size_t i = 0; i < payload.size(); ++i)
        payload[i] = Byte(i * 7 + i / 13);
    // Padding boundaries, aligned blocks and a large unaligned request.
    for (std::size_t length : {0U, 1U, 55U, 56U, 63U, 64U, 111U, 112U, 127U, 128U, 8193U}) {
        H selected(size, 0), source(size, 1);
        selected.update(nullptr, 0);
        source.update(payload.data(), length);
        const std::size_t first = length > 13 ? 13 : length;
        selected.update(payload.data(), first);
        selected.update(payload.data() + first, length - first);
        if (selected.final() != source.final())
            return false;
        // Final() resets the upstream state while preserving its selected implementation.
        selected.update(reinterpret_cast<const Byte *>("abc"), 3);
        if (hex(selected.final()) != abc_digest)
            return false;
    }
    H left(size, 0);
    left.update(payload.data(), 77);
    H right = left; // HMAC and archive derivation rely on copyable partial state.
    if (!left.select(1))
        return false;
    left.update(payload.data() + 77, 311);
    if (!left.select(0))
        return false;
    left.update(payload.data() + 388, 12);
    right.update(payload.data() + 77, 140);
    H reference_left(size, 1), reference_right(size, 1);
    reference_left.update(payload.data(), 400);
    reference_right.update(payload.data(), 217);
    return left.final() == reference_left.final() && right.final() == reference_right.final();
}

bool md5_streaming_and_clone() {
    std::vector<Byte> payload(8193);
    for (std::size_t i = 0; i < payload.size(); ++i)
        payload[i] = Byte(i * 7 + i / 13);
    CMd5 state;
    Md5_Init(&state);
    Md5_Update(&state, payload.data(), 13);
    Md5_Update(&state, payload.data() + 13, payload.size() - 13);
    std::vector<Byte> digest(16);
    Md5_Final(&state, digest.data());
    if (hex(digest) != "99425f91efe5ca58011617efdfed82c3")
        return false;
    Md5_Update(&state, nullptr, 0);
    Md5_Final(&state, digest.data());
    if (hex(digest) != "d41d8cd98f00b204e9800998ecf8427e")
        return false;
    Md5_Update(&state, payload.data(), 77);
    CMd5 branch = state;
    Md5_Update(&state, payload.data() + 77, 323);
    Md5_Update(&branch, payload.data() + 77, 140);
    Md5_Final(&state, digest.data());
    if (hex(digest) != "4ad7359f321cbdc64ceae8d9344f427d")
        return false;
    Md5_Final(&branch, digest.data());
    return hex(digest) == "b44f8dd2846d9684f0a5a680ee52d173";
}

bool original_kdf_with_library_blocks() {
    std::vector<Byte> key(25);
    NCrypto::NSha1::Pbkdf2Hmac(reinterpret_cast<const Byte *>("password"), 8,
                               reinterpret_cast<const Byte *>("salt"), 4, 4096, key.data(),
                               key.size());
    return hex(key) == "4b007901b765489abead49d926f721d065a429c12e463f6c4c";
}
} // namespace

int main() {
    Sha1Prepare();
    Sha256Prepare();
    Sha512Prepare();
    if (!md5_streaming_and_clone() ||
        !exercise<Hash1>(20, "a9993e364706816aba3e25717850c26c9cd0d89d") ||
        !exercise<Hash256>(32,
                           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") ||
        !exercise<Hash512>(28, "4634270f707b6a54daae7530460842e20e37ed265ceee9a43e8924aa") ||
        !exercise<Hash512>(32,
                           "53048e2681941ef99b2e29b76b4c7dabe4c2d0c634fc6d46e0e2f13107e7af23") ||
        !exercise<Hash512>(48, "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8"
                               "086072ba1e7cc2358baeca134c825a7") ||
        !exercise<Hash512>(64, "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2"
                               "192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f")) {
        std::fputs("SHA state, streaming or finalization behavior differs\n", stderr);
        return 1;
    }
#ifdef SEVENZIP_TEST_OPENSSL
    // Disable EVP KDF lookup so the original HMAC copy/GetBlockDigest loop
    // exercises the state-compatible library block adapter as a real consumer.
    if (EVP_set_default_properties(nullptr, "provider=sevenzip_missing") != 1)
        return 1;
    const bool kdf_ok = original_kdf_with_library_blocks();
    if (EVP_set_default_properties(nullptr, nullptr) != 1 || !kdf_ok)
        return 1;
#else
    if (!original_kdf_with_library_blocks())
        return 1;
#endif
    return 0;
}

// Pbkdf2HmacSha1.cpp

#include "StdAfx.h"

#include "../../Common/MyException.h"
#include "Pbkdf2HmacSha1.h"
#include "crypto/opensslkdf.h"

namespace NCrypto {
namespace NSha1 {
void Pbkdf2Hmac(const Byte *pwd, size_t pwdSize, const Byte *salt, size_t saltSize,
                UInt32 numIterations, Byte *key, size_t keySize) {
    if (!sevenzip_pbkdf2_sha1(pwd, pwdSize, salt, saltSize, numIterations, key, keySize))
        throw CSystemException(E_FAIL);
}
} // namespace NSha1
} // namespace NCrypto

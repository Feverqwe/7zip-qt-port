// Crypto/MyAes.cpp

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "../../Common/MyException.h"
#include "MyAes.h"
#include "crypto/opensslblock.h"

namespace NCrypto {

CAesCoder::CAesCoder(unsigned keySize, bool encodeMode, bool ctrMode)
    : _keyIsSet(false), _encodeMode(encodeMode), _ctrMode(ctrMode), _keySize(keySize),
      _aes(AES_NUM_IVMRK_WORDS * 4 + AES_BLOCK_SIZE * 2) {
    memset(_iv, 0, AES_BLOCK_SIZE);
}

Z7_COM7F_IMF(CAesCoder::Init()) {
    _ctrPos = 0;
    AesCbc_Init(Aes(), _iv);
    return _keyIsSet ? S_OK : E_NOTIMPL; // E_FAIL
}

void CAesCoder::Code(Byte *data, size_t numBlocks) {
    Byte iv[AES_BLOCK_SIZE], nextIv[AES_BLOCK_SIZE];
    for (unsigned i = 0; i < 4; ++i) {
        SetUi32(iv + i * 4, Aes()[i])
    }
    if (!_opensslCipher.crypt(data, numBlocks * AES_BLOCK_SIZE, iv, nextIv)) {
        _keyIsSet = false;
        throw CSystemException(E_FAIL);
    }
    for (unsigned i = 0; i < 4; ++i)
        Aes()[i] = GetUi32(nextIv + i * 4);
}

void CAesCoder::CodeCtr(Byte *data, size_t numBlocks) {
    AesCtr_Code(Aes(), data, numBlocks);
    if (sevenzip_aes_failed(Aes() + 4)) {
        _keyIsSet = false;
        throw CSystemException(E_FAIL);
    }
}

Z7_COM7F_IMF2(UInt32, CAesCoder::Filter(Byte *data, UInt32 size)) {
    if (!_keyIsSet)
        return 0;
    if (size < AES_BLOCK_SIZE) {
        if (size == 0)
            return 0;
        return AES_BLOCK_SIZE;
    }
    size >>= 4;
    // (data) must be aligned for 16-bytes here
    Code(data, size);
    return size << 4;
}

Z7_COM7F_IMF(CAesCoder::SetKey(const Byte *data, UInt32 size)) {
    if (!data || (size & 0x7) != 0 || size < 16 || size > 32)
        return E_INVALIDARG;
    if (_keySize != 0 && size != _keySize)
        return E_INVALIDARG;
    _keyIsSet = false;
    if (!_opensslCipher.prepare(data, size, _encodeMode, _ctrMode ? Aes() + 4 : nullptr))
        return E_FAIL;
    _keyIsSet = true;
    return S_OK;
}

Z7_COM7F_IMF(CAesCoder::SetInitVector(const Byte *data, UInt32 size)) {
    if (size != AES_BLOCK_SIZE)
        return E_INVALIDARG;
    memcpy(_iv, data, size);
    /* we allow SetInitVector() call before SetKey() call.
       so we ignore possible error in Init() here */
    CAesCoder::Init(); // don't call virtual function here !!!
    return S_OK;
}

#ifndef Z7_SFX

Z7_COM7F_IMF2(UInt32, CAesCtrCoder::Filter(Byte *data, UInt32 size)) {
    if (!_keyIsSet)
        return 0;
    if (size == 0)
        return 0;

    if (_ctrPos != 0) {
        /* Optimized caller will not call here */
        const Byte *ctr = (Byte *)(Aes() + AES_NUM_IVMRK_WORDS);
        unsigned num = 0;
        for (unsigned i = _ctrPos; i != AES_BLOCK_SIZE; i++) {
            if (num == size) {
                _ctrPos = i;
                return num;
            }
            data[num++] ^= ctr[i];
        }
        _ctrPos = 0;
        return num;
    }

    if (size < AES_BLOCK_SIZE) {
        /* The good optimized caller can call here only in last Filter() call.
           But we support also non-optimized callers,
           where another Filter() calls are allowed after this call.
        */
        Byte *ctr = (Byte *)(Aes() + AES_NUM_IVMRK_WORDS);
        memset(ctr, 0, AES_BLOCK_SIZE);
        memcpy(ctr, data, size);
        CodeCtr(ctr, 1);
        memcpy(data, ctr, size);
        _ctrPos = size;
        return size;
    }

    size >>= 4;
    // (data) must be aligned for 16-bytes here
    CodeCtr(data, size);
    return size << 4;
}

#endif // Z7_SFX


#ifndef Z7_EXTRACT_ONLY

// OpenSSL owns CPU dispatch. Explicit 7-Zip SW/HW implementations no longer exist.
#ifndef Z7_SFX
bool CAesCtrCoder::SetFunctions(UInt32 algo) { return algo == 0; }
#endif
bool CAesCbcEncoder::SetFunctions(UInt32 algo) { return algo == 0; }
bool CAesCbcDecoder::SetFunctions(UInt32 algo) { return algo == 0; }

Z7_COM7F_IMF(CAesCoder::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *coderProps, UInt32 numProps))
{
  UInt32 algo = 0;
  for (UInt32 i = 0; i < numProps; i++)
  {
    if (propIDs[i] == NCoderPropID::kDefaultProp)
    {
      const PROPVARIANT &prop = coderProps[i];
      if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      if (prop.ulVal > 3)
        return E_NOTIMPL;
      algo = prop.ulVal;
    }
  }
  if (!SetFunctions(algo))
    return E_NOTIMPL;
  return S_OK;
}

#endif // Z7_EXTRACT_ONLY

} // namespace NCrypto

// Crypto/MyAes.h

#ifndef ZIP7_INC_CRYPTO_MY_AES_H
#define ZIP7_INC_CRYPTO_MY_AES_H

#include "../../../C/Aes.h"

#include "../../Common/MyBuffer2.h"
#include "../../Common/MyCom.h"

#include "../ICoder.h"
#include "crypto/opensslaes.h"

namespace NCrypto {

#ifdef Z7_EXTRACT_ONLY
#define Z7_IFACEN_IAesCoderSetFunctions(x)
#else
#define Z7_IFACEN_IAesCoderSetFunctions(x) \
  virtual bool SetFunctions(UInt32 algo) x
#endif


class CAesCoder:
  public ICompressFilter,
  public ICryptoProperties,
 #ifndef Z7_EXTRACT_ONLY
  public ICompressSetCoderProperties,
 #endif
  public CMyUnknownImp
{
  Z7_COM_QI_BEGIN2(ICompressFilter)
  Z7_COM_QI_ENTRY(ICryptoProperties)
 #ifndef Z7_EXTRACT_ONLY
  Z7_COM_QI_ENTRY(ICompressSetCoderProperties)
 #endif
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE
  
public:
  Z7_IFACE_COM7_IMP_NONFINAL(ICompressFilter)
  Z7_IFACE_COM7_IMP(ICryptoProperties)
private:
 #ifndef Z7_EXTRACT_ONLY
  Z7_IFACE_COM7_IMP(ICompressSetCoderProperties)
 #endif

protected:
  bool _keyIsSet;
  bool _encodeMode;
  bool _ctrMode;
  SevenzipOpenSslAes _opensslCipher;
  unsigned _keySize;
  unsigned _ctrPos; // we need _ctrPos here for Init() / SetInitVector()
private:
  CAlignedBuffer1 _aes;

  Byte _iv[AES_BLOCK_SIZE];

protected:
  UInt32 *Aes() { return (UInt32 *)(void *)(Byte *)_aes; }
  void Code(Byte *data, size_t numBlocks);
  void CodeCtr(Byte *data, size_t numBlocks);

  Z7_IFACE_PURE(IAesCoderSetFunctions)

public:
  CAesCoder(unsigned keySize, bool encodeMode = false, bool ctrMode = false);
  virtual ~CAesCoder() {} // we need virtual destructor for derived classes
  void SetKeySize(unsigned size) { _keySize = size; }
};


#ifndef Z7_EXTRACT_ONLY
struct CAesCbcEncoder : public CAesCoder {
    CAesCbcEncoder(unsigned keySize = 0) : CAesCoder(keySize, true) {}
    Z7_IFACE_IMP(IAesCoderSetFunctions)
};
#endif

struct CAesCbcDecoder: public CAesCoder
{
  CAesCbcDecoder(unsigned keySize = 0): CAesCoder(keySize)
  {
  }
  Z7_IFACE_IMP(IAesCoderSetFunctions)
};

#ifndef Z7_SFX
struct CAesCtrCoder: public CAesCoder
{
private:
  Z7_COM7F_IMP2(UInt32, Filter(Byte *data, UInt32 size))
public:
  CAesCtrCoder(unsigned keySize = 0) : CAesCoder(keySize, true, true) { _ctrPos = 0; }
  Z7_IFACE_IMP(IAesCoderSetFunctions)
};
#endif

}

#endif

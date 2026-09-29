// Rar5Handler.cpp
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov; see third_party/7zip/License.txt (including applicable RAR notices).
// Only the global native BLAKE2sp hasher/registration dependency is ported here.
// Archive handler/parser/decoder bodies remain in the official 7zz engine.
#include "../../../../../third_party/7zip/C/Blake2.h"
#include "7zip/Common/RegisterCodec.h"
#include "Common/MyBuffer2.h"
#include "Common/MyCom.h"
Z7_CLASS_IMP_COM_2(CBlake2spHasher, IHasher, ICompressSetCoderProperties)
CAlignedBuffer1 _buf;
// CBlake2sp _blake;
#define Z7_BLACK2S_ALIGN_OBJECT_OFFSET 0
CBlake2sp *Obj() { return (CBlake2sp *)(void *)((Byte *)_buf + Z7_BLACK2S_ALIGN_OBJECT_OFFSET); }

public:
Byte _mtDummy[1 << 7]; // it's public to eliminate clang warning: unused private field
CBlake2spHasher() : _buf(sizeof(CBlake2sp) + Z7_BLACK2S_ALIGN_OBJECT_OFFSET) {
    Blake2sp_SetFunction(Obj(), 0);
    Blake2sp_InitState(Obj());
}
}
;

Z7_COM7F_IMF2(void, CBlake2spHasher::Init()) { Blake2sp_InitState(Obj()); }

Z7_COM7F_IMF2(void, CBlake2spHasher::Update(const void *data, UInt32 size)) {
#if 1
    Blake2sp_Update(Obj(), (const Byte *)data, (size_t)size);
#else
    // for debug:
    for (;;) {
        if (size == 0)
            return;
        UInt32 size2 = (size * 0x85EBCA87) % size / 800;
        // UInt32 size2 = size / 2;
        if (size2 == 0)
            size2 = 1;
        Blake2sp_Update(Obj(), (const Byte *)data, size2);
        data = (const void *)((const Byte *)data + size2);
        size -= size2;
    }
#endif
}

Z7_COM7F_IMF2(void, CBlake2spHasher::Final(Byte *digest)) { Blake2sp_Final(Obj(), digest); }

Z7_COM7F_IMF(CBlake2spHasher::SetCoderProperties(const PROPID *propIDs,
                                                 const PROPVARIANT *coderProps, UInt32 numProps)) {
    unsigned algo = 0;
    for (UInt32 i = 0; i < numProps; i++) {
        if (propIDs[i] == NCoderPropID::kDefaultProp) {
            const PROPVARIANT &prop = coderProps[i];
            if (prop.vt != VT_UI4)
                return E_INVALIDARG;
            /*
            if (prop.ulVal > Z7_BLAKE2S_ALGO_MAX)
              return E_NOTIMPL;
            */
            algo = (unsigned)prop.ulVal;
        }
    }
    if (!Blake2sp_SetFunction(Obj(), algo))
        return E_NOTIMPL;
    return S_OK;
}

REGISTER_HASHER(CBlake2spHasher, 0x202, "BLAKE2sp", Z7_BLAKE2S_DIGEST_SIZE)

static struct CBlake2sp_Prepare {
    CBlake2sp_Prepare() { z7_Black2sp_Prepare(); }
} g_Blake2sp_Prepare;

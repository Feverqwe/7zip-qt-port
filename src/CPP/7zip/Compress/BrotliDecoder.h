// (C) 2017 Tino Reichardt. LGPL-2.1-or-later.
// Adapted from 7-Zip-zstd: raw RFC 7932 streams through Google's C decoder.
#ifndef ZIP7_INC_BROTLI_DECODER_H
#define ZIP7_INC_BROTLI_DECODER_H

#include "7zip/ICoder.h"
#include "Common/MyCom.h"

namespace NCompress {
namespace NBROTLI {
class CDecoder final : public ICompressCoder, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ICompressCoder)

  public:
    UInt64 _processedIn = 0;
    UInt64 _processedOut = 0;
    bool NeedMoreInput = false;
    bool DataAfterEnd = false;
    Z7_IFACE_COM7_IMP(ICompressCoder)
};
} // namespace NBROTLI
} // namespace NCompress
#endif

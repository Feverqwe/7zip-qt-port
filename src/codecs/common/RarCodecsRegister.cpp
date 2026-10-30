// RarCodecsRegister.cpp
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov; see third_party/7zip/License.txt (including RAR notices).
// Only RAR5/RAR7 decoder registration is ported; legacy RAR1/2/3 stay external.
#include "7zip/Common/RegisterCodec.h"
#include "7zip/Compress/Rar5Decoder.h"
#include "7zip/Compress/StdAfx.h"

namespace NCompress {
#define CREATE_CODEC(x) REGISTER_CODEC_CREATE(CreateCodec##x, NRar##x::CDecoder())
CREATE_CODEC(5)
#define RAR_CODEC(x, name) {CreateCodec##x, NULL, 0x40300 + x, "Rar" name, 1, false}
REGISTER_CODECS_VAR{RAR_CODEC(5, "5")};
REGISTER_CODECS(Rar)
} // namespace NCompress

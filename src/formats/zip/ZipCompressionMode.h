// CompressionMode.h
// Adapted include ownership from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.

#ifndef ZIP7_INC_ZIP_COMPRESSION_MODE_H
#define ZIP7_INC_ZIP_COMPRESSION_MODE_H

#include "Common/MyString.h"

#ifndef Z7_ST
#include "Windows/System.h"
#endif

#include "formats/common/HandlerOut.h"

namespace NArchive {
namespace NZip {

const CMethodId kMethodId_ZipBase = 0x040100;
const CMethodId kMethodId_BZip2 = 0x040202;

struct CBaseProps : public CMultiMethodProps {
    bool IsAesMode;
    Byte AesKeyMode;

    void Init() {
        CMultiMethodProps::Init();

        IsAesMode = false;
        AesKeyMode = 3;
    }
};

struct CCompressionMethodMode : public CBaseProps {
    CRecordVector<Byte> MethodSequence;
    AString Password; // _Wipe
    bool Password_Defined;
    bool Force_SeqOutMode;
    bool DataSizeReduce_Defined;
    UInt64 DataSizeReduce;

    bool IsRealAesMode() const { return Password_Defined && IsAesMode; }

    CCompressionMethodMode() {
        Password_Defined = false;
        Force_SeqOutMode = false;
        DataSizeReduce_Defined = false;
        DataSizeReduce = 0;
    }

#ifdef Z7_CPP_IS_SUPPORTED_default
    CCompressionMethodMode(const CCompressionMethodMode &) = default;
    CCompressionMethodMode &operator=(const CCompressionMethodMode &) = default;
#endif
    ~CCompressionMethodMode() { Password.Wipe_and_Empty(); }
};

} // namespace NZip
} // namespace NArchive

#endif

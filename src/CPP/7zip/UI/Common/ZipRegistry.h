// Adapted from 7-Zip CPP/7zip/UI/Common/ZipRegistry (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "../../Common/MethodProps.h"
#include <QString>
namespace NCompression {
struct CMemUse {
    // UString Str;
    bool IsDefined;
    bool IsPercent;
    UInt64 Val;

    CMemUse() : IsDefined(false), IsPercent(false), Val(0) {}

    void Clear() {
        // Str.Empty();
        IsDefined = false;
        IsPercent = false;
        Val = 0;
    }

    UInt64 GetBytes(UInt64 ramSize) const {
        if (!IsPercent)
            return Val;
        return Calc_From_Val_Percents(ramSize, Val);
    }
    void Parse(const QString &s);
};

} // namespace NCompression

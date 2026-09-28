// Adapted from 7-Zip CPP/7zip/Common/MethodProps.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "MethodProps.h"
UInt64 Calc_From_Val_Percents(UInt64 val, UInt64 percents) {
    // if (percents == 0) return 0;
    const UInt64 q = percents / 100;
    const UInt32 r = (UInt32)(percents % 100);
    UInt64 res = 0;

    if (q != 0) {
        if (val > (UInt64)(Int64)-1 / q)
            return (UInt64)(Int64)-1;
        res = val * q;
    }

    if (r != 0) {
        UInt64 v2;
        if (val <= (UInt64)(Int64)-1 / r)
            v2 = val * r / 100;
        else
            v2 = val / 100 * r;
        res += v2;
        if (res < v2)
            return (UInt64)(Int64)-1;
    }

    return res;
}

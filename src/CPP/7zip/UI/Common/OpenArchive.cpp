// Adapted from 7-Zip CPP/7zip/UI/Common/OpenArchive.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Complete GetOpenArcErrorFlags body; engine/open callbacks are not yet ported.
#include "OpenArchive.h"

using namespace NWindows;

UInt32 GetOpenArcErrorFlags(const NCOM::CPropVariant &prop, bool *isDefinedProp) {
    if (isDefinedProp != NULL)
        *isDefinedProp = false;

    switch (prop.vt) {
    case VT_UI8:
        if (isDefinedProp)
            *isDefinedProp = true;
        return (UInt32)prop.uhVal.QuadPart;
    case VT_UI4:
        if (isDefinedProp)
            *isDefinedProp = true;
        return prop.ulVal;
    case VT_EMPTY:
        return 0;
    default:
        throw 151199;
    }
}

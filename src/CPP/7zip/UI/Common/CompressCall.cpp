// Partial source port from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Original shared quoting body. Native launch is a typed integration boundary.
#include "CompressCall.h"

UString GetQuotedString(const UString &s) {
    UString s2('"');
    s2 += s;
    s2.Add_Char('"');
    return s2;
}

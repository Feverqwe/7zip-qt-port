// Source subset from 7-Zip ContextMenu.cpp (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Only the shared FindExt dependency; native shell integration remains P2-09.
#include "Common/MyString.h"
bool FindExt(const char *p, const UString &name, CStringFinder &finder) {
    const int dotPos = name.ReverseFind_Dot();
    int len = (int)name.Len() - (dotPos + 1);
    if (len == 0 || len > 32 || dotPos < 0)
        return false;
    return finder.FindWord_In_LowCaseAsciiList_NoCase(p, name.Ptr(dotPos + 1));
}

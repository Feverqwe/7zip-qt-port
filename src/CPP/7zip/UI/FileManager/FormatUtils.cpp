// Adapted from 7-Zip CPP/7zip/UI/FileManager/FormatUtils.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "FormatUtils.h"
#include "Common/IntToString.h"
#include "LangUtils.h"

UString NumberToString(UInt64 number) {
    wchar_t numberString[32];
    ConvertUInt64ToString(number, numberString);
    return numberString;
}

UString MyFormatNew(const UString &format, const UString &argument) {
    UString result = format;
    result.Replace(L"{0}", argument);
    return result;
}

UString MyFormatNew(UINT resourceID, const UString &argument) {
    return MyFormatNew(LangString(resourceID), argument);
}

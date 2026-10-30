// Adapted from 7-Zip CPP/7zip/UI/FileManager/FormatUtils.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "filemanager/FormatUtils.h"
#include "Common/IntToString.h"
#include "localization/LangUtils.h"
#include "localization/qtlanguageservices.h"

UString NumberToString(UInt64 number) {
    wchar_t numberString[32];
    ConvertUInt64ToString(number, numberString);
    return numberString;
}

UString MyFormatNew(const UString &format, const UString &argument) {
    const QString result = QString::fromWCharArray(format).arg(QString::fromWCharArray(argument));
    return UString(result.toStdWString().c_str());
}

UString MyFormatNew(UINT resourceID, UInt64 count) {
    return UString(QtTranslatedResourceCount(resourceID, count).toStdWString().c_str());
}

UString MyFormatNew(UINT resourceID, const UString &argument) {
    return MyFormatNew(LangString(resourceID), argument);
}

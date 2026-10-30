// Adapted from 7-Zip CPP/7zip/UI/FileManager/PropertyName.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "filemanager/PropertyName.h"
#include "Common/IntToString.h"
#include "localization/LangUtils.h"

UString GetNameOfProperty(PROPID propID, const wchar_t *name) {
    if (propID < 1000) {
        UString s = LangString(1000 + propID);
        if (!s.IsEmpty())
            return s;
    }
    if (name)
        return name;
    wchar_t temp[16];
    ConvertUInt32ToString(propID, temp);
    return temp;
}

// Adapted from 7-Zip CPP/7zip/UI/FileManager/PropertyName.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#ifndef ZIP7_INC_PROPERTY_NAME_H
#define ZIP7_INC_PROPERTY_NAME_H

#include "Common/MyString.h"
#include "Common/MyWindows.h"

UString GetNameOfProperty(PROPID propID, const wchar_t *name);

#endif

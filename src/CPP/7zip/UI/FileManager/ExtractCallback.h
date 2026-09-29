// Adapted from 7-Zip CPP/7zip/UI/FileManager/ExtractCallback.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Exposes the source error-message helper. Callback/operation classes remain
// mandatory in their archive operation/progress owners.
#ifndef ZIP7_INC_EXTRACT_CALLBACK_H
#define ZIP7_INC_EXTRACT_CALLBACK_H

#include "Common/MyString.h"

UString GetOpenArcErrorMessage(UInt32 errorFlags);

#endif

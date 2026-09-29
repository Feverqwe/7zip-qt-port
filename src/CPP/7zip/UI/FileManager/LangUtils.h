// Adapted from 7-Zip CPP/7zip/UI/FileManager/LangUtils.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Original non-Z7_LANG resource fallbacks only. Language-file loading,
// selection and platform default-language logic remain mandatory P1-14.
#ifndef ZIP7_INC_LANG_UTILS_H
#define ZIP7_INC_LANG_UTILS_H

#include "CPP/Windows/ResourceString.h"

inline UString LangString(UInt32 langID) { return NWindows::MyLoadString(langID); }
inline void LangString(UInt32 langID, UString &dest) { NWindows::MyLoadString(langID, dest); }
inline void AddLangString(UString &s, UInt32 langID) { s += NWindows::MyLoadString(langID); }

#endif

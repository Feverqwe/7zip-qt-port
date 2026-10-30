// EnumDirItems.h
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.

#ifndef ZIP7_INC_ENUM_DIR_ITEMS_H
#define ZIP7_INC_ENUM_DIR_ITEMS_H

#include "fileops/patterns/Wildcard.h"

#include "7zip/UI/Common/DirItem.h"

HRESULT EnumerateItems(const NWildcard::CCensor &censor, NWildcard::ECensorPathMode pathMode,
                       const UString &addPathPrefix, CDirItems &dirItems);

struct CMessagePathException : public UString {
    CMessagePathException(const char *a, const wchar_t *u = NULL);
    CMessagePathException(const wchar_t *a, const wchar_t *u = NULL);
};

HRESULT EnumerateDirItemsAndSort(NWildcard::CCensor &censor, NWildcard::ECensorPathMode pathMode,
                                 const UString &addPathPrefix, UStringVector &sortedPaths,
                                 UStringVector &sortedFullPaths, CDirItemsStat &st,
                                 IDirItemsCallback *callback);

#ifdef _WIN32
void ConvertToLongNames(NWildcard::CCensor &censor);
#endif

#endif

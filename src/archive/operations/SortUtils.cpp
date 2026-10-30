// SortUtils.cpp
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.

#include "7zip/UI/Common/StdAfx.h"

#include "fileops/patterns/Wildcard.h"

#include "7zip/UI/Common/SortUtils.h"

static int CompareStrings(const unsigned *p1, const unsigned *p2, void *param) {
    const UStringVector &strings = *(const UStringVector *)param;
    return CompareFileNames(strings[*p1], strings[*p2]);
}

void SortFileNames(const UStringVector &strings, CUIntVector &indices) {
    const unsigned numItems = strings.Size();
    indices.ClearAndSetSize(numItems);
    if (numItems == 0)
        return;
    unsigned *vals = &indices[0];
    for (unsigned i = 0; i < numItems; i++)
        vals[i] = i;
    indices.Sort(CompareStrings, (void *)&strings);
}

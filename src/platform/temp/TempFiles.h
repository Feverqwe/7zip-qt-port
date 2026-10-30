// TempFiles.h
// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.

#ifndef ZIP7_INC_TEMP_FILES_H
#define ZIP7_INC_TEMP_FILES_H

#include "Common/MyString.h"

class CTempFiles {
    void Clear();

  public:
    FStringVector Paths;
    bool NeedDeleteFiles;

    CTempFiles() : NeedDeleteFiles(true) {}
    ~CTempFiles() { Clear(); }
};

#endif

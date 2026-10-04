// Partial source port from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Original file-attribute record; native selection transport is adapted separately.
#pragma once
namespace NWindows {
namespace NShell {
struct CFileAttribs {
    int FirstDirIndex;
    // DWORD Sum;
    // DWORD Product;
    // CRecordVector<DWORD> Vals;
    // CRecordVector<bool> IsDirVector;

    CFileAttribs() { Clear(); }
    void Clear() {
        FirstDirIndex = -1;
        // Sum = 0;
        // Product = 0;
        // IsDirVector.Clear();
    }
};
} // namespace NShell
} // namespace NWindows

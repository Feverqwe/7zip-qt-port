// Adapted from 7-Zip CPP/Windows/ResourceString.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
// Win32 LoadString/HINSTANCE storage is replaced by compiled source STRINGTABLE
// strings and the original bundled English language resource.
#include "ResourceString.h"
#include "7zip/UI/Explorer/resource.h"
#include "7zip/UI/FileManager/LangPageRes.h"
#include "7zip/UI/FileManager/OverwriteDialogRes.h"
#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "7zip/UI/FileManager/PropertyNameRes.h"
#include "7zip/UI/FileManager/resource.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "7zip/UI/GUI/ExtractRes.h"
#include "7zip/UI/GUI/resource3.h"
#include "CPP/7zip/UI/FileManager/MessagesDialogRes.h"
#include "CPP/7zip/UI/GUI/resource2.h"
#include "qtlanguageservices.h"

namespace NWindows {

struct ResourceStringEntry {
    UINT Id;
    const wchar_t *Text;
};

#include "ResourceStrings.h"

UString MyLoadString(UINT resourceID) {
    for (const auto &entry : kResourceStrings)
        if (entry.Id == resourceID)
            return UString(
                QString::fromWCharArray(entry.Text).replace("{0}", "%1").toStdWString().c_str());
    return QtEnglishLanguageString(resourceID);
}

void MyLoadString(UINT resourceID, UString &dest) { dest = MyLoadString(resourceID); }

#ifdef _WIN32
// Original Unicode module-resource body: codec DLLs own a different string
// table from the compiled Qt File Manager captions.
static const int kStartSize = 256;
static void MyLoadString2(HINSTANCE hInstance, UINT resourceID, UString &s) {
    int size = kStartSize;
    int len;
    do {
        size <<= 1;
        len = ::LoadStringW(hInstance, resourceID, s.GetBuf((unsigned)size - 1), size);
    } while (size - len <= 1);
    s.ReleaseBuf_CalcLen((unsigned)len);
}
void MyLoadString(HINSTANCE hInstance, UINT resourceID, UString &dest) {
    dest.Empty();
    {
        wchar_t s[kStartSize];
        s[0] = 0;
        int len = ::LoadStringW(hInstance, resourceID, s, kStartSize);
        if (kStartSize - len > 1) {
            dest = s;
            return;
        }
    }
    MyLoadString2(hInstance, resourceID, dest);
}
#endif

} // namespace NWindows

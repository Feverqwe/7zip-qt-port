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
            return entry.Text;
    return QtEnglishLanguageString(resourceID);
}

void MyLoadString(UINT resourceID, UString &dest) { dest = MyLoadString(resourceID); }

#ifdef _WIN32
void MyLoadString(HINSTANCE, UINT resourceID, UString &dest) { MyLoadString(resourceID, dest); }
#endif

} // namespace NWindows

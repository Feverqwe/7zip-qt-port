// Adapted from 7-Zip RegistryAssociations.h (0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "Common/MyString.h"

namespace NRegistryAssoc {
struct CShellExtInfo {
    CSysString ProgramKey;
    UString IconPath;
    int IconIndex = -1;
    bool IsIt7Zip() const;
#ifdef _WIN32
    bool ReadFromRegistry(HKEY hkey, const CSysString &ext);
#endif
};
#ifdef _WIN32
LONG DeleteShellExtensionInfo(HKEY hkey, const CSysString &ext);
LONG AddShellExtensionInfo(HKEY hkey, const CSysString &ext, const UString &programTitle,
                           const UString &programOpenCommand, const UString &iconPath,
                           int iconIndex);
#endif
} // namespace NRegistryAssoc

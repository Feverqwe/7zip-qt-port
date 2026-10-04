// Adapted from 7-Zip 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17.
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "qtshellregistryservices.h"
// Project-owned identity; never replaces the official 7-Zip shell class.
inline constexpr const wchar_t *kSevenzipShellClsid = L"{52D06B47-DA74-4A5F-8B5E-4494C729CC9C}";
bool CheckContextMenuHandler(const UString &path, UInt32 wow,
                             NQtShellRegistry::IServices &services);
LONG SetContextMenuHandler(bool setMode, const UString &path, UInt32 wow,
                           NQtShellRegistry::IServices &services);

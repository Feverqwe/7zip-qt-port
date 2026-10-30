// Native Win32 dialog result values used by copied FileManager callers.
#pragma once
#include "Common/MyWindows.h"
#ifndef _WIN32
inline constexpr int IDOK = 1;
inline constexpr int IDCANCEL = 2;
inline constexpr int IDYES = 6;
inline constexpr int IDNO = 7;
inline constexpr int IDCLOSE = 8;
inline constexpr int IDHELP = 9;
inline constexpr int IDCONTINUE = 11;
#endif

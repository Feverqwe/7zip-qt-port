#pragma once
#include "Common/MyString.h"
#include "Common/MyWindows.h"
#include <functional>

struct COpenResult;

// FM.cpp's startup RAM globals are written once, then shared by all panels.
struct QtFileManagerRamInfo {
    size_t Size = 0;
    bool Defined = false;
};

// Per-launch values replace FM.cpp's process globals, without sharing state
// between independent Qt File Manager windows.
struct QtFileManagerStartup {
    UString FullPath;
    UString ArcFormat;
    bool NeedOpenArc = false;
    // Original WinMain/WinMain2 nCmdShow, default SW_SHOWNORMAL.
    int ShowCommand = 1;
};

struct QtFileManagerStartupFailure {
    UString Message;
};

QtFileManagerStartup WinMain2(const UString &nativeCommandLine);
QtFileManagerStartup WinMain2(const UString &nativePath, const UString &nativeTail);
void CheckFileManagerStartup(HRESULT result, const QtFileManagerStartup &startup,
                             const COpenResult &openRes);
namespace NQtFileManager {
const QtFileManagerRamInfo &GetRamInfo();
int WinMain(const std::function<int()> &main2);
} // namespace NQtFileManager
class QApplication;
int RunSevenzipFileManager(QApplication &application);

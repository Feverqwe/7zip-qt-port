#pragma once
#include "Common/MyString.h"
#include "Common/MyWindows.h"
#include <functional>

struct COpenResult;

// Per-launch values replace FM.cpp's process globals, without sharing state
// between independent Qt File Manager windows.
struct QtFileManagerStartup {
    UString FullPath;
    UString ArcFormat;
    bool NeedOpenArc = false;
};

struct QtFileManagerStartupFailure {
    UString Message;
};

QtFileManagerStartup WinMain2(const UString &nativeCommandLine);
void CheckFileManagerStartup(HRESULT result, const QtFileManagerStartup &startup,
                             const COpenResult &openRes);
namespace NQtFileManager {
int WinMain(const std::function<int()> &main2);
}
class QApplication;
int RunSevenzipFileManager(QApplication &application);

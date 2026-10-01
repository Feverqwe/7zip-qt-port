// Adapted from 7-Zip CPP/7zip/UI/FileManager/MyLoadMenu.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "Common/MyString.h"
class IQtFileMenuServices;
class IQtFileManagerCommandServices;
class IQtFileManagerMenuCommandServices;
class QMenuBar;
void MyLoadMenu(QMenuBar &menu, bool needResetMenu);
bool ExecuteFileCommand(unsigned id, IQtFileManagerCommandServices &app);
bool OnMenuCommand(unsigned id, IQtFileManagerMenuCommandServices &app);

struct CFileMenu {
    bool programMenu;
    bool readOnly;
    bool isHashFolder;
    bool isFsFolder;
    bool allAreFiles;
    bool isAltStreamsSupported;
    unsigned numItems;
    FString FilePath;

    CFileMenu()
        : programMenu(false), readOnly(false), isHashFolder(false), isFsFolder(false),
          allAreFiles(false), isAltStreamsSupported(true), numItems(0) {}
    void Load(IQtFileMenuServices &menu, unsigned startPos);
};

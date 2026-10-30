// Adapted from 7-Zip CPP/7zip/UI/FileManager/HelpUtils.cpp (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#include "filemanager/HelpUtils.h"
#include "Common/StringConvert.h"
#include "help/qthelpservices.h"

#define kHelpFileName "7-zip.chm::/"

void ShowHelpWindow(const char *topicFile, QWidget *parent) {
    // Source module-prefix + CHM member + topic construction. The native
    // HtmlHelp/hh.exe display service is replaced by the offline Qt adapter.
    FString path = QtHelpModuleDirPrefix();
    path += kHelpFileName;
    path += topicFile;
    QtDisplayHelpTopic(path, parent);
}

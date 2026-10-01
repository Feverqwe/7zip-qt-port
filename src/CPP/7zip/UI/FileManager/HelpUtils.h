// Adapted from 7-Zip CPP/7zip/UI/FileManager/HelpUtils.h (0766b73).
// Igor Pavlov, LGPL-2.1-or-later; see third_party/7zip/License.txt.
#pragma once
#include "Common/MyString.h"
class QWidget;
void ShowHelpWindow(const char *topicFile, QWidget *parent = nullptr);

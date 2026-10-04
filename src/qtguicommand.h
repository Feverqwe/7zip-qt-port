#pragma once
#include <functional>
class QtShellArchiveServices;
class QApplication;

// Native 7zG entry into the original GUI.cpp dispatcher/exit-code policy.
using QtGUIServiceFactory = std::function<QtShellArchiveServices &()>;
int SevenzipGUI(const QtGUIServiceFactory &factory);
int RunSevenzipGUI(QApplication &application);

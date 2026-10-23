#pragma once
#include <functional>
class QtShellArchiveServices;
class QApplication;
struct CArcCmdLineOptions;

// Native 7zG entry into the original GUI.cpp dispatcher/exit-code policy.
using QtGUIServiceFactory = std::function<QtShellArchiveServices &()>;
using QtGUICommandSource = std::function<void(CArcCmdLineOptions &)>;
int SevenzipGUI(const QtGUIServiceFactory &factory, const QtGUICommandSource &command = {});
int RunSevenzipGUI(QApplication &application);
int RunSevenzipGUICommand(QApplication &application, const QtGUICommandSource &command);

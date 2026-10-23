#pragma once
#include <QStringList>
#include <functional>
class QtShellArchiveServices;
class QApplication;
struct CArcCmdLineOptions;

// Native 7zG entry into the original GUI.cpp dispatcher/exit-code policy.
using QtGUIServiceFactory = std::function<QtShellArchiveServices &()>;
using QtGUICommandSource = std::function<void(CArcCmdLineOptions &)>;
using QtGUIOutputSink = std::function<void(const QString &)>;
int SevenzipGUI(const QtGUIServiceFactory &factory, const QtGUICommandSource &command = {});
int RunSevenzipGUI(QApplication &application);
int RunSevenzipGUICommand(QApplication &application, const QtGUICommandSource &command,
                          const QtGUIOutputSink &outputReady = {});

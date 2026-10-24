#pragma once
#include <QStringList>
#include <functional>
#include <optional>
class QtShellArchiveServices;
class QApplication;
struct CArcCmdLineOptions;

// Native 7zG entry into the original GUI.cpp dispatcher/exit-code policy.
using QtGUIServiceFactory = std::function<QtShellArchiveServices &(bool hashOnly)>;
struct QtGUIFileHashSelection {
    QString BasePrefix;
    QStringList Paths;
    bool EnterToDirs = true;
};
struct QtGUICommandSource {
    std::function<void(CArcCmdLineOptions &)> Configure;
    std::optional<QtGUIFileHashSelection> FileHash;
};
enum class QtGUIOutputKind { Directory, Archive };
using QtGUIOutputSink = std::function<void(const QString &, QtGUIOutputKind)>;
int SevenzipGUI(const QtGUIServiceFactory &factory, const QtGUICommandSource &command = {});
int RunSevenzipGUI(QApplication &application);
int RunSevenzipGUICommand(QApplication &application, const QtGUICommandSource &command,
                          const QtGUIOutputSink &outputReady = {});

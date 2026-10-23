// Independent 7zG host for a structured operation received over a local channel.
#pragma once
#include "qtguicommand.h"
#include <QString>

using QtGUIOperationRunner =
    std::function<int(const QtGUICommandSource &, const QtGUIOutputSink &)>;
int RunGUIOperationEndpoint(QApplication &application, const QString &channel,
                            const QtGUIOperationRunner &run);

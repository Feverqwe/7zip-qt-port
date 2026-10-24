#pragma once
#include <QString>

inline QString NativeOperationProgram(const char *builtProgram) {
    const QString installed = qEnvironmentVariable("SEVENZIP_OPERATION_GUI");
    return installed.isEmpty() ? QString::fromUtf8(builtProgram) : installed;
}

// Runtime packages deliberately omit the offscreen test plugin. The test parent
// is a QCoreApplication; only its actual GUI children need the native platform.
inline void ConfigureInstalledOperationPlatform() {
    if (qEnvironmentVariableIsEmpty("SEVENZIP_OPERATION_GUI"))
        return;
    const auto platform = qgetenv("SEVENZIP_OPERATION_QPA");
    if (platform.isEmpty())
        qunsetenv("QT_QPA_PLATFORM");
    else
        qputenv("QT_QPA_PLATFORM", platform);
}

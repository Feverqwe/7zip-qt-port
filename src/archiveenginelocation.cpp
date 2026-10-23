#include "archiveenginelocation.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

QString FindArchiveEngine() {
    const QString overridden = qEnvironmentVariable("SEVENZIP_ENGINE_PATH");
    if (!overridden.isEmpty())
        return overridden; // Explicit missing paths must report a launch error.
#ifdef Q_OS_WIN
    const QString dynamicName = "7z.exe";
    const QString moduleName = "7z.dll";
#else
    const QString dynamicName = "7z";
    const QString moduleName = "7z.so";
#endif
    const QDir app(QCoreApplication::applicationDirPath());
    QStringList candidates{app.filePath(dynamicName),
                           app.filePath("../Resources/7zip/" + dynamicName)};
#ifdef SEVENZIP_BUILD_CODEC_RUNTIME
    candidates.append(QDir(QString::fromUtf8(SEVENZIP_BUILD_CODEC_RUNTIME)).filePath(dynamicName));
#endif
    for (const QString &candidate : candidates) {
        const QDir directory = QFileInfo(candidate).dir();
        if (QFileInfo(candidate).isExecutable() ||
            QFileInfo(directory.filePath(moduleName)).isFile())
            return QDir::cleanPath(candidate);
    }
    const QString standalone = QStandardPaths::findExecutable("7zz");
    if (!standalone.isEmpty())
        return standalone;
    for (const QString &candidate : {"/opt/homebrew/bin/7zz", "/usr/local/bin/7zz"})
        if (QFileInfo(candidate).isExecutable())
            return candidate;
    return "7zz";
}

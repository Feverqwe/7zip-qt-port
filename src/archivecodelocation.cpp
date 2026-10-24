#include "archivecodelocation.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

QString FindArchiveModule(const QString &location) {
#ifdef Q_OS_WIN
    const QString name = QStringLiteral("7z.dll");
#else
    const QString name = QStringLiteral("7z.so");
#endif
    if (!location.isEmpty()) {
        const QFileInfo selected(location);
        if (selected.isDir())
            return QDir(selected.absoluteFilePath()).filePath(name);
        if (selected.fileName() == name)
            return selected.absoluteFilePath();
        return selected.dir().absoluteFilePath(name);
    }
    const auto configured = qEnvironmentVariable("SEVENZIP_CODEC_MODULE_PATH");
    if (!configured.isEmpty())
        return configured;
    const QDir app(QCoreApplication::applicationDirPath());
    QStringList candidates{app.filePath(name), app.filePath("../Resources/7zip/" + name),
                           app.filePath("../lib/7zip/" + name)};
#ifdef SEVENZIP_BUILD_CODEC_RUNTIME
    candidates.append(QDir(QString::fromUtf8(SEVENZIP_BUILD_CODEC_RUNTIME)).filePath(name));
#endif
    for (const auto &candidate : candidates)
        if (QFileInfo(candidate).isFile())
            return QDir::cleanPath(candidate);
    return candidates.first();
}

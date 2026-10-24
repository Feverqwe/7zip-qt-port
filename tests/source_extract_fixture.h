#pragma once
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
namespace SourceExtractFixture {
inline const QString kName = QStringLiteral("inside/密碼 с пробелом.bin");
inline bool writeFile(const QString &path, const QByteArray &bytes) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        return false;
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}
inline QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
inline QString makeArchive(const QString &directory, const QByteArray &bytes) {
    if (!writeFile(QDir(directory).filePath(kName), bytes))
        return {};
    QProcess process;
    process.setWorkingDirectory(directory);
    process.start(QStandardPaths::findExecutable("7z"), {"a", "-tzip", "input.zip", kName});
    if (!process.waitForFinished(10000) || process.exitCode() != 0)
        return {};
    return QDir(directory).filePath("input.zip");
}
} // namespace SourceExtractFixture

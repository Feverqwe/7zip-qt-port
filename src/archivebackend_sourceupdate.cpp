#include "CPP/7zip/UI/Common/ExtractingFilePath.h"
#include "CPP/Common/Wildcard.h"
#include "archivebackend.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>

bool IsSafePath(const UString &path);

bool ArchiveBackend::sourceUpdatePathIsSafe(const QString &path) {
    const auto value = path.toStdWString();
    const UString native(value.c_str());
    if (path.contains(QChar(0)) || !IsSafePath(native))
        return false;
    UStringVector parts;
    SplitPathToParts(native, parts);
    UStringVector corrected = parts;
    Correct_FsPath(false, false, corrected, false);
    if (corrected.Size() != parts.Size())
        return false;
    FOR_VECTOR(i, parts) {
        if (parts[i] != corrected[i])
            return false;
    }
    return true;
}

void ArchiveBackend::updateSourceFile(const QString &archive, const QString &entry,
                                      const QString &sourceRoot, const QString &output,
                                      const QString &format, const QString &password) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    if ((format != "7z" && format != "zip") || !sourceUpdatePathIsSafe(entry) ||
        !QFileInfo(archive).isFile() || !QFileInfo(QDir(sourceRoot).filePath(entry)).isFile() ||
        output.isEmpty() || QFileInfo::exists(output)) {
        emit finished(false, QStringLiteral("Недопустимый файл или путь обновления архива."));
        return;
    }
    archive_ = QFileInfo(archive).absoluteFilePath();
    createTemporary_ = QFileInfo(output).absoluteFilePath();
    pendingPassword_ = password;
    // ArchiveCommandLine.cpp ParseUpdateCommandString: '-' suppresses modification
    // of the input; '!' selects the separate output. All matching states compress
    // the source file, preserving the native UpdateOneFile NewData/NewProps choice.
    start(Job::SourceUpdate,
          {"u", "-y", "-bsp1", "-bb1", "-spd", "-t" + format, "-u-",
           "-up1q1r2x2y2z2w2!" + createTemporary_, "--", archive_, entry},
          sourceRoot);
}

void ArchiveBackend::updateSourceMutation(const QString &archive, const QString &output,
                                          const QString &format, SourceMutation operation,
                                          const QStringList &paths, const QString &sourceRoot) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    const QFileInfo input(archive);
    const QFileInfo result(output);
    if (encrypted_ || (format != "7z" && format != "zip") || !input.isFile() || input.isSymLink() ||
        result.exists() || paths.isEmpty()) {
        emit finished(false, QStringLiteral("Недопустимый запрос на изменение архива."));
        return;
    }
    for (const QString &path : paths) {
        if (!sourceUpdatePathIsSafe(path) || path.contains('*') || path.contains('?') ||
            path.startsWith('@')) {
            emit finished(false, QStringLiteral("Небезопасный путь в архиве."));
            return;
        }
    }
    if ((operation == SourceMutation::Rename && paths.size() != 2) ||
        (operation == SourceMutation::CreateFolder && paths.size() != 1) ||
        (operation != SourceMutation::CreateFolder && !sourceRoot.isEmpty())) {
        emit finished(false, QStringLiteral("Недопустимый план изменения архива."));
        return;
    }
    if (operation == SourceMutation::CreateFolder &&
        !QFileInfo(QDir(sourceRoot).filePath(paths.front())).isDir()) {
        emit finished(false, QStringLiteral("Новая папка не подготовлена."));
        return;
    }
    if (!QFile::copy(input.absoluteFilePath(), result.absoluteFilePath())) {
        emit finished(false, QStringLiteral("Не удалось подготовить копию архива."));
        return;
    }
    createTemporary_ = result.absoluteFilePath();
    QStringList args;
    switch (operation) {
    case SourceMutation::Delete:
        args = {"d", "-y", "-bsp1", "-bb1", "-spd", "--", createTemporary_};
        args += paths;
        break;
    case SourceMutation::Rename:
        args = {"rn", "-y", "-bsp1", "-bb1", "-spd", "--", createTemporary_, paths[0], paths[1]};
        break;
    case SourceMutation::CreateFolder:
        args = {"a",           "-y", "-bsp1",          "-bb1",  "-spd",
                "-t" + format, "--", createTemporary_, paths[0]};
        break;
    }
    start(Job::SourceMutation, args, sourceRoot);
}

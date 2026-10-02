#include "CPP/7zip/UI/Common/ExtractingFilePath.h"
#include "CPP/Common/Wildcard.h"
#include "archivebackend.h"
#include "qtlanguageservices.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

bool IsSafePath(const UString &path);

bool ArchiveBackend::sourceUpdatePathIsSafe(const QString &path, bool allowAbsolute) {
    const auto value = path.toStdWString();
    const UString native(value.c_str());
    if (path.contains(QChar(0)) || !IsSafePath(native)) {
        if (!allowAbsolute || !QDir::isAbsolutePath(path))
            return false;
        const QString normalized = QDir::fromNativeSeparators(path);
        if (normalized.split('/').contains("..") || normalized.split('/').contains("."))
            return false;
        QString relative;
#ifdef Q_OS_WIN
        const QRegularExpression rootPattern(R"(^(?:[A-Za-z]:/|//[^/]+/[^/]+/))");
        const auto match = rootPattern.match(normalized);
        if (!match.hasMatch())
            return false;
        relative = normalized.mid(match.capturedLength());
#else
        relative = normalized.mid(1);
#endif
        return !relative.isEmpty() && sourceUpdatePathIsSafe(relative);
    }
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

bool ArchiveBackend::prepareSourceSelection(const QString &archive, const QStringList &sources,
                                            QString *root, QStringList *relativePaths,
                                            QString *error) {
    if (!root || !relativePaths || !error || sources.isEmpty())
        return false;
    const QFileInfo target(archive);
    QString commonRoot = QFileInfo(sources.first()).absolutePath();
    while (!commonRoot.isEmpty()) {
        bool containsAll = true;
        for (const QString &source : sources) {
            const QString relative =
                QDir(commonRoot).relativeFilePath(QFileInfo(source).absoluteFilePath());
            if (relative == ".." || relative.startsWith("../") || QDir::isAbsolutePath(relative)) {
                containsAll = false;
                break;
            }
        }
        if (containsAll)
            break;
        const QString parent = QFileInfo(commonRoot).absolutePath();
        if (parent == commonRoot)
            break;
        commonRoot = parent;
    }
    QStringList names;
    for (const QString &source : sources) {
        const QFileInfo info(source);
        const QString relative = QDir(commonRoot).relativeFilePath(info.absoluteFilePath());
        if (!info.exists() || info.isSymLink() || relative == ".." || relative.startsWith("../") ||
            QDir::isAbsolutePath(relative) || !sourceUpdatePathIsSafe(relative) ||
            relative.contains('*') || relative.contains('?') || relative.startsWith('@') ||
            info.absoluteFilePath() == target.absoluteFilePath()) {
            *error =
                QtLocalText("Update source unavailable: %1", "Недоступный источник обновления: %1")
                    .arg(source);
            return false;
        }
        if (info.isDir()) {
            const QString nested =
                QDir(info.canonicalFilePath()).relativeFilePath(target.absoluteFilePath());
            if (nested == "." ||
                (nested != ".." && !nested.startsWith("../") && !QDir::isAbsolutePath(nested))) {
                *error = QtLocalText("The archive is inside the selected folder.",
                                     "Архив находится внутри выбранной папки.");
                return false;
            }
            QDirIterator iterator(info.absoluteFilePath(),
                                  QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden |
                                      QDir::System,
                                  QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                iterator.next();
                if (iterator.fileInfo().isSymLink()) {
                    *error = QtLocalText("Sources must not contain links: %1",
                                         "Источники не должны содержать ссылки: %1")
                                 .arg(iterator.filePath());
                    return false;
                }
            }
        }
        names.append(relative);
    }
    *root = commonRoot;
    *relativePaths = names;
    return true;
}

void ArchiveBackend::updateSourceFile(const QString &archive, const QString &entry,
                                      const QString &sourceRoot, const QString &output,
                                      const QString &format, const QString &password) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    if (!supportsArchiveUpdate(format) || !sourceUpdatePathIsSafe(entry) ||
        !QFileInfo(archive).isFile() || !QFileInfo(QDir(sourceRoot).filePath(entry)).isFile() ||
        output.isEmpty() || QFileInfo::exists(output)) {
        emit finished(false, QtLocalText("Invalid archive update file or path.",
                                         "Недопустимый файл или путь обновления архива."));
        return;
    }
    archive_ = QFileInfo(archive).absoluteFilePath();
    createTemporary_ = QFileInfo(output).absoluteFilePath();
    pendingPassword_ = password;
    // ArchiveCommandLine.cpp ParseUpdateCommandString: '-' suppresses modification
    // of the input; '!' selects the separate output. All matching states compress
    // the source file, preserving the native UpdateOneFile NewData/NewProps choice.
    QStringList arguments = {
        "u",    "-y",          "-bsp1", "-bb1",
        "-spd", "-t" + format, "-u-",   "-up1q1r2x2y2z2w2!" + createTemporary_};
    // ZipHandlerOut.cpp uses the update callback's PasswordIsDefined for new
    // data. CLI reading credentials alone do not enable encryption of that data.
    // The existing 7z handler inherits encryption from the opened archive.
    if (format == "zip" && !password.isEmpty())
        arguments.append("-p");
    arguments += QStringList{"--", archive_, entry};
    start(Job::SourceUpdate, arguments, sourceRoot);
}

void ArchiveBackend::updateSourceAdd(const QString &archive, const QStringList &entries,
                                     const QString &sourceRoot, const QString &output,
                                     const QString &format, const QString &password) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    const QFileInfo input(archive);
    const QFileInfo result(output);
    const QFileInfo root(sourceRoot);
    if (!supportsArchiveUpdate(format) || !input.isFile() || input.isSymLink() || result.exists() ||
        result.isSymLink() || !root.isDir() || root.isSymLink() || entries.isEmpty()) {
        emit finished(false, QtLocalText("Invalid archive add request.",
                                         "Недопустимый запрос на добавление в архив."));
        return;
    }
    for (const QString &entry : entries) {
        if (!sourceUpdatePathIsSafe(entry) || entry.contains('*') || entry.contains('?') ||
            entry.startsWith('@')) {
            emit finished(false, QtLocalText("Unsafe archive add path.",
                                             "Небезопасный путь добавления в архив."));
            return;
        }
        const QFileInfo staged(QDir(root.absoluteFilePath()).filePath(entry));
        if ((!staged.isFile() && !staged.isDir()) || staged.isSymLink() ||
            !staged.canonicalFilePath().startsWith(root.canonicalFilePath() + QDir::separator())) {
            emit finished(false, QtLocalText("The added file has not been prepared.",
                                             "Файл добавления не подготовлен."));
            return;
        }
    }
    archive_ = input.absoluteFilePath();
    createTemporary_ = result.absoluteFilePath();
    pendingPassword_ = password;
    QStringList args = {"u",    "-y",          "-bsp1", "-bb1",
                        "-spd", "-t" + format, "-u-",   "-up1q1r2x2y2z2w2!" + createTemporary_,
                        "--",   archive_};
    args += entries;
    start(Job::SourceUpdate, args, root.absoluteFilePath());
}

void ArchiveBackend::updateSourceMutation(const QString &archive, const QString &output,
                                          const QString &format, SourceMutation operation,
                                          const QStringList &paths, const QString &sourceRoot,
                                          const QString &password) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    const QFileInfo input(archive);
    const QFileInfo result(output);
    if (!supportsArchiveUpdate(format) || !input.isFile() || input.isSymLink() || result.exists() ||
        paths.isEmpty()) {
        emit finished(false, QtLocalText("Invalid archive mutation request.",
                                         "Недопустимый запрос на изменение архива."));
        return;
    }
    for (const QString &path : paths) {
        if (!sourceUpdatePathIsSafe(path) || path.contains('*') || path.contains('?') ||
            path.startsWith('@')) {
            emit finished(false,
                          QtLocalText("Unsafe archive path.", "Небезопасный путь в архиве."));
            return;
        }
    }
    if ((operation == SourceMutation::Rename && paths.size() != 2) ||
        (operation == SourceMutation::CreateFolder && paths.size() != 1) ||
        (operation != SourceMutation::CreateFolder && !sourceRoot.isEmpty())) {
        emit finished(false, QtLocalText("Invalid archive mutation plan.",
                                         "Недопустимый план изменения архива."));
        return;
    }
    if (operation == SourceMutation::CreateFolder &&
        !QFileInfo(QDir(sourceRoot).filePath(paths.front())).isDir()) {
        emit finished(false, QtLocalText("The new folder has not been prepared.",
                                         "Новая папка не подготовлена."));
        return;
    }
    if (!QFile::copy(input.absoluteFilePath(), result.absoluteFilePath())) {
        emit finished(false, QtLocalText("Could not prepare a copy of the archive.",
                                         "Не удалось подготовить копию архива."));
        return;
    }
    createTemporary_ = result.absoluteFilePath();
    pendingPassword_ = password;
    QStringList args;
    switch (operation) {
    case SourceMutation::Delete:
        args = {"d", "-y", "-bsp1", "-bb1", "-spd", "-t" + format, "--", createTemporary_};
        args += paths;
        break;
    case SourceMutation::Rename:
        args = {"rn",          "-y", "-bsp1",          "-bb1",   "-spd",
                "-t" + format, "--", createTemporary_, paths[0], paths[1]};
        break;
    case SourceMutation::CreateFolder:
        args = {"a",           "-y", "-bsp1",          "-bb1",  "-spd",
                "-t" + format, "--", createTemporary_, paths[0]};
        break;
    }
    // This is an exact private file path, including extensionless spools.
    // Source CArchivePath::ParseFromPath(k_ArcNameMode_Exact) prevents the
    // console updater from appending its format extension to that input.
    args.insert(1, QStringLiteral("-sae"));
    start(Job::SourceMutation, args, sourceRoot);
}

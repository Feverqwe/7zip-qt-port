#include "CPP/7zip/UI/Common/ExtractingFilePath.h"
#include "CPP/Common/Wildcard.h"
#include "archivebackend.h"
#include <QDir>
#include <QDirIterator>
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
            *error = QStringLiteral("Недоступный источник обновления: %1").arg(source);
            return false;
        }
        if (info.isDir()) {
            const QString nested =
                QDir(info.canonicalFilePath()).relativeFilePath(target.absoluteFilePath());
            if (nested == "." ||
                (nested != ".." && !nested.startsWith("../") && !QDir::isAbsolutePath(nested))) {
                *error = QStringLiteral("Архив находится внутри выбранной папки.");
                return false;
            }
            QDirIterator iterator(info.absoluteFilePath(),
                                  QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden |
                                      QDir::System,
                                  QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                iterator.next();
                if (iterator.fileInfo().isSymLink()) {
                    *error = QStringLiteral("Источники не должны содержать ссылки: %1")
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

void ArchiveBackend::updateSourceAdd(const QString &archive, const QStringList &entries,
                                     const QString &sourceRoot, const QString &output,
                                     const QString &format, const QString &password) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    const QFileInfo input(archive);
    const QFileInfo result(output);
    const QFileInfo root(sourceRoot);
    if ((format != "7z" && format != "zip") || !input.isFile() || input.isSymLink() ||
        result.exists() || result.isSymLink() || !root.isDir() || root.isSymLink() ||
        entries.isEmpty()) {
        emit finished(false, QStringLiteral("Недопустимый запрос на добавление в архив."));
        return;
    }
    for (const QString &entry : entries) {
        if (!sourceUpdatePathIsSafe(entry) || entry.contains('*') || entry.contains('?') ||
            entry.startsWith('@')) {
            emit finished(false, QStringLiteral("Небезопасный путь добавления в архив."));
            return;
        }
        const QFileInfo staged(QDir(root.absoluteFilePath()).filePath(entry));
        if ((!staged.isFile() && !staged.isDir()) || staged.isSymLink() ||
            !staged.canonicalFilePath().startsWith(root.canonicalFilePath() + QDir::separator())) {
            emit finished(false, QStringLiteral("Файл добавления не подготовлен."));
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

void ArchiveBackend::updateSourcePlan(const QString &archive, const SourceUpdatePlan &plan,
                                      const QString &output, const QString &format,
                                      const QString &password,
                                      const std::optional<CompressionOptions> &options) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    const QFileInfo input(archive);
    const QFileInfo result(output);
    if ((format != "7z" && format != "zip") || !input.isFile() || input.isSymLink() ||
        result.exists() || result.isSymLink()) {
        emit finished(false, QStringLiteral("Недопустимый запрос на обновление архива."));
        return;
    }
    if (!password.isEmpty()) {
        emit finished(false, QStringLiteral("Обновление защищённого архива пока недоступно."));
        return;
    }
    const QDir fileRoot(plan.fileRoot);
    const QDir directoryRoot(plan.directoryRoot);
    if ((!plan.files.isEmpty() && (!fileRoot.exists() || QFileInfo(plan.fileRoot).isSymLink())) ||
        (!plan.directories.isEmpty() &&
         (!directoryRoot.exists() || QFileInfo(plan.directoryRoot).isSymLink()))) {
        emit finished(false, QStringLiteral("Подготовленные источники недоступны."));
        return;
    }
    for (const QStringList *group : {&plan.removed, &plan.files, &plan.directories}) {
        for (const QString &path : *group) {
            if (!sourceUpdatePathIsSafe(path) || path.contains('*') || path.contains('?') ||
                path.startsWith('@')) {
                emit finished(false, QStringLiteral("Небезопасный путь обновления архива."));
                return;
            }
        }
    }
    for (const QString &path : plan.files) {
        const QFileInfo staged(fileRoot.filePath(path));
        if (!staged.isFile() || staged.isSymLink() ||
            !staged.canonicalFilePath().startsWith(fileRoot.canonicalPath() + '/')) {
            emit finished(false, QStringLiteral("Файл обновления не подготовлен."));
            return;
        }
    }
    for (const QString &path : plan.directories) {
        const QFileInfo staged(directoryRoot.filePath(path));
        if (!staged.isDir() || staged.isSymLink() ||
            !staged.canonicalFilePath().startsWith(directoryRoot.canonicalPath() + '/')) {
            emit finished(false, QStringLiteral("Папка обновления не подготовлена."));
            return;
        }
    }
    QStringList switches;
    if (options && (!plan.files.isEmpty() || !plan.directories.isEmpty())) {
        QString error;
        if (!compressionSwitches(format, password, false, *options, &switches, &error)) {
            emit finished(false, error);
            return;
        }
    }
    if (!QFile::copy(input.absoluteFilePath(), result.absoluteFilePath())) {
        emit finished(false, QStringLiteral("Не удалось подготовить копию архива."));
        return;
    }
    archive_ = input.absoluteFilePath();
    createTemporary_ = result.absoluteFilePath();
    sourcePlan_ = plan;
    sourcePlanPassword_ = password;
    sourcePlanFormat_ = format;
    sourcePlanSwitches_ = switches;
    if (!plan.removed.isEmpty())
        startSourcePlanStage(Job::SourcePlanDelete);
    else if (!plan.files.isEmpty())
        startSourcePlanStage(Job::SourcePlanFiles);
    else if (!plan.directories.isEmpty())
        startSourcePlanStage(Job::SourcePlanDirectories);
    else {
        lastResult_ = S_OK;
        sourcePlanPassword_.clear();
        emit finished(true, QStringLiteral("Архив не требует обновления."));
    }
}

void ArchiveBackend::startSourcePlanStage(Job stage) {
    pendingPassword_ = sourcePlanPassword_;
    QStringList args = {stage == Job::SourcePlanDelete ? "d" : "a", "-y", "-bsp1", "-bb1", "-spd"};
    if (stage != Job::SourcePlanDelete)
        args << "-t" + sourcePlanFormat_;
    if (stage != Job::SourcePlanDelete)
        args += sourcePlanSwitches_;
    args << "--" << createTemporary_;
    if (stage == Job::SourcePlanDelete) {
        args += sourcePlan_.removed;
        start(stage, args);
    } else if (stage == Job::SourcePlanFiles) {
        args += sourcePlan_.files;
        start(stage, args, sourcePlan_.fileRoot);
    } else {
        args += sourcePlan_.directories;
        start(stage, args, sourcePlan_.directoryRoot);
    }
}

void ArchiveBackend::updateSourceMutation(const QString &archive, const QString &output,
                                          const QString &format, SourceMutation operation,
                                          const QStringList &paths, const QString &sourceRoot,
                                          const QString &password) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    const QFileInfo input(archive);
    const QFileInfo result(output);
    if ((format != "7z" && format != "zip") || !input.isFile() || input.isSymLink() ||
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
    pendingPassword_ = password;
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

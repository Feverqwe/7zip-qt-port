#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "qtlanguageservices.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <algorithm>

bool ArchiveBackend::validateSourcePlan(const QString &archive, const SourceUpdatePlan &plan,
                                        const QString &output, const QString &format,
                                        const QString &password,
                                        const std::optional<CompressionOptions> &options) {
    if (options && !options->VolumeSizes.IsEmpty()) {
        emit finished(false, QtLocalText("Updating archive volumes is not supported.",
                                         "Обновление архивных томов не поддерживается."));
        return false;
    }
    const QFileInfo input(archive);
    const QFileInfo result(output);
    if (!supportsArchiveUpdate(format) || !input.isFile() || input.isSymLink() || result.exists() ||
        result.isSymLink()) {
        emit finished(false, QtLocalText("Invalid archive update request.",
                                         "Недопустимый запрос на обновление архива."));
        return false;
    }
    if (!password.isEmpty() && !plan.allowProtected) {
        emit finished(false, QtLocalText("Protected archive update has not been confirmed.",
                                         "Обновление защищённого архива не подтверждено."));
        return false;
    }
    const QDir fileRoot(plan.fileRoot);
    const QDir directoryRoot(plan.directoryRoot);
    if ((!plan.files.isEmpty() && (!fileRoot.exists() || QFileInfo(plan.fileRoot).isSymLink())) ||
        (!plan.directories.isEmpty() &&
         (!directoryRoot.exists() || QFileInfo(plan.directoryRoot).isSymLink()))) {
        emit finished(false, QtLocalText("Prepared sources are unavailable.",
                                         "Подготовленные источники недоступны."));
        return false;
    }
    for (const QString &path : plan.removed + plan.removedDirectories + plan.retainedPaths)
        if (!sourceUpdatePathIsSafe(path, plan.allowAbsolute) || path.contains('*') ||
            path.contains('?') || path.startsWith('@')) {
            emit finished(false, QtLocalText("Unsafe archive update path.",
                                             "Небезопасный путь обновления архива."));
            return false;
        }
    for (const QStringList *group : {&plan.files, &plan.directories})
        for (const QString &path : *group)
            if (!sourceUpdatePathIsSafe(path) || path.contains('*') || path.contains('?') ||
                path.startsWith('@')) {
                emit finished(false, QtLocalText("Unsafe archive update path.",
                                                 "Небезопасный путь обновления архива."));
                return false;
            }
    for (const auto &rename : plan.renames)
        if (!sourceUpdatePathIsSafe(rename.first) ||
            !sourceUpdatePathIsSafe(rename.second, plan.allowAbsolute) ||
            (!plan.files.contains(rename.first) && !plan.directories.contains(rename.first))) {
            emit finished(false, QtLocalText("Unsafe update rename.",
                                             "Небезопасное переименование обновления."));
            return false;
        }
    for (const QString &path : plan.files) {
        const QFileInfo staged(fileRoot.filePath(path));
        if (!staged.isFile() || staged.isSymLink() ||
            !staged.canonicalFilePath().startsWith(fileRoot.canonicalPath() + '/')) {
            emit finished(false, QtLocalText("The update file has not been prepared.",
                                             "Файл обновления не подготовлен."));
            return false;
        }
    }
    for (const QString &path : plan.directories) {
        const QFileInfo staged(directoryRoot.filePath(path));
        if (!staged.isDir() || staged.isSymLink() ||
            !staged.canonicalFilePath().startsWith(directoryRoot.canonicalPath() + '/')) {
            emit finished(false, QtLocalText("The update folder has not been prepared.",
                                             "Папка обновления не подготовлена."));
            return false;
        }
    }
    return true;
}

bool ArchiveBackend::startKeepNameSourcePlan(const QString &archive, const SourceUpdatePlan &plan,
                                             const QString &output, const QString &format,
                                             const QString &password, const QStringList &switches) {
    const QFileInfo input(archive), result(output);
    const QDir fileRoot(plan.fileRoot);
    const auto catalog = codecCatalog();
    const int formatIndex = catalog ? catalog->formatIndex(format) : -1;
    if (formatIndex >= 0 &&
        catalog->nativeCodecs().Formats[unsigned(formatIndex)].Flags_KeepName() &&
        plan.files.size() == 1 && plan.directories.isEmpty() && plan.removed.isEmpty() &&
        plan.removedDirectories.isEmpty() && plan.renames.isEmpty() &&
        (plan.retainedPaths.isEmpty() || plan.retainedPaths == plan.files)) {
        // Preserve the input archive's source-derived default name. A copied
        // result.bzip2/result.xz would be listed as "result" before Add, so the
        // replacement would look like a second item to a one-stream handler.
        archive_ = input.absoluteFilePath();
        createTemporary_ = result.absoluteFilePath();
        pendingPassword_ = password;
        QStringList args = {"u",    "-y",          "-bsp1", "-bb1",
                            "-spd", "-t" + format, "-u-",   "-up1q1r2x2y2z2w2!" + createTemporary_};
        args += switches;
        args << "--" << archive_ << plan.files.front();
        start(Job::SourceUpdate, args, fileRoot.absolutePath());
        return true;
    }
    return false;
}

void ArchiveBackend::updateSourcePlan(const QString &archive, const SourceUpdatePlan &plan,
                                      const QString &output, const QString &format,
                                      const QString &password,
                                      const std::optional<CompressionOptions> &options) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    if (!validateSourcePlan(archive, plan, output, format, password, options))
        return;
    const QFileInfo input(archive), result(output);
    QStringList switches;
    if (options && (!plan.files.isEmpty() || !plan.directories.isEmpty())) {
        QString error;
        if (!compressionSwitches(format, password, false, *options, &switches, &error)) {
            emit finished(false, error);
            return;
        }
        if (!password.isEmpty() && format == "7z") {
            switches.removeAll("-p");
            switches.removeAll("-mhe=off");
        }
    }
    if (startKeepNameSourcePlan(archive, plan, output, format, password, switches))
        return;
    if (!QFile::copy(input.absoluteFilePath(), result.absoluteFilePath())) {
        emit finished(false, QtLocalText("Could not prepare a copy of the archive.",
                                         "Не удалось подготовить копию архива."));
        return;
    }
    archive_ = input.absoluteFilePath();
    createTemporary_ = result.absoluteFilePath();
    sourcePlan_ = plan;
    // CLI deletion matches a directory's descendants as well. Remove omitted
    // directories deepest first, excluding the source callback's retained items.
    std::sort(sourcePlan_.removedDirectories.begin(), sourcePlan_.removedDirectories.end(),
              [](const QString &a, const QString &b) { return a.count('/') > b.count('/'); });
    sourcePlanPassword_ = password;
    sourcePlanFormat_ = format;
    sourcePlanSwitches_ = switches;
    if (!plan.removed.isEmpty() || !plan.removedDirectories.isEmpty())
        startSourcePlanStage(Job::SourcePlanDelete);
    else if (!plan.files.isEmpty())
        startSourcePlanStage(Job::SourcePlanFiles);
    else if (!plan.directories.isEmpty())
        startSourcePlanStage(Job::SourcePlanDirectories);
    else if (!plan.renames.isEmpty())
        startSourcePlanStage(Job::SourcePlanRename);
    else {
        lastResult_ = S_OK;
        sourcePlanPassword_.clear();
        emit finished(true, QtLocalText("The archive does not need updating.",
                                        "Архив не требует обновления."));
    }
}

void ArchiveBackend::startSourcePlanStage(Job stage) {
    pendingPassword_ = sourcePlanPassword_;
    if (stage == Job::SourcePlanRename) {
        QStringList args = {"rn",
                            "-y",
                            "-bsp1",
                            "-bb1",
                            "-spd",
                            "-spf",
                            "-t" + sourcePlanFormat_,
                            "--",
                            createTemporary_};
        for (const auto &rename : sourcePlan_.renames)
            args << rename.first << rename.second;
        start(stage, args);
        return;
    }
    QStringList args = {stage == Job::SourcePlanDelete ? "d" : "a", "-y", "-bsp1", "-bb1", "-spd"};
    if (stage == Job::SourcePlanDelete && sourcePlan_.allowAbsolute)
        args << "-spf";
    args << "-t" + sourcePlanFormat_;
    if (stage != Job::SourcePlanDelete)
        args += sourcePlanSwitches_;
    QStringList removed;
    if (stage == Job::SourcePlanDelete) {
        if (!sourcePlan_.removed.isEmpty()) {
            removed = sourcePlan_.removed;
            sourcePlan_.removed.clear();
        } else {
            const QString directory = sourcePlan_.removedDirectories.takeFirst();
            removed.append(directory);
            for (const QString &path : sourcePlan_.retainedPaths)
                if (path.startsWith(directory + '/'))
                    args << "-x!" + path;
        }
    }
    args << "--" << createTemporary_;
    if (stage == Job::SourcePlanDelete) {
        args += removed;
        start(stage, args);
    } else if (stage == Job::SourcePlanFiles) {
        args += sourcePlan_.files;
        start(stage, args, sourcePlan_.fileRoot);
    } else {
        args += sourcePlan_.directories;
        start(stage, args, sourcePlan_.directoryRoot);
    }
}

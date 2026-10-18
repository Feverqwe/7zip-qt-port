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
                                            QString *error, bool storeLinks) {
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
        if ((!info.exists() && !(storeLinks && info.isSymLink())) ||
            (info.isSymLink() && !storeLinks) || relative == ".." || relative.startsWith("../") ||
            QDir::isAbsolutePath(relative) || !sourceUpdatePathIsSafe(relative) ||
            relative.contains('*') || relative.contains('?') || relative.startsWith('@') ||
            info.absoluteFilePath() == target.absoluteFilePath()) {
            *error = QCoreApplication::translate("archivebackend_sourceupdate",
                                                 "Update source unavailable: %1")
                         .arg(source);
            return false;
        }
        if (info.isDir() && !info.isSymLink()) {
            const QString nested =
                QDir(info.canonicalFilePath()).relativeFilePath(target.absoluteFilePath());
            if (nested == "." ||
                (nested != ".." && !nested.startsWith("../") && !QDir::isAbsolutePath(nested))) {
                *error = QCoreApplication::translate("archivebackend_sourceupdate",
                                                     "The archive is inside the selected folder.");
                return false;
            }
            QDirIterator iterator(info.absoluteFilePath(),
                                  QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden |
                                      QDir::System,
                                  QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                iterator.next();
                if (iterator.fileInfo().isSymLink() && !storeLinks) {
                    *error = QCoreApplication::translate("archivebackend_sourceupdate",
                                                         "Sources must not contain links: %1")
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
        emit finished(false, QCoreApplication::translate("archivebackend_sourceupdate",
                                                         "Another operation is still running."));
        return;
    }
    lastResult_ = E_FAIL;
    if (!supportsArchiveUpdate(format) || !sourceUpdatePathIsSafe(entry) ||
        !QFileInfo(archive).isFile() || !QFileInfo(QDir(sourceRoot).filePath(entry)).isFile() ||
        output.isEmpty() || QFileInfo::exists(output)) {
        emit finished(false, QCoreApplication::translate("archivebackend_sourceupdate",
                                                         "Invalid archive update file or path."));
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
        emit finished(false, QCoreApplication::translate("archivebackend_sourceupdate",
                                                         "Another operation is still running."));
        return;
    }
    lastResult_ = E_FAIL;
    const QFileInfo input(archive);
    const QFileInfo result(output);
    const QFileInfo root(sourceRoot);
    if (!supportsArchiveUpdate(format) || !input.isFile() || input.isSymLink() || result.exists() ||
        result.isSymLink() || !root.isDir() || root.isSymLink() || entries.isEmpty()) {
        emit finished(false, QCoreApplication::translate("archivebackend_sourceupdate",
                                                         "Invalid archive add request."));
        return;
    }
    for (const QString &entry : entries) {
        if (!sourceUpdatePathIsSafe(entry) || entry.contains('*') || entry.contains('?') ||
            entry.startsWith('@')) {
            emit finished(false, QCoreApplication::translate("archivebackend_sourceupdate",
                                                             "Unsafe archive add path."));
            return;
        }
        const QFileInfo staged(QDir(root.absoluteFilePath()).filePath(entry));
        if ((!staged.isFile() && !staged.isDir()) || staged.isSymLink() ||
            // QFileInfo canonical paths use Qt separators on Windows too.
            !staged.canonicalFilePath().startsWith(root.canonicalFilePath() + '/')) {
            emit finished(false,
                          QCoreApplication::translate("archivebackend_sourceupdate",
                                                      "The added file has not been prepared."));
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
        emit finished(false, QCoreApplication::translate("archivebackend_sourceupdate",
                                                         "Another operation is still running."));
        return;
    }
    lastResult_ = E_FAIL;
    const QFileInfo input(archive);
    const QFileInfo result(output);
    if (!supportsArchiveUpdate(format) || !input.isFile() || input.isSymLink() || result.exists() ||
        paths.isEmpty()) {
        emit finished(false, QCoreApplication::translate("archivebackend_sourceupdate",
                                                         "Invalid archive mutation request."));
        return;
    }
    for (const QString &path : paths) {
        if (!sourceUpdatePathIsSafe(path) || path.contains('*') || path.contains('?') ||
            path.startsWith('@')) {
            emit finished(false, QCoreApplication::translate("archivebackend_sourceupdate",
                                                             "Unsafe archive path."));
            return;
        }
    }
    if ((operation == SourceMutation::Rename && paths.size() != 2) ||
        (operation == SourceMutation::CreateFolder && paths.size() != 1) ||
        (operation != SourceMutation::CreateFolder && !sourceRoot.isEmpty())) {
        emit finished(false, QCoreApplication::translate("archivebackend_sourceupdate",
                                                         "Invalid archive mutation plan."));
        return;
    }
    if (operation == SourceMutation::CreateFolder &&
        !QFileInfo(QDir(sourceRoot).filePath(paths.front())).isDir()) {
        emit finished(false, QCoreApplication::translate("archivebackend_sourceupdate",
                                                         "The new folder has not been prepared."));
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
    prepareArchiveCopy(input.absoluteFilePath(), createTemporary_,
                       [this, args, sourceRoot] { start(Job::SourceMutation, args, sourceRoot); });
}

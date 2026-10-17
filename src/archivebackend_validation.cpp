#include "CPP/7zip/UI/Common/ExtractingFilePath.h"
#include "Common/Wildcard.h"
#include "archivebackend.h"
#include "qtlanguageservices.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

bool ArchiveBackend::safePath(const QString &path) {
    QString normalized = QDir::fromNativeSeparators(path);
    normalized.replace('\\', '/');
    if (normalized.isEmpty() || normalized.startsWith('/') ||
        QRegularExpression(R"(^[A-Za-z]:)").match(normalized).hasMatch())
        return false;
    for (const QString &part : normalized.split('/')) {
        const QString base = part.section('.', 0, 0).toUpper();
        if (part.isEmpty() || part == ".." || part == "." || part.endsWith('.') ||
            part.endsWith(' ') || part.contains(':') ||
            QRegularExpression(R"([\x00-\x1f])").match(part).hasMatch() || base == "CON" ||
            base == "PRN" || base == "AUX" || base == "NUL" ||
            QRegularExpression(R"(^(COM[1-9]|LPT[1-9])$)").match(base).hasMatch())
            return false;
    }
    return true;
}

bool ArchiveBackend::validateExtractionOutput(const QString &destination, const QString &path,
                                              bool directory, QString *error) {
    const QString root = QDir::cleanPath(QFileInfo(destination).absoluteFilePath());
    const QString output = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    const QString relative = QDir(root).relativeFilePath(output);
    if ((relative == "." && !directory) || relative == ".." || relative.startsWith("../") ||
        QDir::isAbsolutePath(relative) || (relative != "." && !safePath(relative))) {
        *error = QtLocalText("Extraction path is outside the destination folder: %1",
                             "Путь извлечения находится вне папки назначения: %1")
                     .arg(path);
        return false;
    }
    QString candidate = output;
    bool leaf = true;
    while (true) {
        const QFileInfo existing(candidate);
#ifdef Q_OS_MACOS
        const bool systemAlias = candidate == "/var" || candidate == "/tmp" || candidate == "/etc";
#else
        const bool systemAlias = false;
#endif
        if ((existing.isSymLink() && !systemAlias) ||
            (existing.exists() && (leaf && !directory ? !existing.isFile() : !existing.isDir()))) {
            *error = QtLocalText("Destination path is occupied by a file or link: %1",
                                 "Путь назначения занят файлом или ссылкой: %1")
                         .arg(candidate);
            return false;
        }
        const QString parent = existing.absolutePath();
        if (parent == candidate)
            break;
        candidate = parent;
        leaf = false;
    }
    return true;
}

bool ArchiveBackend::validateExtractionStreamOutput(const QString &destination, const QString &path,
                                                    QString *error) {
    const QString output = QDir::fromNativeSeparators(path);
    const int colon = output.lastIndexOf(':');
    if (colon > output.lastIndexOf('/')) {
        const QString name = output.mid(colon + 1);
        if (!safePath(name) || name.contains('/')) {
            *error = QtLocalText("Unsafe alternate stream name",
                                 QStringLiteral("Небезопасное имя потока"));
            return false;
        }
        const QString base = output.left(colon);
        return validateExtractionOutput(destination, base, QFileInfo(base).isDir(), error);
    }
    // Extracting from a source ADS folder removes the base and colon.
    return validateExtractionOutput(destination, output, false, error);
}

bool ArchiveBackend::validateExtraction(const QString &destination, const QStringList &selected,
                                        const ExtractionOptions &options, QString *error,
                                        bool nativeSource, const QString &sourceFolder) const {
    if (destination.trimmed().isEmpty()) {
        *error = QtLocalText("Specify a destination folder.",
                             QStringLiteral("Укажите папку назначения."));
        return false;
    }
    QFileInfo target(destination);
    QString ancestor = QDir::cleanPath(target.absoluteFilePath());
    while (true) {
        const QFileInfo part(ancestor);
        // macOS exposes temporary directories through /var or /tmp, which are system aliases
        // for /private/var and /private/tmp. Archive previews use QTemporaryDir beneath them.
#ifdef Q_OS_MACOS
        const bool systemAlias = ancestor == QStringLiteral("/var") ||
                                 ancestor == QStringLiteral("/tmp") ||
                                 ancestor == QStringLiteral("/etc");
#else
        const bool systemAlias = false;
#endif
        if (part.isSymLink() && !systemAlias) {
            *error = QtLocalText("Destination path contains a link: %1",
                                 QStringLiteral("Путь назначения содержит ссылку: %1"))
                         .arg(ancestor);
            return false;
        }
        if (part.exists() && !part.isDir()) {
            *error = QtLocalText("Destination path is occupied by a file: %1",
                                 QStringLiteral("Путь назначения занят файлом: %1"))
                         .arg(ancestor);
            return false;
        }
        if (ancestor == QDir::rootPath())
            break;
        const QString parent = QFileInfo(ancestor).absolutePath();
        if (parent == ancestor)
            break;
        ancestor = parent;
    }

    QSet<QString> names;
    QSet<QString> files;
    QSet<QString> selectedPaths;
    for (QString path : selected) {
        path.replace('\\', '/');
        selectedPaths.insert(path);
    }
    QString currentFolder = options.currentFolder;
    currentFolder.replace('\\', '/');
    QString boundFolder = sourceFolder;
    boundFolder.replace('\\', '/');
    for (const ArchiveEntry &entry : entries_) {
        QString entryPath = entry.path;
        entryPath.replace('\\', '/');
        if (!selectedPaths.isEmpty() && !selectedPaths.contains(entryPath))
            continue;
        QString boundedPath = entryPath;
        if (options.pathMode == ExtractionOptions::PathMode::AbsolutePaths &&
            QDir::isAbsolutePath(entryPath)) {
            // Reject traversal before cleanPath can erase it. The source path
            // algorithm keeps absolute prefixes only in kAbsPaths mode.
            if (entryPath.split('/').contains("..")) {
                *error = QtLocalText("Archive contains an unsafe path: %1",
                                     "Архив содержит небезопасный путь: %1")
                             .arg(entry.path);
                return false;
            }
            if (!validateExtractionOutput(destination, entryPath, entry.directory, error))
                return false;
            boundedPath = QDir(target.absoluteFilePath()).relativeFilePath(entryPath);
        }
        const bool streamFolder = options.pathMode == ExtractionOptions::PathMode::CurrentPaths &&
                                  currentFolder.endsWith(':');
        const bool stream = streamFolder && !entry.directory &&
                            entry.properties.value(QStringLiteral("Alternate Stream")) == "+";
        const QString base = streamFolder ? currentFolder.chopped(1) : QString();
        const QString streamName = entryPath.mid(currentFolder.size());
        // Original ArchiveExtractCallback::CorrectPathParts removes the base
        // and colon when extracting from an ADS folder. Validate that exact
        // output name; ordinary colon paths remain refused.
        const bool safeStream = stream && entryPath.startsWith(currentFolder) && safePath(base) &&
                                safePath(streamName) && !streamName.contains('/') &&
                                !streamName.contains('\\');
        bool nativeStream = false;
#ifdef Q_OS_WIN
        const int colon = boundedPath.indexOf(':');
        nativeStream = nativeSource && !streamFolder && !entry.directory && !entry.link &&
                       entry.properties.value(QStringLiteral("Alternate Stream")) == "+" &&
                       colon > 0 && safePath(boundedPath.left(colon)) &&
                       safePath(boundedPath.mid(colon + 1)) &&
                       !boundedPath.mid(colon + 1).contains('/');
#endif
        QString relative = boundedPath;
        if (options.pathMode == ExtractionOptions::PathMode::CurrentPaths &&
            !currentFolder.isEmpty()) {
            const QString prefix = currentFolder + (safeStream ? QString() : QString('/'));
            if (!relative.startsWith(prefix)) {
                *error =
                    QtLocalText("Item is outside the current archive folder: %1",
                                QStringLiteral("Объект находится вне текущей папки архива: %1"))
                        .arg(entry.path);
                return false;
            }
            relative.remove(0, prefix.size());
        } else if (options.pathMode == ExtractionOptions::PathMode::NoPaths)
            relative = relative.section('/', -1);
        // Only the bound source folder guarantees removal of this exact
        // archive prefix. A plain console extraction still uses the full name.
        const bool sourceRelative = options.pathMode == ExtractionOptions::PathMode::CurrentPaths &&
                                    !boundFolder.isEmpty() && boundFolder == currentFolder;
        QStringList checkedParts = (sourceRelative ? relative : boundedPath).split('/');
        checkedParts.removeAll(QStringLiteral("."));
        const bool safeRelative =
            (entry.directory && checkedParts.isEmpty()) || safePath(checkedParts.join('/'));
        if ((!safeStream && !nativeStream && !safeRelative) || (entry.link && !nativeSource) ||
            (streamFolder && !safeStream)) {
            *error = QtLocalText("Archive contains an unsafe path or link: %1",
                                 QStringLiteral("Архив содержит небезопасный путь или ссылку: %1"))
                         .arg(entry.path);
            return false;
        }
        if (!safeStream && !nativeStream) {
            UStringVector parts;
            SplitPathToParts(UString(relative.toStdWString().c_str()), parts);
            Correct_FsPath(false, false, parts, entry.directory);
            relative = QString::fromWCharArray(MakePathFromParts(parts).Ptr());
            relative.replace('\\', '/');
        }
        const QString key = relative.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (names.contains(key)) {
            *error = QtLocalText("Archive contains conflicting names: %1",
                                 QStringLiteral("Архив содержит конфликт имён: %1"))
                         .arg(entry.path);
            return false;
        }
        names.insert(key);
        if (!entry.directory || entry.link)
            files.insert(key);
        QString candidate = target.absoluteFilePath();
        const QStringList parts = relative.split('/');
        for (int index = 0; index < parts.size(); ++index) {
            const QString &part = parts.at(index);
            candidate = QDir(candidate).filePath(part);
            const QFileInfo existing(candidate);
            const bool finalFile = index == parts.size() - 1 && !entry.directory;
            if (existing.isSymLink() ||
                (existing.exists() && (finalFile ? !existing.isFile() : !existing.isDir()))) {
                *error = QtLocalText("Destination path is occupied by a file or link: %1",
                                     QStringLiteral("Путь назначения занят файлом или ссылкой: %1"))
                             .arg(candidate);
                return false;
            }
        }
        if (QFileInfo(candidate).exists() && !entry.directory &&
            options.overwriteMode == ExtractionOptions::OverwriteMode::RejectExisting) {
            *error = QtLocalText("Already exists in the destination folder: %1",
                                 QStringLiteral("В папке назначения уже существует: %1"))
                         .arg(entry.path);
            return false;
        }
    }
    for (const QString &name : names) {
        QString parent = name;
        while (parent.contains('/')) {
            parent = parent.section('/', 0, -2);
            if (files.contains(parent)) {
                *error = QtLocalText("Archive contains a file/folder conflict: %1",
                                     QStringLiteral("Архив содержит конфликт файла и папки: %1"))
                             .arg(name);
                return false;
            }
        }
    }
    return true;
}

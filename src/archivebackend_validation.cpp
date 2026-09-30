#include "archivebackend.h"

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

bool ArchiveBackend::validateExtraction(const QString &destination, const QStringList &selected,
                                        const ExtractionOptions &options, QString *error) const {
    if (destination.trimmed().isEmpty()) {
        *error = QStringLiteral("Укажите папку назначения.");
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
            *error = QStringLiteral("Путь назначения содержит ссылку: %1").arg(ancestor);
            return false;
        }
        if (part.exists() && !part.isDir()) {
            *error = QStringLiteral("Путь назначения занят файлом: %1").arg(ancestor);
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
    for (const ArchiveEntry &entry : entries_) {
        if (!selected.isEmpty() && !selected.contains(entry.path))
            continue;
        if (!safePath(entry.path) || entry.link) {
            *error =
                QStringLiteral("Архив содержит небезопасный путь или ссылку: %1").arg(entry.path);
            return false;
        }
        QString relative = entry.path;
        relative.replace('\\', '/');
        if (options.pathMode == ExtractionOptions::PathMode::CurrentPaths &&
            !options.currentFolder.isEmpty()) {
            const QString prefix = options.currentFolder + '/';
            if (!relative.startsWith(prefix)) {
                *error =
                    QStringLiteral("Объект находится вне текущей папки архива: %1").arg(entry.path);
                return false;
            }
            relative.remove(0, prefix.size());
        } else if (options.pathMode == ExtractionOptions::PathMode::NoPaths)
            relative = relative.section('/', -1);
        const QString key = relative.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (names.contains(key)) {
            *error = QStringLiteral("Архив содержит конфликт имён: %1").arg(entry.path);
            return false;
        }
        names.insert(key);
        if (!entry.directory)
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
                *error =
                    QStringLiteral("Путь назначения занят файлом или ссылкой: %1").arg(candidate);
                return false;
            }
        }
        if (QFileInfo(candidate).exists() && !entry.directory &&
            options.overwriteMode == ExtractionOptions::OverwriteMode::RejectExisting) {
            *error = QStringLiteral("В папке назначения уже существует: %1").arg(entry.path);
            return false;
        }
    }
    for (const QString &name : names) {
        QString parent = name;
        while (parent.contains('/')) {
            parent = parent.section('/', 0, -2);
            if (files.contains(parent)) {
                *error = QStringLiteral("Архив содержит конфликт файла и папки: %1").arg(name);
                return false;
            }
        }
    }
    return true;
}

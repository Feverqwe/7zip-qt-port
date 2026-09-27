#include "archivebackend.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

void ArchiveBackend::create(const QString &archive, const QStringList &sources,
                            const QString &password, bool encryptHeaders,
                            const CompressionOptions &options) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    const QString suffix = QFileInfo(archive).suffix().toLower();
    if ((suffix != "7z" && suffix != "zip") || sources.isEmpty()) {
        emit finished(false, QStringLiteral("Выберите файлы или папку и имя архива .7z или .zip."));
        return;
    }
    const QStringList methods = suffix == "7z"
                                    ? QStringList{"LZMA2", "LZMA", "PPMd", "BZip2"}
                                    : QStringList{"Deflate", "Deflate64", "BZip2", "LZMA"};
    if (!QList<int>{0, 1, 3, 5, 7, 9}.contains(options.level) || options.threads < 0 ||
        options.threads > 256 || (!options.method.isEmpty() && !methods.contains(options.method))) {
        emit finished(false,
                      QStringLiteral("Недопустимые параметры сжатия для выбранного формата."));
        return;
    }
    if (suffix == "zip") {
        for (const QChar character : password) {
            if (character.unicode() > 127) {
                emit finished(false, QStringLiteral("Для ZIP AES используйте пароль из ASCII: "
                                                    "эта версия 7zz не принимает Unicode пароль."));
                return;
            }
        }
    }
    const QFileInfo target(archive);
    if (!QFileInfo(target.absolutePath()).isDir() || target.exists() || target.isSymLink()) {
        emit finished(false,
                      QStringLiteral("Папка назначения не найдена или архив уже существует: %1")
                          .arg(target.absoluteFilePath()));
        return;
    }
    QString root = QFileInfo(sources.first()).absolutePath();
    while (!root.isEmpty()) {
        bool containsAll = true;
        for (const QString &source : sources) {
            const QString relative =
                QDir(root).relativeFilePath(QFileInfo(source).absoluteFilePath());
            if (relative == ".." || relative.startsWith("../") || QDir::isAbsolutePath(relative)) {
                containsAll = false;
                break;
            }
        }
        if (containsAll)
            break;
        const QString parent = QFileInfo(root).absolutePath();
        if (parent == root)
            break;
        root = parent;
    }
    QStringList arguments = {"a", "-y", "-bsp1", "-bb1", "-t" + suffix, "--"};
    arguments.insert(5, "-mx=" + QString::number(options.level));
    if (options.level != 0 && !options.method.isEmpty())
        arguments.insert(5, "-m0=" + options.method);
    if (options.threads > 0)
        arguments.insert(5, "-mmt=" + QString::number(options.threads));
    if (suffix == "7z" && !options.solid)
        arguments.insert(5, "-ms=off");
    if (!password.isEmpty()) {
        arguments.insert(5, "-p");
        arguments.insert(5, suffix == "7z" ? (encryptHeaders ? "-mhe=on" : "-mhe=off")
                                           : "-mem=AES256");
    }
    for (const QString &source : sources) {
        if (!QFileInfo::exists(source)) {
            emit finished(false, QStringLiteral("Файл или папка не найдены: %1").arg(source));
            return;
        }
        const QString relative = QDir(root).relativeFilePath(QFileInfo(source).absoluteFilePath());
        if (relative == ".." || relative.startsWith("../") || QDir::isAbsolutePath(relative)) {
            emit finished(false, QStringLiteral("Файлы должны находиться на одном диске."));
            return;
        }
        arguments << relative;
    }
    for (const QString &source : sources) {
        const QFileInfo info(source);
        if (info.isDir()) {
            const QString relative =
                QDir(info.canonicalFilePath()).relativeFilePath(target.absoluteFilePath());
            if (relative == "." || (relative != ".." && !relative.startsWith("../") &&
                                    !QDir::isAbsolutePath(relative))) {
                emit finished(false,
                              QStringLiteral("Нельзя сохранять архив внутри выбранной папки."));
                return;
            }
        }
    }
    createTarget_ = target.absoluteFilePath();
    createTemporary_ = target.absolutePath() + "/.sevenzip-" +
                       QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    arguments.insert(arguments.indexOf("--") + 1, createTemporary_);
    pendingPassword_ = password;
    start(Job::Create, arguments, root);
}

void ArchiveBackend::add(const QString &archive, const QStringList &sources) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return;
    }
    if (encrypted_) {
        emit finished(false, QStringLiteral("Добавление в защищённый архив пока недоступно."));
        return;
    }
    const QFileInfo target(archive);
    if (!listed_ || target.absoluteFilePath() != archive_ || !target.isFile() ||
        target.isSymLink() || sources.isEmpty()) {
        emit finished(false, QStringLiteral("Откройте существующий архив и выберите источники."));
        return;
    }
    const QString suffix = target.suffix().toLower();
    if (suffix != "7z" && suffix != "zip") {
        emit finished(false, QStringLiteral("Поддерживаются только архивы .7z и .zip."));
        return;
    }
    QString root = QFileInfo(sources.first()).absolutePath();
    while (!root.isEmpty()) {
        bool containsAll = true;
        for (const QString &source : sources) {
            const QString relative =
                QDir(root).relativeFilePath(QFileInfo(source).absoluteFilePath());
            if (relative == ".." || relative.startsWith("../") || QDir::isAbsolutePath(relative)) {
                containsAll = false;
                break;
            }
        }
        if (containsAll)
            break;
        const QString parent = QFileInfo(root).absolutePath();
        if (parent == root)
            break;
        root = parent;
    }
    QSet<QString> existing;
    for (const ArchiveEntry &entry : entries_)
        existing.insert(entry.path.normalized(QString::NormalizationForm_C).toCaseFolded());
    QSet<QString> incoming;
    QStringList sourceNames;
    for (const QString &source : sources) {
        const QFileInfo info(source);
        const QString relative = QDir(root).relativeFilePath(info.absoluteFilePath());
        if (!info.exists() || info.isSymLink() || relative == ".." || relative.startsWith("../") ||
            QDir::isAbsolutePath(relative) ||
            info.absoluteFilePath() == target.absoluteFilePath()) {
            emit finished(false, QStringLiteral("Недоступный источник: %1").arg(source));
            return;
        }
        QStringList paths = {relative};
        if (info.isDir()) {
            const QString destination =
                QDir(info.canonicalFilePath()).relativeFilePath(target.absoluteFilePath());
            if (destination == "." || (destination != ".." && !destination.startsWith("../") &&
                                       !QDir::isAbsolutePath(destination))) {
                emit finished(false, QStringLiteral("Архив находится внутри выбранной папки."));
                return;
            }
            QDirIterator iterator(info.absoluteFilePath(),
                                  QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden |
                                      QDir::System,
                                  QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                iterator.next();
                if (iterator.fileInfo().isSymLink()) {
                    emit finished(false, QStringLiteral("Источники не должны содержать ссылки: %1")
                                             .arg(iterator.filePath()));
                    return;
                }
                paths << QDir(root).relativeFilePath(iterator.filePath());
            }
        }
        for (const QString &path : paths) {
            if (!safePath(path)) {
                emit finished(false, QStringLiteral("Небезопасное имя источника: %1").arg(path));
                return;
            }
            const QString key = path.normalized(QString::NormalizationForm_C).toCaseFolded();
            if (existing.contains(key) || incoming.contains(key)) {
                emit finished(false,
                              QStringLiteral("Имя уже есть в архиве или выборе: %1").arg(path));
                return;
            }
            incoming.insert(key);
        }
        sourceNames << relative;
    }
    createTarget_ = target.absoluteFilePath();
    createTemporary_ = target.absolutePath() + "/.sevenzip-" +
                       QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    addBackup_ = target.absolutePath() + "/.sevenzip-backup-" +
                 QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    if (!QFile::copy(createTarget_, createTemporary_)) {
        QFile::remove(createTemporary_);
        emit finished(false, QStringLiteral("Не удалось подготовить копию архива для записи."));
        return;
    }
    QStringList arguments = {"a", "-y", "-bsp1", "-bb1", "-t" + suffix, "--", createTemporary_};
    arguments += sourceNames;
    start(Job::Add, arguments, root);
}

bool ArchiveBackend::prepareMutation(const QString &archive) {
    if (busy()) {
        emit finished(false, QStringLiteral("Другая операция ещё выполняется."));
        return false;
    }
    const QFileInfo target(archive);
    const QString suffix = target.suffix().toLower();
    if (!listed_ || archive_ != target.absoluteFilePath() || !target.isFile() ||
        target.isSymLink() || encrypted_ || (suffix != "7z" && suffix != "zip")) {
        emit finished(false, QStringLiteral("Откройте незашифрованный архив .7z или .zip."));
        return false;
    }
    createTarget_ = target.absoluteFilePath();
    createTemporary_ = target.absolutePath() + "/.sevenzip-" +
                       QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    addBackup_ = target.absolutePath() + "/.sevenzip-backup-" +
                 QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    if (!QFile::copy(createTarget_, createTemporary_)) {
        QFile::remove(createTemporary_);
        emit finished(false, QStringLiteral("Не удалось подготовить копию архива для записи."));
        return false;
    }
    return true;
}

void ArchiveBackend::remove(const QString &archive, const QStringList &paths) {
    if (paths.isEmpty())
        return;
    QSet<QString> known;
    for (const ArchiveEntry &entry : entries_)
        known.insert(entry.path);
    for (const QString &path : paths) {
        if (!known.contains(path) || !safePath(path) || path.contains('*') || path.contains('?') ||
            path.startsWith('@')) {
            emit finished(false, QStringLiteral("Объект архива не найден: %1").arg(path));
            return;
        }
    }
    if (!prepareMutation(archive))
        return;
    QStringList arguments = {"d", "-y", "-bsp1", "-bb1", "--", createTemporary_};
    arguments += paths;
    start(Job::Delete, arguments);
}

void ArchiveBackend::renameEntry(const QString &archive, const QString &oldPath,
                                 const QString &newPath) {
    bool found = false;
    const QString key = newPath.normalized(QString::NormalizationForm_C).toCaseFolded();
    for (const ArchiveEntry &entry : entries_) {
        if (entry.path == oldPath && !entry.directory)
            found = true;
        const QString existing = entry.path.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (existing == key || existing.startsWith(key + '/')) {
            emit finished(false, QStringLiteral("Имя уже есть в архиве."));
            return;
        }
    }
    if (!found || !safePath(newPath) || oldPath.contains('*') || oldPath.contains('?') ||
        oldPath.startsWith('@') || newPath.contains('*') || newPath.contains('?') ||
        newPath.startsWith('@') || oldPath.section('/', 0, -2) != newPath.section('/', 0, -2)) {
        emit finished(false, QStringLiteral("Переименование этого объекта недоступно."));
        return;
    }
    if (!prepareMutation(archive))
        return;
    start(Job::Rename, {"rn", "-y", "-bsp1", "-bb1", "--", createTemporary_, oldPath, newPath});
}

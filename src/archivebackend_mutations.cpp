#include "7zip/Archive/Common/ItemNameUtils.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "7zip/UI/GUI/CompressDialogRes.h"
#include "CPP/7zip/UI/Common/Update.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "CPP/7zip/UI/GUI/CompressDialog.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "archivebackend.h"
#include "archivesfxmodule.h"
#include "qtarchivecreationlifecycle.h"
#include "qtlanguageservices.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

namespace {
QString mutationPath(const QString &path) {
    UString native(path.toStdWString().c_str());
    NArchive::NItemName::ReplaceSlashes_OsToUnix(native);
    return QString::fromWCharArray(native.Ptr());
}
} // namespace

void ArchiveBackend::add(const QString &archive, const QStringList &sources) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    if (!restoreListedArchive(archive)) {
        emit finished(false, QtLocalText("Open the archive and inspect its contents first.",
                                         "Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    if (encrypted_) {
        emit finished(false, QtLocalText("Adding to a protected archive is currently unavailable.",
                                         "Добавление в защищённый архив пока недоступно."));
        return;
    }
    const QFileInfo target(archive);
    if (!listed_ || target.absoluteFilePath() != archive_ || !target.isFile() ||
        target.isSymLink() || sources.isEmpty()) {
        emit finished(false, QtLocalText("Open an existing archive and select sources.",
                                         "Откройте существующий архив и выберите источники."));
        return;
    }
    const QString suffix = target.suffix().toLower();
    if (suffix != "7z" && suffix != "zip") {
        emit finished(false, QtLocalText("Only .7z and .zip archives are supported.",
                                         "Поддерживаются только архивы .7z и .zip."));
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
            emit finished(
                false,
                QtLocalText("Source unavailable: %1", "Недоступный источник: %1").arg(source));
            return;
        }
        QStringList paths = {relative};
        if (info.isDir()) {
            const QString destination =
                QDir(info.canonicalFilePath()).relativeFilePath(target.absoluteFilePath());
            if (destination == "." || (destination != ".." && !destination.startsWith("../") &&
                                       !QDir::isAbsolutePath(destination))) {
                emit finished(false, QtLocalText("The archive is inside the selected folder.",
                                                 "Архив находится внутри выбранной папки."));
                return;
            }
            QDirIterator iterator(info.absoluteFilePath(),
                                  QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden |
                                      QDir::System,
                                  QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                iterator.next();
                if (iterator.fileInfo().isSymLink()) {
                    emit finished(false, QtLocalText("Sources must not contain links: %1",
                                                     "Источники не должны содержать ссылки: %1")
                                             .arg(iterator.filePath()));
                    return;
                }
                paths << QDir(root).relativeFilePath(iterator.filePath());
            }
        }
        for (const QString &path : paths) {
            if (!safePath(path)) {
                emit finished(
                    false, QtLocalText("Unsafe source name: %1", "Небезопасное имя источника: %1")
                               .arg(path));
                return;
            }
            const QString key = path.normalized(QString::NormalizationForm_C).toCaseFolded();
            if (existing.contains(key) || incoming.contains(key)) {
                emit finished(false,
                              QtLocalText("Name already exists in the archive or selection: %1",
                                          "Имя уже есть в архиве или выборе: %1")
                                  .arg(path));
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
    QStringList arguments = {"a", "-y", "-bsp1", "-bb1", "-t" + suffix, "--", createTemporary_};
    arguments += sourceNames;
    prepareArchiveCopy(createTarget_, createTemporary_,
                       [this, arguments, root] { start(Job::Add, arguments, root); });
}

bool ArchiveBackend::prepareMutation(const QString &archive) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return false;
    }
    const QFileInfo target(archive);
    const QString suffix = target.suffix().toLower();
    if (!restoreListedArchive(archive) || !target.isFile() || target.isSymLink() || encrypted_ ||
        (suffix != "7z" && suffix != "zip")) {
        emit finished(false, QtLocalText("Open an unencrypted .7z or .zip archive.",
                                         "Откройте незашифрованный архив .7z или .zip."));
        return false;
    }
    createTarget_ = target.absoluteFilePath();
    createTemporary_ = target.absolutePath() + "/.sevenzip-" +
                       QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    addBackup_ = target.absolutePath() + "/.sevenzip-backup-" +
                 QUuid::createUuid().toString(QUuid::WithoutBraces) + "." + suffix;
    return true;
}

void ArchiveBackend::remove(const QString &archive, const QStringList &paths) {
    if (paths.isEmpty())
        return;
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    if (!restoreListedArchive(archive)) {
        emit finished(false, QtLocalText("Open the archive and inspect its contents first.",
                                         "Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    QSet<QString> known;
    for (const ArchiveEntry &entry : entries_)
        known.insert(mutationPath(entry.path));
    for (const QString &path : paths) {
        if (!known.contains(mutationPath(path)) || !safePath(path) || path.contains('*') ||
            path.contains('?') || path.startsWith('@')) {
            emit finished(
                false,
                QtLocalText("Archive item not found: %1", "Объект архива не найден: %1").arg(path));
            return;
        }
    }
    if (!prepareMutation(archive))
        return;
    QStringList arguments = {"d", "-y", "-bsp1", "-bb1", "--", createTemporary_};
    arguments += paths;
    prepareArchiveCopy(createTarget_, createTemporary_,
                       [this, arguments] { start(Job::Delete, arguments); });
}

void ArchiveBackend::renameEntry(const QString &archive, const QString &oldName,
                                 const QString &newName) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    const QString oldPath = mutationPath(oldName);
    const QString newPath = mutationPath(newName);
    if (!restoreListedArchive(archive)) {
        emit finished(false, QtLocalText("Open the archive and inspect its contents first.",
                                         "Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    bool found = false;
    bool directory = false;
    for (const ArchiveEntry &entry : entries_) {
        const QString path = mutationPath(entry.path);
        if (path == oldPath) {
            found = true;
            directory = entry.directory;
        } else if (path.startsWith(oldPath + '/')) {
            found = true;
            directory = true;
        }
    }
    const QString oldPrefix = oldPath + '/';
    QSet<QString> existing;
    QSet<QString> renamed;
    for (const ArchiveEntry &entry : entries_) {
        const QString entryPath = mutationPath(entry.path);
        const bool member = entryPath == oldPath || (directory && entryPath.startsWith(oldPrefix));
        const QString path = member ? newPath + entryPath.mid(oldPath.size()) : entryPath;
        const QString key = path.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (member) {
            if (!safePath(entry.path) || entry.link) {
                emit finished(
                    false, QtLocalText("Unsafe archive item: %1", "Небезопасный объект архива: %1")
                               .arg(entry.path));
                return;
            }
            if (renamed.contains(key)) {
                emit finished(false, QtLocalText("The name already exists in the archive.",
                                                 "Имя уже есть в архиве."));
                return;
            }
            renamed.insert(key);
        } else
            existing.insert(key);
    }
    if (!found || !safePath(newPath) || oldPath.contains('*') || oldPath.contains('?') ||
        oldPath.startsWith('@') || newPath.contains('*') || newPath.contains('?') ||
        newPath.startsWith('@') || oldPath == newPath ||
        oldPath.section('/', 0, -2) != newPath.section('/', 0, -2)) {
        emit finished(false, QtLocalText("Renaming this item is unavailable.",
                                         "Переименование этого объекта недоступно."));
        return;
    }
    const QString targetKey = newPath.normalized(QString::NormalizationForm_C).toCaseFolded();
    for (const QString &key : existing) {
        if (renamed.contains(key) ||
            (directory && (key == targetKey || key.startsWith(targetKey + '/')))) {
            emit finished(false, QtLocalText("The name already exists in the archive.",
                                             "Имя уже есть в архиве."));
            return;
        }
    }
    if (!prepareMutation(archive))
        return;
    prepareArchiveCopy(createTarget_, createTemporary_, [this, oldPath, newPath] {
        start(Job::Rename, {"rn", "-y", "-bsp1", "-bb1", "--", createTemporary_, oldPath, newPath});
    });
}

void ArchiveBackend::replaceEntry(const QString &archive, const QString &entry,
                                  const QString &sourceRoot) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         "Другая операция ещё выполняется."));
        return;
    }
    lastResult_ = E_FAIL;
    if (!restoreListedArchive(archive)) {
        emit finished(false, QtLocalText("Open the archive and inspect its contents first.",
                                         "Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    bool found = false;
    for (const ArchiveEntry &item : entries_)
        found |= item.path == entry && !item.directory && !item.link;
    if (!found || !safePath(entry) || entry.contains('*') || entry.contains('?') ||
        entry.startsWith('@')) {
        emit finished(false, QtLocalText("This archive file cannot be replaced.",
                                         "Этот файл архива нельзя заменить."));
        return;
    }
    const QDir root(sourceRoot);
    const QFileInfo source(root.filePath(entry));
    if (!root.exists() || !source.isFile() || source.isSymLink() ||
        !source.canonicalFilePath().startsWith(root.canonicalPath() + '/')) {
        emit finished(false, QtLocalText("Modified file not found.", "Изменённый файл не найден."));
        return;
    }
    QString part = root.absolutePath();
    for (const QString &component : entry.split('/')) {
        part = QDir(part).filePath(component);
        if (QFileInfo(part).isSymLink()) {
            emit finished(false, QtLocalText("Links in temporary files are not allowed.",
                                             "Ссылки во временном файле запрещены."));
            return;
        }
    }
    if (!prepareMutation(archive))
        return;
    replacementEntry_ = entry;
    replacementRoot_ = root.absolutePath();
    prepareArchiveCopy(createTarget_, createTemporary_, [this, entry] {
        start(Job::ReplaceDelete,
              {"d", "-y", "-bsp1", "-bb1", "-spd", "--", createTemporary_, entry});
    });
}

bool ArchiveBackend::publishProcessMutation() {
    if (!QFile::rename(createTarget_, addBackup_)) {
        QFile::remove(createTemporary_);
        emit finished(false, QtLocalText("Could not replace archive: %1",
                                         QStringLiteral("Не удалось заменить архив: %1"))
                                 .arg(createTarget_));
        return false;
    }
    if (!QFile::rename(createTemporary_, createTarget_)) {
        const bool restored = QFile::rename(addBackup_, createTarget_);
        QFile::remove(createTemporary_);
        emit finished(
            false, restored
                       ? QtLocalText("Could not save updated archive: %1",
                                     QStringLiteral("Не удалось сохранить обновлённый архив: %1"))
                             .arg(createTarget_)
                       : QtLocalText("Could not restore archive. Backup: %1",
                                     QStringLiteral("Не удалось восстановить архив. Копия: %1"))
                             .arg(addBackup_));
        return false;
    }
    QFile::remove(addBackup_);
    listed_ = false;
    entries_.clear();
    listedSourceArchives_.remove(createTarget_);
    return true;
}

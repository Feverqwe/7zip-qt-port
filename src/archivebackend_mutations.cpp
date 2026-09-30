#include "CPP/7zip/UI/GUI/CompressDialog.h"
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
    const auto &formatInfo = NCompressDialog::GetFormatInfo(suffix);
    const QStringList methods = NCompressDialog::GetMethodNames(suffix);
    if (options.level < 0 || options.level > 9 ||
        !(formatInfo.LevelsMask & (UInt32(1) << options.level)) || options.threads < 0 ||
        (!options.method.isEmpty() && !methods.contains(options.method))) {
        emit finished(false,
                      QStringLiteral("Недопустимые параметры сжатия для выбранного формата."));
        return;
    }
    const QString method = options.method.isEmpty() ? (suffix == "7z" ? QStringLiteral("LZMA2")
                                                                      : QStringLiteral("Deflate"))
                                                    : options.method;
    const auto methodID =
        options.level == 0 ? NCompressDialog::kCopy : NCompressDialog::GetMethodID(method, suffix);
    const auto dictionary = NCompressDialog::SetDictionary2(methodID, UInt32(options.level));
    const UInt64 dict = options.dictionaryBytes ? options.dictionaryBytes
                        : dictionary.AutoDict   ? UInt64(dictionary.AutoDict)
                                                : UInt64(-1);
    const auto system = NCompressDialog::GetSystemInfo();
    UInt32 blockSizeLog = UInt32(-1);
    if (options.SolidIsSpecified) {
        if (options.SolidBlockSize == UInt64(-1))
            blockSizeLog = 64;
        else if (options.SolidBlockSize == 0)
            blockSizeLog = 0;
        else
            for (UInt32 i = 20; i <= 36; i++)
                if (options.SolidBlockSize == (UInt64(1) << i)) {
                    blockSizeLog = i;
                    break;
                }
    }
    const auto threadOptions = NCompressDialog::SetNumThreads2(
        suffix, methodID, UInt32(options.level), dict, blockSizeLog, system, options.MemUsage);
    if (options.threads != 0 && !threadOptions.Values.contains(UInt32(options.threads))) {
        emit finished(false,
                      QStringLiteral("Число потоков не подходит для выбранного метода сжатия."));
        return;
    }
    bool validSolid = !options.SolidIsSpecified;
    if (options.SolidIsSpecified) {
        const auto solid = NCompressDialog::SetSolidBlockSize2(
            suffix, UInt32(options.level), NCompressDialog::GetMethodID(method, suffix),
            UInt64(-1));
        for (UInt32 log : solid.Values) {
            NCompressDialog::CInfo info;
            NCompressDialog::OnOK(log, info);
            if (info.SolidBlockSize == options.SolidBlockSize) {
                validSolid = true;
                break;
            }
        }
    }
    if ((options.dictionaryBytes != 0 &&
         (!NCompressDialog::SetDictionary2(NCompressDialog::GetMethodID(method, suffix),
                                           UInt32(options.level))
               .Values.contains(options.dictionaryBytes))) ||
        (options.wordSize != 0 &&
         !NCompressDialog::SetOrder2(NCompressDialog::GetMethodID(method, suffix),
                                     UInt32(options.level))
              .Values.contains(options.wordSize)) ||
        !validSolid || (options.level == 0 && (options.dictionaryBytes || options.wordSize))) {
        emit finished(false,
                      QStringLiteral("Размер словаря, слова или непрерывного блока не подходит "
                                     "для выбранного метода сжатия."));
        return;
    }
    UInt64 decompressMemory;
    const UInt64 memUsage = NCompressDialog::GetMemoryUsage_Threads_Dict_DecompMem(
        suffix, methodID, UInt32(options.level),
        options.threads ? UInt32(options.threads) : threadOptions.AutoNumThreads, dict,
        blockSizeLog, decompressMemory);
    const UInt64 memLimit = NCompressDialog::Get_MemUse_Bytes(options.MemUsage, system);
    if ((options.MemUsage.IsDefined || system.RamSizeDefined) && memUsage != UInt64(-1) &&
        memUsage > memLimit) {
        emit finished(false, QStringLiteral("Для сжатия требуется %1 памяти, лимит — %2.")
                                 .arg(NCompressDialog::AddMemUsage(memUsage),
                                      NCompressDialog::AddMemUsage(memLimit)));
        return;
    }
    if ((!options.EncryptionMethod.isEmpty() &&
         options.EncryptionMethod.compare("AES256", Qt::CaseInsensitive) != 0) ||
        (suffix == "7z" && !options.EncryptionMethod.isEmpty()) ||
        (suffix == "zip" && encryptHeaders)) {
        emit finished(false,
                      QStringLiteral("Недопустимый метод шифрования для выбранного формата."));
        return;
    }
    const auto passwordError =
        CCompressDialog::CheckPassword(password, password, true, suffix, options.EncryptionMethod);
    if (passwordError != CCompressDialog::kPasswordOK) {
        emit finished(
            false,
            passwordError == CCompressDialog::kPasswordTooLong
                ? QStringLiteral("Пароль ZIP AES не должен превышать 99 символов.")
                : QStringLiteral("Для ZIP используйте пароль из ASCII без управляющих символов."));
        return;
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
    if (options.level != 0) {
        QString methodArgument = "-m0=" + method;
        if (options.dictionaryBytes)
            methodArgument +=
                (NCompressDialog::GetOrderMode(NCompressDialog::GetMethodID(method, suffix))
                     ? ":mem="
                     : ":d=") +
                QString::number(options.dictionaryBytes) + "b";
        if (options.wordSize)
            methodArgument +=
                (NCompressDialog::GetOrderMode(NCompressDialog::GetMethodID(method, suffix))
                     ? ":o="
                     : ":fb=") +
                QString::number(options.wordSize);
        arguments.insert(5, methodArgument);
    }
    if (options.threads > 0)
        arguments.insert(5, "-mmt=" + QString::number(options.threads));
    if (options.MemUsage.IsDefined)
        arguments.insert(5, "-mmemuse=" + QString::number(options.MemUsage.Val) +
                                (options.MemUsage.IsPercent ? "%" : "b"));
    if (options.SolidIsSpecified)
        arguments.insert(5, "-ms=" + QString::number(options.SolidBlockSize) + "b");
    if (!password.isEmpty()) {
        arguments.insert(5, "-p");
        if (suffix == "7z")
            arguments.insert(5, encryptHeaders ? "-mhe=on" : "-mhe=off");
        else if (!options.EncryptionMethod.isEmpty())
            arguments.insert(5, "-mem=" + options.EncryptionMethod);
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
    if (!restoreListedArchive(archive)) {
        emit finished(false, QStringLiteral("Сначала откройте архив и проверьте его содержимое."));
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
    if (!restoreListedArchive(archive) || !target.isFile() || target.isSymLink() || encrypted_ ||
        (suffix != "7z" && suffix != "zip")) {
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
    if (!restoreListedArchive(archive)) {
        emit finished(false, QStringLiteral("Сначала откройте архив и проверьте его содержимое."));
        return;
    }
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
    if (!restoreListedArchive(archive)) {
        emit finished(false, QStringLiteral("Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    bool found = false;
    bool directory = false;
    for (const ArchiveEntry &entry : entries_) {
        if (entry.path == oldPath) {
            found = true;
            directory = entry.directory;
        } else if (entry.path.startsWith(oldPath + '/')) {
            found = true;
            directory = true;
        }
    }
    const QString oldPrefix = oldPath + '/';
    QSet<QString> existing;
    QSet<QString> renamed;
    for (const ArchiveEntry &entry : entries_) {
        const bool member =
            entry.path == oldPath || (directory && entry.path.startsWith(oldPrefix));
        const QString path = member ? newPath + entry.path.mid(oldPath.size()) : entry.path;
        const QString key = path.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (member) {
            if (!safePath(entry.path) || entry.link) {
                emit finished(false,
                              QStringLiteral("Небезопасный объект архива: %1").arg(entry.path));
                return;
            }
            if (renamed.contains(key)) {
                emit finished(false, QStringLiteral("Имя уже есть в архиве."));
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
        emit finished(false, QStringLiteral("Переименование этого объекта недоступно."));
        return;
    }
    const QString targetKey = newPath.normalized(QString::NormalizationForm_C).toCaseFolded();
    for (const QString &key : existing) {
        if (renamed.contains(key) ||
            (directory && (key == targetKey || key.startsWith(targetKey + '/')))) {
            emit finished(false, QStringLiteral("Имя уже есть в архиве."));
            return;
        }
    }
    if (!prepareMutation(archive))
        return;
    start(Job::Rename, {"rn", "-y", "-bsp1", "-bb1", "--", createTemporary_, oldPath, newPath});
}

void ArchiveBackend::replaceEntry(const QString &archive, const QString &entry,
                                  const QString &sourceRoot) {
    if (!restoreListedArchive(archive)) {
        emit finished(false, QStringLiteral("Сначала откройте архив и проверьте его содержимое."));
        return;
    }
    bool found = false;
    for (const ArchiveEntry &item : entries_)
        found |= item.path == entry && !item.directory && !item.link;
    if (!found || !safePath(entry) || entry.contains('*') || entry.contains('?') ||
        entry.startsWith('@')) {
        emit finished(false, QStringLiteral("Этот файл архива нельзя заменить."));
        return;
    }
    const QDir root(sourceRoot);
    const QFileInfo source(root.filePath(entry));
    if (!root.exists() || !source.isFile() || source.isSymLink() ||
        !source.canonicalFilePath().startsWith(root.canonicalPath() + '/')) {
        emit finished(false, QStringLiteral("Изменённый файл не найден."));
        return;
    }
    QString part = root.absolutePath();
    for (const QString &component : entry.split('/')) {
        part = QDir(part).filePath(component);
        if (QFileInfo(part).isSymLink()) {
            emit finished(false, QStringLiteral("Ссылки во временном файле запрещены."));
            return;
        }
    }
    if (!prepareMutation(archive))
        return;
    replacementEntry_ = entry;
    replacementRoot_ = root.absolutePath();
    start(Job::ReplaceDelete, {"d", "-y", "-bsp1", "-bb1", "-spd", "--", createTemporary_, entry});
}

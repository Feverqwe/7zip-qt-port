#include "CPP/7zip/UI/GUI/CompressDialog.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "archivebackend.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

bool ArchiveBackend::compressionSwitches(const QString &suffix, const QString &password,
                                         bool encryptHeaders, const CompressionOptions &options,
                                         QStringList *switches, QString *error) {
    const auto &formatInfo = NCompressDialog::GetFormatInfo(suffix);
    if (options.SetArcMTime.Def) {
        *error = QStringLiteral("Изменение времени архива пока недоступно.");
        return false;
    }
    const QStringList methods = NCompressDialog::GetMethodNames(suffix);
    if (options.level < 0 || options.level > 9 ||
        !(formatInfo.LevelsMask & (UInt32(1) << options.level)) || options.threads < 0 ||
        (!options.method.isEmpty() && !methods.contains(options.method))) {
        *error = QStringLiteral("Недопустимые параметры сжатия для выбранного формата.");
        return false;
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
        *error = QStringLiteral("Число потоков не подходит для выбранного метода сжатия.");
        return false;
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
        *error = QStringLiteral("Размер словаря, слова или непрерывного блока не подходит "
                                "для выбранного метода сжатия.");
        return false;
    }
    UInt64 decompressMemory;
    const UInt64 memUsage = NCompressDialog::GetMemoryUsage_Threads_Dict_DecompMem(
        suffix, methodID, UInt32(options.level),
        options.threads ? UInt32(options.threads) : threadOptions.AutoNumThreads, dict,
        blockSizeLog, decompressMemory);
    const UInt64 memLimit = NCompressDialog::Get_MemUse_Bytes(options.MemUsage, system);
    if ((options.MemUsage.IsDefined || system.RamSizeDefined) && memUsage != UInt64(-1) &&
        memUsage > memLimit) {
        *error = QStringLiteral("Для сжатия требуется %1 памяти, лимит — %2.")
                     .arg(NCompressDialog::AddMemUsage(memUsage),
                          NCompressDialog::AddMemUsage(memLimit));
        return false;
    }
    if ((!options.EncryptionMethod.isEmpty() &&
         options.EncryptionMethod.compare("AES256", Qt::CaseInsensitive) != 0) ||
        (suffix == "7z" && !options.EncryptionMethod.isEmpty()) ||
        (suffix == "zip" && encryptHeaders)) {
        *error = QStringLiteral("Недопустимый метод шифрования для выбранного формата.");
        return false;
    }
    const auto passwordError =
        CCompressDialog::CheckPassword(password, password, true, suffix, options.EncryptionMethod);
    if (passwordError != CCompressDialog::kPasswordOK) {
        *error =
            passwordError == CCompressDialog::kPasswordTooLong
                ? QStringLiteral("Пароль ZIP AES не должен превышать 99 символов.")
                : QStringLiteral("Для ZIP используйте пароль из ASCII без управляющих символов.");
        return false;
    }
    // Qt request fields become the original GUI property DTO. Source
    // SetOutProperties/parser owns method selection, overrides and time props.
    NCompressDialog::CInfo info = options;
    info.Level = UInt32(options.level);
    info.Method = options.level ? UString(method.toStdWString().c_str()) : UString();
    info.Dict64 = options.dictionaryBytes ? UInt64(options.dictionaryBytes) : UInt64(-1);
    info.OrderMode = NCompressDialog::GetOrderMode(methodID);
    info.Order = options.wordSize ? UInt32(options.wordSize) : UInt32(-1);
    info.NumThreads = options.threads ? UInt32(options.threads) : UInt32(-1);
    info.EncryptHeadersIsAllowed = !password.isEmpty() && suffix == "7z";
    info.EncryptHeaders = encryptHeaders;
    if (password.isEmpty())
        info.EncryptionMethod.clear();
    switches->clear();
    const auto properties = NCompressDialog::GetCompressionProperties(info, suffix == "7z");
    FOR_VECTOR(i, properties) {
        const CProperty &property = properties[i];
        QString argument = "-m" + QString::fromWCharArray(property.Name.Ptr());
        if (!property.Value.IsEmpty())
            argument += '=' + QString::fromWCharArray(property.Value.Ptr());
        switches->append(argument);
    }
    if (!password.isEmpty())
        switches->append("-p");
    return true;
}

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
    QStringList compressionArguments;
    QString compressionError;
    if (!compressionSwitches(suffix, password, encryptHeaders, options, &compressionArguments,
                             &compressionError)) {
        emit finished(false, compressionError);
        return;
    }
    const QFileInfo target(archive);
    if (!QFileInfo(target.absolutePath()).isDir() || target.exists() || target.isSymLink()) {
        emit finished(false,
                      QStringLiteral("Папка назначения не найдена или архив уже существует: %1")
                          .arg(target.absoluteFilePath()));
        return;
    }
    FOR_VECTOR(i, options.VolumeSizes) {
        if (options.VolumeSizes[i] == 0 || options.VolumeSizes[i] > UInt64(INT64_MAX)) {
            emit finished(false, QStringLiteral("Недопустимый размер архивного тома."));
            return;
        }
    }
    if (!options.VolumeSizes.IsEmpty()) {
        QString error;
        if (!checkVolumeTargets(archive, &error)) {
            emit finished(false, error);
            return;
        }
    }
    QList<CreateStage> stages;
    QSet<QString> logicalNames;
    auto appendStage = [&stages](const QString &directory, const QString &name) {
        for (CreateStage &stage : stages)
            if (stage.directory == directory) {
                stage.arguments.append(name);
                return;
            }
        stages.append({directory, {name}});
    };
    for (const QString &source : sources) {
        const QFileInfo info(source);
        if (!info.exists()) {
            emit finished(false, QStringLiteral("Файл или папка не найдены: %1").arg(source));
            return;
        }
        const QString absolute = QDir::fromNativeSeparators(info.absoluteFilePath());
        QString directory;
        QString logical;
        if (options.PathMode == NWildcard::k_RelatPath) {
            directory = info.absolutePath();
            logical = info.fileName();
        } else if (options.PathMode == NWildcard::k_FullPath) {
#ifdef Q_OS_WIN
            const QRegularExpression rootPattern(R"(^(?:[A-Za-z]:/|//[^/]+/[^/]+/))");
            const auto match = rootPattern.match(absolute);
            if (!match.hasMatch()) {
                emit finished(false, QStringLiteral("Не удалось определить корень источника."));
                return;
            }
            directory = match.captured();
#else
            directory = QDir::rootPath();
#endif
            logical = absolute.mid(directory.length());
        } else if (options.PathMode == NWildcard::k_AbsPath) {
            directory = QDir::rootPath();
            logical = absolute;
        } else {
            emit finished(false, QStringLiteral("Недопустимый режим путей архива."));
            return;
        }
        if (logical.isEmpty() || logical.contains('*') || logical.contains('?') ||
            logical.startsWith('@') ||
            (options.PathMode != NWildcard::k_AbsPath && !sourceUpdatePathIsSafe(logical))) {
            emit finished(false, QStringLiteral("Недопустимый путь источника: %1").arg(source));
            return;
        }
        const QString key = logical.normalized(QString::NormalizationForm_C).toCaseFolded();
        if (logicalNames.contains(key)) {
            emit finished(
                false, QStringLiteral("Источники дают одинаковое имя в архиве: %1").arg(logical));
            return;
        }
        logicalNames.insert(key);
        appendStage(directory, logical);
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
    bool firstStage = true;
    for (CreateStage &stage : stages) {
        QStringList arguments = {"a", "-y", "-bsp1", "-bb1", "-spd", "-t" + suffix};
        arguments += compressionArguments;
        // Later roots update the encrypted output created by the first stage.
        // Reuse its opened password instead of prompting for a new one too.
        if (!firstStage && suffix == "7z" && !password.isEmpty() && encryptHeaders)
            arguments.removeAll("-p");
        firstStage = false;
        if (options.PathMode == NWildcard::k_AbsPath)
            arguments << "-spf";
        arguments << "--" << createTemporary_;
        arguments += stage.arguments;
        stage.arguments = arguments;
    }
    createStages_ = stages;
    createVolumeSizes_ = options.VolumeSizes;
    createPassword_ = password;
    const CreateStage first = createStages_.takeFirst();
    pendingPassword_ = password;
    start(Job::Create, first.arguments, first.directory);
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
